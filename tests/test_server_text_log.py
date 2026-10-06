"""
Integration tests for APX server text log extension configuration and formatting.
"""

import json
import os
import re
import select
import socket
import subprocess
import pytest
from conftest import _set_pdeathsig, wait_for_unix_socket


def test_server_text_log_file_and_timestamp(tmp_path, apx_server_bin: str):
    """Verify that apx-server logs events with timestamps and configured log level to a file."""
    socket_path = str(tmp_path / "apx_test.socket")
    log_path = str(tmp_path / "server.log")
    config_path = str(tmp_path / "server.json")

    config = {
        "socket-server-extension": {
            "unix-file": socket_path
        },
        "textlog-extension": {
            "enabled": True,
            "file-enabled": True,
            "file-path": log_path,
            "log-level": "DEBUG",
            "use-timestamp": True
        }
    }

    with open(config_path, "w", encoding="utf-8") as f:
        json.dump(config, f)

    r_fd, w_fd = os.pipe()
    os.set_inheritable(w_fd, True)

    cmd = [apx_server_bin, "--ready-fd", str(w_fd), config_path]
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

    wait_for_unix_socket(socket_path, timeout=3.0)

    # Connect and disconnect a client
    client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    client.connect(socket_path)
    client.close()

    # Gracefully stop server
    proc.terminate()
    proc.wait(timeout=3.0)
    assert proc.returncode == 0

    assert os.path.exists(log_path), "Log file should have been created"
    with open(log_path, "r", encoding="utf-8") as f:
        log_content = f.read()

    assert len(log_content) > 0, "Log file should not be empty"

    # Verify timestamp and level formatting: [YYYY-MM-DD HH:MM:SS.mmm] [<LEVEL>] [<LABEL>] <message>
    pattern = re.compile(
        r"^\[\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3}\] \[(INFO|DEBUG|WARNING|ERROR|CRITICAL)\] \[[^\]]+\] .+$",
        re.MULTILINE,
    )
    matches = pattern.findall(log_content)
    assert len(matches) > 0, f"Expected formatted log lines with timestamp in:\n{log_content}"


def test_server_text_log_level_filter(tmp_path, apx_server_bin: str):
    """Verify that events below configured log-level (ERROR) are not written to the log file."""
    socket_path = str(tmp_path / "apx_test.socket")
    log_path = str(tmp_path / "server_filtered.log")
    config_path = str(tmp_path / "server.json")

    config = {
        "socket-server-extension": {
            "unix-file": socket_path
        },
        "textlog-extension": {
            "enabled": True,
            "file-enabled": True,
            "file-path": log_path,
            "log-level": "ERROR",
            "use-timestamp": False
        }
    }

    with open(config_path, "w", encoding="utf-8") as f:
        json.dump(config, f)

    r_fd, w_fd = os.pipe()
    os.set_inheritable(w_fd, True)

    cmd = [apx_server_bin, "--ready-fd", str(w_fd), config_path]
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

    wait_for_unix_socket(socket_path, timeout=3.0)

    client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    client.connect(socket_path)
    client.close()

    proc.terminate()
    proc.wait(timeout=3.0)
    assert proc.returncode == 0

    assert os.path.exists(log_path), "Log file should have been created"
    with open(log_path, "r", encoding="utf-8") as f:
        log_content = f.read()

    # Normal connection / debug events should NOT be logged since level is ERROR
    assert "[DEBUG]" not in log_content
    assert "[INFO]" not in log_content
