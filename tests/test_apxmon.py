"""
Integration tests for apxmon CLI diagnostic application and APX diagnostic protocol.
"""

import json
import os
import re
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
    startup_wall_ns = time.time_ns()
    startup_monotonic_ns = time.monotonic_ns()
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
    ready_wall_ns = time.time_ns()
    ready_monotonic_ns = time.monotonic_ns()

    wait_for_unix_socket(server_socket, timeout=3.0)
    wait_for_unix_socket(monitor_socket, timeout=3.0)

    server = ApxServerInstance(proc, server_socket, tmp_path, config_path=config_file)
    server.monitor_socket = monitor_socket
    server.startup_wall_time_ns = (startup_wall_ns, ready_wall_ns)
    server.startup_monotonic_time_ns = (startup_monotonic_ns, ready_monotonic_ns)

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
    assert re.search(r"^Uptime:\s+\d+h \d+m \d+s \(\d+ ms\)$", res.stdout, re.MULTILINE)
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


def test_apxmon_server_uptime(apx_server_with_monitor, apxmon_bin: str):
    command = [apxmon_bin, "-c", apx_server_with_monitor.monitor_socket, "--json", "info"]
    startup_wall_ns, ready_wall_ns = apx_server_with_monitor.startup_wall_time_ns
    startup_monotonic_ns, ready_monotonic_ns = apx_server_with_monitor.startup_monotonic_time_ns

    def query_info():
        before_ns = time.monotonic_ns()
        result = subprocess.run(command, capture_output=True, text=True, check=True, timeout=5.0)
        after_ns = time.monotonic_ns()
        data = json.loads(result.stdout)
        assert before_ns - ready_monotonic_ns <= data["uptime_ns"] <= after_ns - startup_monotonic_ns
        assert startup_wall_ns <= data["server_start_unix_ns"] <= ready_wall_ns
        assert data["uptime_ms"] == data["uptime_ns"] // 1_000_000
        return data

    first = query_info()
    second = query_info()
    assert second["server_start_unix_ns"] == first["server_start_unix_ns"]
    assert second["uptime_ns"] > first["uptime_ns"]


