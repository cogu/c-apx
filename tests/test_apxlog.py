"""
Integration tests for apx-log CLI application and .apxlog container files.
"""

import json
import os
import signal
import subprocess
import time
from pathlib import Path
import pytest
from conftest import _set_pdeathsig, wait_for_unix_socket, ApxServerInstance, ApxNodeInstance


@pytest.fixture
def apx_server_with_monitor(tmp_path: Path, apx_server_bin: str):
    """
    Starts an apx-server instance configured with both standard socket
    extension and the monitor diagnostic extension on UNIX domain sockets.
    """
    server_socket = str(tmp_path / "apx.socket")
    monitor_socket = str(tmp_path / "monitor.socket")
    config_file = str(tmp_path / "server.json")

    config = {
        "socket-server-extension": {
            "unix-file": server_socket
        },
        "monitor-extension": {
            "enabled": True,
            "socket-file": monitor_socket
        }
    }

    with open(config_file, "w", encoding="utf-8") as f:
        json.dump(config, f)

    r_fd, w_fd = os.pipe()
    os.set_inheritable(w_fd, True)

    cmd = [apx_server_bin, "--ready-fd", str(w_fd), config_file]
    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        pass_fds=(w_fd,),
        preexec_fn=_set_pdeathsig,
    )
    os.close(w_fd)

    import select
    rlist, _, _ = select.select([r_fd], [], [], 3.0)
    assert rlist, "Server did not signal readiness in time"
    os.read(r_fd, 1)
    os.close(r_fd)

    wait_for_unix_socket(server_socket, timeout=3.0)
    wait_for_unix_socket(monitor_socket, timeout=3.0)

    server = ApxServerInstance(proc, server_socket, tmp_path, config_path=config_file)
    server.monitor_socket = monitor_socket

    yield server

    proc.terminate()
    try:
        proc.wait(timeout=2.0)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait()


def test_apxlog_help_and_version(apx_log_bin: str):
    """Verify help and version options."""
    res = subprocess.run([apx_log_bin, "--help"], capture_output=True, text=True, check=True)
    assert "Usage: apx-log" in res.stdout
    assert "record" in res.stdout
    assert "info" in res.stdout
    assert "print" in res.stdout

    res = subprocess.run([apx_log_bin, "--version"], capture_output=True, text=True, check=True)
    assert "apx-log" in res.stdout

    res = subprocess.run([apx_log_bin, "record", "--help"], capture_output=True, text=True, check=True)
    assert "Usage: apx-log record" in res.stdout
    assert "--connect" in res.stdout

    res = subprocess.run([apx_log_bin, "info", "--help"], capture_output=True, text=True, check=True)
    assert "Usage: apx-log info" in res.stdout

    res = subprocess.run([apx_log_bin, "print", "--help"], capture_output=True, text=True, check=True)
    assert "Usage: apx-log print" in res.stdout
    assert "--json" in res.stdout


