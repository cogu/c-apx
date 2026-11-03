"""
Tests for APX server unknown configuration key warnings (Backlog Item 15).
"""

import json
import os
import select
import socket
import subprocess
from conftest import _set_pdeathsig


def test_server_unknown_section_and_keys_warn_and_continue(tmp_path, apx_server_bin: str):
    """Verify that unknown sections and keys emit warnings on stderr and server continues starting."""
    socket_path = str(tmp_path / "apx.socket")
    config_file = str(tmp_path / "server.json")

    config_data = {
        "unknown-section": {
            "key": "val"
        },
        "orphan-key": "hello",
        "apx-server": {
            "security": {
                "bogus-sec-key": True
            }
        },
        "textlog-extension": {
            "use-timestmp": True,
            "enabled": True
        },
        "socket-server-extension": {
            "unix-file": socket_path
        }
    }

    with open(config_file, "w", encoding="utf-8") as f:
        json.dump(config_data, f)

    r_fd, w_fd = os.pipe()
    os.set_inheritable(w_fd, True)

    cmd = [
        apx_server_bin,
        "--ready-fd", str(w_fd),
        config_file
    ]

    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        pass_fds=(w_fd,),
        preexec_fn=_set_pdeathsig
    )
    os.close(w_fd)

    # Wait for ready signal (proves server continued startup despite warnings)
    rlist, _, _ = select.select([r_fd], [], [], 3.0)
    assert rlist, "Server should signal readiness despite unknown configuration keys"
    os.read(r_fd, 1)
    os.close(r_fd)

    # Verify server is accepting connections
    client_sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    client_sock.connect(socket_path)
    client_sock.close()

    # Graceful shutdown
    proc.terminate()
    out, err = proc.communicate(timeout=3.0)
    assert proc.returncode == 0, f"Server exited with non-zero code: {proc.returncode}"

    stderr_text = err.decode("utf-8", errors="replace")

    assert "Warning: Unknown configuration section 'unknown-section'" in stderr_text
    assert "Warning: Unknown configuration key 'orphan-key'" in stderr_text
    assert "Warning: Unknown configuration key 'apx-server.security.bogus-sec-key'" in stderr_text
    assert "Warning: Unknown configuration key 'textlog-extension.use-timestmp'" in stderr_text


def test_server_socket_config_unknown_key_warns_and_continues(tmp_path, apx_server_bin: str):
    """Verify that unknown keys passed via --socket-config inline JSON emit warnings."""
    socket_path = str(tmp_path / "apx_override.socket")
    socket_cfg = json.dumps({
        "unix-file": socket_path,
        "invalid-sock-param": 123
    })

    r_fd, w_fd = os.pipe()
    os.set_inheritable(w_fd, True)

    cmd = [
        apx_server_bin,
        "--ready-fd", str(w_fd),
        "--socket-config", socket_cfg
    ]

    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        pass_fds=(w_fd,),
        preexec_fn=_set_pdeathsig
    )
    os.close(w_fd)

    rlist, _, _ = select.select([r_fd], [], [], 3.0)
    assert rlist, "Server should signal readiness"
    os.read(r_fd, 1)
    os.close(r_fd)

    client_sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    client_sock.connect(socket_path)
    client_sock.close()

    proc.terminate()
    out, err = proc.communicate(timeout=3.0)
    assert proc.returncode == 0

    stderr_text = err.decode("utf-8", errors="replace")
    assert "Warning: Unknown configuration key 'socket-server-extension.invalid-sock-param'" in stderr_text


def test_server_recognized_keys_and_aliases_no_warnings(tmp_path, apx_server_bin: str):
    """Verify that recognized keys and supported aliases do not trigger any warnings."""
    socket_path = str(tmp_path / "apx_clean.socket")
    config_file = str(tmp_path / "server_clean.json")

    config_data = {
        "apx-server": {
            "security": {
                "require_signed_nodes": False,
                "trusted_keys": []
            },
            "multiple-provider-policy": "first",
            "max-concurrent-connections": 128,
            "apx-cache-enabled": False
        },
        "socket-server-extension": {
            "enabled": True,
            "unix-file": socket_path,
            "port": 5099,
            "tag": "local"
        },
        "textlog-extension": {
            "enabled": True,
            "log_level": "DEBUG",
            "use_timestamp": False
        }
    }

    with open(config_file, "w", encoding="utf-8") as f:
        json.dump(config_data, f)

    r_fd, w_fd = os.pipe()
    os.set_inheritable(w_fd, True)

    cmd = [
        apx_server_bin,
        "--ready-fd", str(w_fd),
        config_file
    ]

    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        pass_fds=(w_fd,),
        preexec_fn=_set_pdeathsig
    )
    os.close(w_fd)

    rlist, _, _ = select.select([r_fd], [], [], 3.0)
    assert rlist, "Server should signal readiness"
    os.read(r_fd, 1)
    os.close(r_fd)

    client_sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    client_sock.connect(socket_path)
    client_sock.close()

    proc.terminate()
    out, err = proc.communicate(timeout=3.0)
    assert proc.returncode == 0

    stderr_text = err.decode("utf-8", errors="replace")
    assert "Warning: Unknown configuration" not in stderr_text