def test_apxmon_top_server_uptime(apx_server_with_monitor, apxmon_bin: str):
    process = subprocess.Popen(
        [apxmon_bin, "-c", apx_server_with_monitor.monitor_socket, "top"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        preexec_fn=_set_pdeathsig,
    )
    try:
        try:
            output, error = process.communicate(timeout=1.2)
        except subprocess.TimeoutExpired:
            process.terminate()
            output, error = process.communicate(timeout=5.0)
        assert process.returncode == 0, error
        uptime_lines = [line for line in output.splitlines() if line.startswith("Uptime:")]
        assert uptime_lines, output
        max_uptime_seconds = (
            time.monotonic_ns() - apx_server_with_monitor.startup_monotonic_time_ns[0]
        ) // 1_000_000_000
        for line in uptime_lines:
            sample = re.fullmatch(r"Uptime:\s+(\d+)h (\d+)m (\d+)s", line)
            assert sample is not None, line
            hours, minutes, seconds = map(int, sample.groups())
            assert 0 <= minutes < 60
            assert 0 <= seconds < 60
            assert 0 <= hours * 3600 + minutes * 60 + seconds <= max_uptime_seconds
    finally:
        if process.poll() is None:
            process.kill()
            process.communicate(timeout=5.0)


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


@pytest.mark.parametrize("unconnected", [False, True])
def test_apxmon_routing_empty(apx_server_with_monitor, apxmon_bin: str, unconnected: bool):
    args = ["routing", *(["--unconnected"] if unconnected else [])]
    command = [apxmon_bin, "-c", apx_server_with_monitor.monitor_socket]
    result = subprocess.run([*command, *args], capture_output=True, text=True, check=True, timeout=5.0)
    lines = result.stdout.splitlines()
    expected_header = (
        ["NODE_NAME", "DIRECTION", "REASON", "PORT_NAME"]
        if unconnected else ["PORT_NAME", "PROVIDER_NODE", "REQUIRER_NODE"]
    )
    assert len(lines) == 2
    assert lines[0].split() == expected_header
    result_json = subprocess.run(
        [*command, "--json", *args], capture_output=True, text=True, check=True, timeout=5.0
    )
    assert json.loads(result_json.stdout) == []


@pytest.mark.parametrize("duplicate_names", [False, True])
def test_apxmon_routing_names_and_fanout(
    apx_server_with_monitor, apxmon_bin: str, apx_node_bin: str, tmp_path: Path, duplicate_names: bool
):
    provider = "PowertrainControllerWithALongName"
    requirers = (
        (provider, provider) if duplicate_names else ("Dashboard", "DiagnosticRecorderWithALongName")
    )
    port_name = "VehicleSpeedWithALongPortName"
    definitions = [
        (provider, f'P"{port_name}"C\nP"UnconsumedValue"C\n'),
        (requirers[0], f'R"{port_name}"C\nR"MissingValue"C\n'),
        (requirers[1], f'R"{port_name}"C\n'),
    ]
    nodes = []
    command = [apxmon_bin, "-c", apx_server_with_monitor.monitor_socket]
    try:
        for index, (node_name, ports) in enumerate(definitions):
            subprocess.run(
                [*command, "info"], capture_output=True, text=True, check=True, timeout=5.0
            )
            definition = tmp_path / f"node_{index}.apx"
            definition.write_text(f'APX/1.3\nN"{node_name}"\n{ports}', encoding="utf-8")
            node = _spawn_node(
                [apx_node_bin, "-c", apx_server_with_monitor.socket_path, "--no-bind", str(definition)],
                definition,
            )
            nodes.append(node)
            assert node.wait_for_output("[APX-CONNECTION] connected to APX server", timeout=3.0)

        deadline = time.monotonic() + 3.0
        while True:
            result_json = subprocess.run(
                [*command, "--json", "routing"], capture_output=True, text=True, check=True, timeout=5.0
            )
            routes = json.loads(result_json.stdout)
            if len(routes) == 2:
                break
            assert time.monotonic() < deadline, routes
            time.sleep(0.05)

        for route in routes:
            assert set(route) == {
                "port_name", "provide_node_id", "provide_port_id", "require_node_id", "require_port_id"
            }
            assert route["port_name"] == port_name
            assert all(isinstance(value, int) for key, value in route.items() if key != "port_name")
        assert len({route["provide_node_id"] for route in routes}) == 1
        assert len({route["require_node_id"] for route in routes}) == 2

        result_nodes = subprocess.run(
            [*command, "--json", "nodes"], capture_output=True, text=True, check=True, timeout=5.0
        )
        node_items = json.loads(result_nodes.stdout)
        assert len(node_items) == 3
        assert len({node["connection_id"] for node in node_items}) == 3
        assert len({node["node_name"] for node in node_items}) == (1 if duplicate_names else 3)
        assert any(node["connection_id"] != node["node_id"] for node in node_items)
        endpoints = {
            node["node_id"]: f'{node["connection_id"]}:{node["node_name"]}' for node in node_items
        }

        result = subprocess.run(
            [*command, "routing"], capture_output=True, text=True, check=True, timeout=5.0
        )
        lines = result.stdout.splitlines()
        assert lines[0].split() == ["PORT_NAME", "PROVIDER_NODE", "REQUIRER_NODE"]
        rows = [line.split() for line in lines[2:]]
        assert len(rows) == 2
        assert {tuple(row) for row in rows} == {
            (port_name, endpoints[route["provide_node_id"]], endpoints[route["require_node_id"]])
            for route in routes
        }
        for line, row in zip(lines[2:], rows):
            assert line.index(row[1]) == lines[0].index("PROVIDER_NODE")
            assert line.rindex(row[2]) == lines[0].index("REQUIRER_NODE")

        result_unconnected = subprocess.run(
            [*command, "routing", "--unconnected"], capture_output=True, text=True, check=True, timeout=5.0
        )
        result_unconnected_json = subprocess.run(
            [*command, "--json", "routing", "--unconnected"],
            capture_output=True, text=True, check=True, timeout=5.0,
        )
        unconnected = json.loads(result_unconnected_json.stdout)
        assert len(unconnected) == 2
        assert {port["port_name"] for port in unconnected} == {"UnconsumedValue", "MissingValue"}
        for port in unconnected:
            assert set(port) == {"node_id", "port_id", "direction", "reason", "port_name"}
            assert isinstance(port["node_id"], int)
            assert isinstance(port["port_id"], int)
        assert {tuple(line.split()) for line in result_unconnected.stdout.splitlines()[2:]} == {
            (endpoints[port["node_id"]], port["direction"], port["reason"], port["port_name"])
            for port in unconnected
        }
    finally:
        for node in reversed(nodes):
            node.stop()


@pytest.mark.parametrize("json_output", [False, True], ids=["text", "json"])
def test_apxmon_watch(
    apx_server_with_monitor, apxmon_bin: str, apx_node_bin: str,
    apx_control: ApxControl, tmp_path: Path, json_output: bool,
):
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
        watch_cmd = [
            apxmon_bin, "-c", sock, *(["--json"] if json_output else []),
            "watch", "apx_sender:VehicleSpeed",
        ]
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

            assert watch_proc.stdout is not None
            deadline = time.monotonic() + 3.0
            buffer = b""
            received_signal = False
            while not received_signal:
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    break
                readable, _, _ = select.select([watch_proc.stdout], [], [], remaining)
                if not readable:
                    break
                chunk = os.read(watch_proc.stdout.fileno(), 4096)
                if not chunk:
                    break
                buffer += chunk
                while b"\n" in buffer:
                    raw_line, buffer = buffer.split(b"\n", 1)
                    line = raw_line.decode("utf-8")
                    max_timestamp_ns = (
                        time.monotonic_ns() - apx_server_with_monitor.startup_monotonic_time_ns[0]
                    )
                    if json_output:
                        entry = json.loads(line)
                        if entry.get("type") != "PORT_VALUE":
                            continue
                        assert isinstance(entry["timestamp_ns"], int)
                        assert 0 <= entry["timestamp_ns"] <= max_timestamp_ns
                        received_signal = entry["value_hex"] == "7B00"
                    else:
                        if not line.startswith("["):
                            continue
                        sample = re.fullmatch(
                            r"\[\+(\d+)\.(\d{3}) s\] Node \d+, Port \d+: \(len=2\) 0x([0-9A-F]{4})",
                            line,
                        )
                        assert sample is not None, line
                        seconds, milliseconds, value_hex = sample.groups()
                        timestamp_ms = int(seconds) * 1000 + int(milliseconds)
                        assert 0 <= timestamp_ms <= max_timestamp_ns // 1_000_000
                        received_signal = value_hex == "7B00"
                    if received_signal:
                        break

            assert received_signal, "apxmon watch did not receive expected PORT_VALUE notification"
        finally:
            watch_proc.terminate()
            try:
                _, watch_error = watch_proc.communicate(timeout=3.0)
            except subprocess.TimeoutExpired:
                watch_proc.kill()
                _, watch_error = watch_proc.communicate(timeout=3.0)
            assert watch_proc.returncode == 0, watch_error
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