def test_apxlog_record_info_print(apx_server_with_monitor, apx_log_bin: str, tmp_path: Path):
    """
    Test recording an event stream from a running server, stopping via SIGINT,
    and inspecting/printing the resulting .apxlog container.
    """
    log_file = tmp_path / "session.apxlog"

    # Start recording process
    cmd = [
        apx_log_bin,
        "record",
        "-c", apx_server_with_monitor.monitor_socket,
        str(log_file)
    ]
    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        preexec_fn=_set_pdeathsig
    )

    # Let recorder connect and capture STREAM_BEGIN + SNAPSHOT_END
    time.sleep(0.8)

    # Send SIGINT to gracefully finalize recording
    proc.send_signal(signal.SIGINT)
    stdout, stderr = proc.communicate(timeout=3.0)
    assert proc.returncode == 0, f"Recorder failed (code={proc.returncode}): {stderr.decode()}"
    assert "Recording stopped" in stdout.decode()

    assert log_file.exists()
    assert log_file.stat().st_size >= 64

    # 1. Test 'apx-log info'
    res_info = subprocess.run(
        [apx_log_bin, "info", str(log_file)],
        capture_output=True, text=True, check=True
    )
    assert "Format Version:  1.0" in res_info.stdout
    assert "Status:          finalized" in res_info.stdout
    assert "Record Count:    " in res_info.stdout

    # 2. Test 'apx-log info --json'
    res_info_json = subprocess.run(
        [apx_log_bin, "info", "--json", str(log_file)],
        capture_output=True, text=True, check=True
    )
    info_data = json.loads(res_info_json.stdout)
    assert info_data["format_major"] == 1
    assert info_data["format_minor"] == 0
    assert info_data["finalized"] is True
    assert info_data["record_count"] >= 2
    assert info_data["file_size"] >= 64

    # 3. Test 'apx-log print'
    res_print = subprocess.run(
        [apx_log_bin, "print", str(log_file)],
        capture_output=True, text=True, check=True
    )
    assert "STREAM_BEGIN" in res_print.stdout
    assert "SNAPSHOT_END" in res_print.stdout

    # 4. Test 'apx-log print --json' (NDJSON output)
    res_print_json = subprocess.run(
        [apx_log_bin, "print", "--json", str(log_file)],
        capture_output=True, text=True, check=True
    )
    lines = [json.loads(line) for line in res_print_json.stdout.strip().splitlines() if line.strip()]
    assert len(lines) >= 2
    events = [line["event"] for line in lines]
    assert "STREAM_BEGIN" in events
    assert "SNAPSHOT_END" in events
    assert events[0] == "STREAM_BEGIN"


def _spawn_node(cmd: list[str], apx_file: Path, bind_path: str | None = None) -> ApxNodeInstance:
    import shutil
    stdbuf_bin = shutil.which("stdbuf")
    if stdbuf_bin:
        cmd = [stdbuf_bin, "-oL", *cmd]
    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1,
        preexec_fn=_set_pdeathsig
    )
    return ApxNodeInstance(proc, apx_file, bind_path=bind_path)


def test_apxlog_record_with_node_traffic(
    apx_server_with_monitor, apx_log_bin: str, apx_node_bin: str, tmp_path: Path
):
    """
    Test recording an event stream while a client node connects, attaches, and publishes files.
    """
    log_file = tmp_path / "traffic.apxlog"
    repo_root = Path(__file__).resolve().parent.parent
    sender_apx = repo_root / "example" / "nodes" / "unsigned_sender.apx"
    assert sender_apx.is_file()

    # Start recorder
    rec_cmd = [
        apx_log_bin,
        "record",
        "-c", apx_server_with_monitor.monitor_socket,
        str(log_file)
    ]
    rec_proc = subprocess.Popen(
        rec_cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        preexec_fn=_set_pdeathsig
    )

    # Allow recorder to connect and receive initial snapshot
    time.sleep(0.5)

    # Spawn node connecting to the server
    node_cmd = [
        apx_node_bin,
        "-c", apx_server_with_monitor.socket_path,
        "--no-bind",
        str(sender_apx)
    ]
    node = _spawn_node(node_cmd, sender_apx)
    try:
        assert node.wait_for_output("[APX-CONNECTION] connected to APX server", timeout=3.0)
        time.sleep(0.5)
    finally:
        node.stop()

    time.sleep(0.3)
    rec_proc.send_signal(signal.SIGINT)
    stdout, stderr = rec_proc.communicate(timeout=3.0)
    assert rec_proc.returncode == 0, f"Recorder failed (code={rec_proc.returncode}): {stderr.decode()}"

    # Print NDJSON
    res_print_json = subprocess.run(
        [apx_log_bin, "print", "--json", str(log_file)],
        capture_output=True, text=True, check=True
    )
    lines = [json.loads(line) for line in res_print_json.stdout.strip().splitlines() if line.strip()]
    events = [line["event"] for line in lines]

    assert "STREAM_BEGIN" in events
    assert "SNAPSHOT_END" in events
    assert "CONNECTION_CREATE" in events
    assert "FILE_PUBLISH" in events
    assert "FILE_OPEN" in events