def test_server_unsupported_underscore_key_emits_warning(tmp_path, apx_server_bin: str):
    """Verify that an unsupported underscore key variant emits a warning."""
    config_file = str(tmp_path / "server_underscore.json")

    config_data = {
        "socket-server-extension": {
            "unix_file": "/tmp/test_underscore.socket"
        }
    }

    with open(config_file, "w", encoding="utf-8") as f:
        json.dump(config_data, f)

    cmd = [
        apx_server_bin,
        "--help"  # We just want to check config warning; running server briefly or checking warning
    ]
    # Run server without ready fd, terminate after short time
    proc = subprocess.Popen(
        [apx_server_bin, config_file],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        preexec_fn=_set_pdeathsig
    )
    import time
    time.sleep(0.5)
    proc.terminate()
    out, err = proc.communicate(timeout=3.0)

    stderr_text = err.decode("utf-8", errors="replace")
    assert "Warning: Unknown configuration key 'socket-server-extension.unix_file'" in stderr_text


def test_server_missing_minimum_extension_config_warns_and_continues(tmp_path, apx_server_bin: str):
    """Verify that enabled extensions with missing minimum configuration warn and continue."""
    config_file = str(tmp_path / "server_empty_ext.json")

    config_data = {
        "socket-server-extension": {
            "enabled": True
        },
        "monitor-extension": {
            "enabled": True
        }
    }

    with open(config_file, "w", encoding="utf-8") as f:
        json.dump(config_data, f)

    r_fd, w_fd = os.pipe()
    os.set_inheritable(w_fd, True)

    cmd = [
        apx_server_bin,
        "--ready-fd", str(w_fd),
        config_file
    ]

    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        pass_fds=(w_fd,),
        preexec_fn=_set_pdeathsig
    )
    os.close(w_fd)

    rlist, _, _ = select.select([r_fd], [], [], 3.0)
    assert rlist, "Server should signal readiness despite minimum configuration warnings"
    os.read(r_fd, 1)
    os.close(r_fd)

    proc.terminate()
    out, err = proc.communicate(timeout=3.0)
    assert proc.returncode == 0

    stderr_text = err.decode("utf-8", errors="replace")
    assert "Warning: 'socket-server-extension' is enabled, but no listening transports are configured" in stderr_text
    assert "Warning: 'monitor-extension' is enabled, but no listening endpoints are configured" in stderr_text


def test_server_retired_max_num_events_warns(tmp_path, apx_server_bin: str):
    """Verify that the retired 'max-num-events' configuration key emits an unknown key warning."""
    config_file = str(tmp_path / "server_retired_events.json")

    config_data = {
        "apx-server": {
            "max-num-events": 200
        }
    }

    with open(config_file, "w", encoding="utf-8") as f:
        json.dump(config_data, f)

    proc = subprocess.Popen(
        [apx_server_bin, config_file],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        preexec_fn=_set_pdeathsig
    )
    import time
    time.sleep(0.5)
    proc.terminate()
    out, err = proc.communicate(timeout=3.0)

    stderr_text = err.decode("utf-8", errors="replace")
    assert "Warning: Unknown configuration key 'apx-server.max-num-events'" in stderr_text


def test_server_invalid_max_concurrent_connections_warns(tmp_path, apx_server_bin: str):
    """Verify that invalid max-concurrent-connections emits a warning and server continues."""
    config_file = str(tmp_path / "server_invalid_conn.json")

    config_data = {
        "apx-server": {
            "max-concurrent-connections": -5
        }
    }

    with open(config_file, "w", encoding="utf-8") as f:
        json.dump(config_data, f)

    proc = subprocess.Popen(
        [apx_server_bin, config_file],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        preexec_fn=_set_pdeathsig
    )
    import time
    time.sleep(0.5)
    proc.terminate()
    out, err = proc.communicate(timeout=3.0)

    stderr_text = err.decode("utf-8", errors="replace")
    assert "Warning: Invalid value '-5' for 'apx-server.max-concurrent-connections' (expected integer between 1 and 65535); defaulting to 256" in stderr_text




