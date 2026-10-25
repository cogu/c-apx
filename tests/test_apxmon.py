"""
Integration tests for apxmon CLI diagnostic application and APX diagnostic protocol.
"""

import json
import os
import select
import subprocess
import time
from pathlib import Path
import pytest
from conftest import _set_pdeathsig, wait_for_unix_socket, ApxServerInstance, ApxNodeInstance, ApxControl


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

    rlist, _, _ = select.select([r_fd], [], [], 3.0)
    assert rlist, "Server did not signal readiness in time"
    os.read(r_fd, 1)
    os.close(r_fd)

    wait_for_unix_socket(server_socket, timeout=3.0)
    wait_for_unix_socket(monitor_socket, timeout=3.0)

    server = ApxServerInstance(proc, server_socket, tmp_path, config_path=config_file)
    server.monitor_socket = monitor_socket

    yield server

    server.stop(timeout=3.0)
    assert server.process.returncode == 0, (
        f"Server exited with unexpected code: {server.process.returncode}\n"
        f"Stderr: {server.stderr}\nStdout: {server.stdout}"
    )


def test_apxmon_help_and_version(apxmon_bin: str):
    """Verify that apxmon responds to -h and -v."""
    res = subprocess.run([apxmon_bin, "-h"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True)
    assert "Usage: apxmon [OPTIONS] COMMAND" in res.stdout
    assert "info" in res.stdout
    assert "clients" in res.stdout
    assert "nodes" in res.stdout
    assert "routing" in res.stdout
    assert "get-def" in res.stdout
    assert "watch" in res.stdout

    res_ver = subprocess.run([apxmon_bin, "-v"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True)
    assert "apxmon" in res_ver.stdout


def test_apxmon_info(apx_server_with_monitor, apxmon_bin: str):
    """Verify apxmon info command in text and json format."""
    sock = apx_server_with_monitor.monitor_socket

    # Text output
    res = subprocess.run([apxmon_bin, "-c", sock, "info"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True)
    assert "Server Version:" in res.stdout
    assert "Uptime:" in res.stdout
    assert "Active Connections:" in res.stdout
    assert "Active Nodes:" in res.stdout

    # JSON output
    res_json = subprocess.run([apxmon_bin, "-c", sock, "--json", "info"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True)
    data = json.loads(res_json.stdout)
    assert "server_version" in data
    assert "uptime_ms" in data
    assert "active_connections" in data
    assert "active_nodes" in data
    assert "total_ports" in data
    assert isinstance(data["uptime_ms"], int)


def test_apxmon_clients(apx_server_with_monitor, apxmon_bin: str):
    """Verify apxmon clients command in text and json format."""
    sock = apx_server_with_monitor.monitor_socket

    # Text output
    res = subprocess.run([apxmon_bin, "-c", sock, "clients"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True)
    assert "ID" in res.stdout
    assert "TYPE" in res.stdout
    assert "STATE" in res.stdout

    # JSON output
    res_json = subprocess.run([apxmon_bin, "-c", sock, "--json", "clients"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True)
    clients = json.loads(res_json.stdout)
    assert isinstance(clients, list)


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


def test_apxmon_nodes_and_get_def(apx_server_with_monitor, apxmon_bin: str, apx_node_bin: str, tmp_path: Path):
    """Verify nodes listing and get-def command."""
    sock = apx_server_with_monitor.monitor_socket
    repo_root = Path(__file__).resolve().parent.parent
    sender_apx = repo_root / "example" / "nodes" / "unsigned_sender.apx"
    assert sender_apx.is_file()

    # Start unsigned_sender node connected to the server's primary socket
    node_cmd = [apx_node_bin, "-c", apx_server_with_monitor.socket_path, "--no-bind", str(sender_apx)]
    node = _spawn_node(node_cmd, sender_apx)

    try:
        assert node.wait_for_output("[APX-CONNECTION] connected to APX server", timeout=3.0)

        # Run nodes in text format
        res = subprocess.run([apxmon_bin, "-c", sock, "nodes"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True)
        assert "apx_sender" in res.stdout

        # Run nodes in json format
        res_json = subprocess.run([apxmon_bin, "-c", sock, "--json", "nodes"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True)
        nodes = json.loads(res_json.stdout)
        assert isinstance(nodes, list)
        matching = [n for n in nodes if n.get("node_name") == "apx_sender"]
        assert len(matching) == 1
        assert matching[0]["provide_port_count"] >= 1

        # Run get-def in text format
        res_def = subprocess.run([apxmon_bin, "-c", sock, "get-def", "apx_sender"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        assert res_def.returncode == 0, f"res_def failed:\nstdout: {res_def.stdout}\nstderr: {res_def.stderr}"
        assert "APX/1.3" in res_def.stdout
        assert 'N"apx_sender"' in res_def.stdout

        # Run get-def in json format
        res_def_json = subprocess.run([apxmon_bin, "-c", sock, "--json", "get-def", "apx_sender"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        assert res_def_json.returncode == 0, f"res_def_json failed:\nstdout: {res_def_json.stdout}\nstderr: {res_def_json.stderr}"
        def_obj = json.loads(res_def_json.stdout)
        assert def_obj.get("node_name") == "apx_sender"
        assert "APX/1.3" in def_obj.get("definition", "")
    finally:
        node.stop()


def test_apxmon_routing(apx_server_with_monitor, apxmon_bin: str, apx_node_bin: str):
    """Verify routing and routing --unconnected command."""
    sock = apx_server_with_monitor.monitor_socket
    repo_root = Path(__file__).resolve().parent.parent
    listener_apx = repo_root / "example" / "nodes" / "unsigned_listener.apx"
    sender_apx = repo_root / "example" / "nodes" / "unsigned_sender.apx"

    # Start listener node
    listener_cmd = [apx_node_bin, "-c", apx_server_with_monitor.socket_path, "--no-bind", str(listener_apx)]
    listener = _spawn_node(listener_cmd, listener_apx)

    try:
        assert listener.wait_for_output("[APX-CONNECTION] connected to APX server", timeout=3.0)
        time.sleep(0.3)

        # Before sender is connected: VehicleSpeed should be unconnected
        res_unconn = subprocess.run([apxmon_bin, "-c", sock, "routing", "--unconnected"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True)
        assert "VehicleSpeed" in res_unconn.stdout
        assert "REQUIRE" in res_unconn.stdout

        res_unconn_json = subprocess.run([apxmon_bin, "-c", sock, "--json", "routing", "--unconnected"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True)
        unconn_list = json.loads(res_unconn_json.stdout)
        assert any(item.get("port_name") == "VehicleSpeed" for item in unconn_list)

        # Start sender node
        sender_cmd = [apx_node_bin, "-c", apx_server_with_monitor.socket_path, "--no-bind", str(sender_apx)]
        sender = _spawn_node(sender_cmd, sender_apx)

        try:
            assert sender.wait_for_output("[APX-CONNECTION] connected to APX server", timeout=3.0)
            time.sleep(0.3)

            # Route should now exist
            res_routes = subprocess.run([apxmon_bin, "-c", sock, "routing"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True)
            assert "VehicleSpeed" in res_routes.stdout

            res_routes_json = subprocess.run([apxmon_bin, "-c", sock, "--json", "routing"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True)
            route_list = json.loads(res_routes_json.stdout)
            assert any(item.get("port_name") == "VehicleSpeed" for item in route_list)
        finally:
            sender.stop()
    finally:
        listener.stop()


def test_apxmon_watch(apx_server_with_monitor, apxmon_bin: str, apx_node_bin: str, apx_control: ApxControl, tmp_path: Path):
    """Verify live signal watch notifications via apxmon watch command."""
    sock = apx_server_with_monitor.monitor_socket
    repo_root = Path(__file__).resolve().parent.parent
    sender_apx = repo_root / "example" / "nodes" / "unsigned_sender.apx"

    bind_socket = str(tmp_path / "sender.socket")
    sender_cmd = [apx_node_bin, "-c", apx_server_with_monitor.socket_path, "-b", bind_socket, str(sender_apx)]
    sender = _spawn_node(sender_cmd, sender_apx, bind_path=bind_socket)

    try:
        assert sender.wait_for_output("[APX-CONNECTION] connected to APX server", timeout=3.0)
        wait_for_unix_socket(bind_socket, timeout=3.0)

        # Launch apxmon watch as background process
        watch_cmd = [apxmon_bin, "-c", sock, "--json", "watch", "apx_sender:VehicleSpeed"]
        watch_proc = subprocess.Popen(
            watch_cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1,
            preexec_fn=_set_pdeathsig
        )

        try:
            # Send signal update via apx_control
            time.sleep(0.3)
            res = apx_control.set_signal("VehicleSpeed", 123, connect_path=bind_socket)
            assert res.returncode == 0

            # Read lines from watch_proc stdout
            start = time.monotonic()
            received_signal = False
            while time.monotonic() - start < 3.0:
                line = watch_proc.stdout.readline()
                if line:
                    try:
                        entry = json.loads(line)
                        if entry.get("type") == "PORT_VALUE":
                            received_signal = True
                            break
                    except Exception:
                        pass
                else:
                    time.sleep(0.05)

            assert received_signal, "apxmon watch did not receive expected PORT_VALUE notification"
        finally:
            watch_proc.terminate()
            try:
                watch_proc.wait(timeout=1.0)
            except subprocess.TimeoutExpired:
                watch_proc.kill()
    finally:
        sender.stop()


def test_apxmon_tls_mtls(tmp_path: Path, apx_server_bin: str, apxmon_bin: str):
    """Verify that apxmon can connect to the monitor extension over TLS with mTLS."""
    import socket
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind(("127.0.0.1", 0))
        monitor_port = s.getsockname()[1]

    repo_root = Path(__file__).resolve().parent.parent
    certs_dir = repo_root / "tests" / "certs"
    ca_cert = str(certs_dir / "ca_cert.pem")
    server_cert = str(certs_dir / "server_cert.pem")
    server_key = str(certs_dir / "server_key.pem")
    client_cert = str(certs_dir / "client_cert.pem")
    client_key = str(certs_dir / "client_key.pem")

    server_socket = str(tmp_path / "apx.socket")
    config_file = str(tmp_path / "server.json")

    config = {
        "socket-server-extension": {
            "unix-file": server_socket
        },
        "monitor-extension": {
            "enabled": True,
            "tcp-port": monitor_port,
            "bind-address": "127.0.0.1",
            "tls": {
                "server-cert": server_cert,
                "server-key": server_key,
                "ca-cert": ca_cert,
                "require-client-cert": True
            }
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

    rlist, _, _ = select.select([r_fd], [], [], 3.0)
    assert rlist, "Server did not signal readiness in time"
    os.read(r_fd, 1)
    os.close(r_fd)

    try:
        # 1. Connect with valid client cert -> should succeed
        res = subprocess.run([
            apxmon_bin,
            "-c", f"127.0.0.1:{monitor_port}",
            "--tls",
            "--ca-cert", ca_cert,
            "--client-cert", client_cert,
            "--client-key", client_key,
            "info"
        ], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True)
        assert "Server Version:" in res.stdout

        # 2. Connect without client cert -> should fail due to require-client-cert
        res_fail = subprocess.run([
            apxmon_bin,
            "-c", f"127.0.0.1:{monitor_port}",
            "--tls",
            "--ca-cert", ca_cert,
            "info"
        ], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        assert res_fail.returncode != 0
    finally:
        proc.terminate()
        try:
            proc.wait(timeout=3.0)
        except subprocess.TimeoutExpired:
            proc.kill()
