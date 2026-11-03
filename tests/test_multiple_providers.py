"""
Integration tests for Multiple Providers per Require Port ("multiple-provider-policy").
"""

import json
import os
import select
import shutil
import subprocess
import time
from pathlib import Path
import pytest
from conftest import _set_pdeathsig, wait_for_unix_socket, ApxServerInstance, ApxNodeInstance


def _start_server_with_policy(tmp_path: Path, apx_server_bin: str, policy: str):
    server_socket = str(tmp_path / "apx.socket")
    monitor_socket = str(tmp_path / "monitor.socket")
    log_file = str(tmp_path / "server.log")
    config_file = str(tmp_path / "server.json")

    config = {
        "apx-server": {
            "multiple-provider-policy": policy
        },
        "socket-server-extension": {
            "unix-file": server_socket
        },
        "monitor-extension": {
            "enabled": True,
            "socket-file": monitor_socket
        },
        "textlog-extension": {
            "enabled": True,
            "log-level": "INFO",
            "file-enabled": True,
            "file-path": log_file
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
    server.log_file = Path(log_file)
    return server


def _spawn_node(apx_node_bin: str, server_socket: str, def_file: Path) -> ApxNodeInstance:
    cmd = [apx_node_bin, "-c", server_socket, "--no-bind", str(def_file)]
    stdbuf_bin = shutil.which("stdbuf")
    if stdbuf_bin:
        cmd = [stdbuf_bin, "-oL", *cmd]
    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1,
        preexec_fn=_set_pdeathsig,
    )
    return ApxNodeInstance(proc, def_file)


def test_multiple_providers_policy_first_and_apx_mon(tmp_path: Path, apx_server_bin: str, apx_node_bin: str, apxmon_bin: str):
    """Verify 'first' provider arbitration policy, failover on disconnect, and apx-mon diagnostics."""
    server = _start_server_with_policy(tmp_path, apx_server_bin, "first")
    nodes: list[ApxNodeInstance] = []
    try:
        prov_a_file = tmp_path / "ProviderA.apx"
        prov_a_file.write_text('APX/1.3\nN"ProviderA"\nP"Signal"S:=10\n', encoding="utf-8")

        prov_b_file = tmp_path / "ProviderB.apx"
        prov_b_file.write_text('APX/1.3\nN"ProviderB"\nP"Signal"S:=20\n', encoding="utf-8")

        cons_file = tmp_path / "Consumer.apx"
        cons_file.write_text('APX/1.3\nN"Consumer"\nR"Signal"S:=0\n', encoding="utf-8")

        node_a = _spawn_node(apx_node_bin, server.socket_path, prov_a_file)
        nodes.append(node_a)

        node_c = _spawn_node(apx_node_bin, server.socket_path, cons_file)
        nodes.append(node_c)
        assert node_c.wait_for_output('"Signal": 10', timeout=3.0)

        node_b = _spawn_node(apx_node_bin, server.socket_path, prov_b_file)
        nodes.append(node_b)

        # Wait until apx-mon sees 2 providers for Signal
        deadline = time.monotonic() + 3.0
        mp_data = []
        while time.monotonic() < deadline:
            res = subprocess.run(
                [apxmon_bin, "-c", server.monitor_socket, "--json", "routing", "--multi-provider"],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                check=True,
            )
            mp_data = json.loads(res.stdout)
            if len(mp_data) == 2:
                break
            time.sleep(0.05)

        assert len(mp_data) == 2
        assert mp_data[0]["port_name"] == "Signal"
        assert mp_data[0]["status"] == "ACTIVE"
        assert mp_data[1]["port_name"] == "Signal"
        assert mp_data[1]["status"] == "SHADOWED"

        # Verify tabular output of routing --multi-provider
        res_tab = subprocess.run(
            [apxmon_bin, "-c", server.monitor_socket, "routing", "--multi-provider"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=True,
        )
        assert "ProviderA" in res_tab.stdout
        assert "ProviderB" in res_tab.stdout
        assert "ACTIVE" in res_tab.stdout
        assert "(shadowed)" in res_tab.stdout

        # Verify routing --unconnected reports ProviderB as SHADOWED
        res_unconn_json = subprocess.run(
            [apxmon_bin, "-c", server.monitor_socket, "--json", "routing", "--unconnected"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=True,
        )
        unconn_data = json.loads(res_unconn_json.stdout)
        shadowed_ports = [p for p in unconn_data if p["port_name"] == "Signal" and p["reason"] == "SHADOWED"]
        assert len(shadowed_ports) == 1
        assert shadowed_ports[0]["node_id"] == mp_data[1]["node_id"]

        res_unconn_tab = subprocess.run(
            [apxmon_bin, "-c", server.monitor_socket, "routing", "--unconnected"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=True,
        )
        assert "ProviderB" in res_unconn_tab.stdout
        assert "(shadowed)" in res_unconn_tab.stdout

        # Stop ProviderA -> ProviderB becomes active and Consumer receives 20
        node_a.stop()
        assert node_c.wait_for_output('"Signal": 20', timeout=3.0)

    finally:
        for n in nodes:
            n.stop()
        server.stop()

    log_content = server.log_file.read_text(encoding="utf-8")
    assert "Provider changed:" in log_content
    assert "ProviderA/Signal ->" in log_content
    assert "ProviderB/Signal" in log_content


def test_multiple_providers_policy_last(tmp_path: Path, apx_server_bin: str, apx_node_bin: str, apxmon_bin: str):
    """Verify 'last' provider arbitration policy takeover on connect and fallback on disconnect."""
    server = _start_server_with_policy(tmp_path, apx_server_bin, "last")
    nodes: list[ApxNodeInstance] = []
    try:
        prov_a_file = tmp_path / "ProviderA.apx"
        prov_a_file.write_text('APX/1.3\nN"ProviderA"\nP"Signal"S:=10\n', encoding="utf-8")

        prov_b_file = tmp_path / "ProviderB.apx"
        prov_b_file.write_text('APX/1.3\nN"ProviderB"\nP"Signal"S:=20\n', encoding="utf-8")

        cons_file = tmp_path / "Consumer.apx"
        cons_file.write_text('APX/1.3\nN"Consumer"\nR"Signal"S:=0\n', encoding="utf-8")

        node_a = _spawn_node(apx_node_bin, server.socket_path, prov_a_file)
        nodes.append(node_a)

        node_c = _spawn_node(apx_node_bin, server.socket_path, cons_file)
        nodes.append(node_c)
        assert node_c.wait_for_output('"Signal": 10', timeout=3.0)

        # Spawn ProviderB -> under 'last' policy, ProviderB takes over immediately!
        node_b = _spawn_node(apx_node_bin, server.socket_path, prov_b_file)
        nodes.append(node_b)
        assert node_c.wait_for_output('"Signal": 20', timeout=3.0)

        res = subprocess.run(
            [apxmon_bin, "-c", server.monitor_socket, "--json", "routing", "--multi-provider"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=True,
        )
        mp_data = json.loads(res.stdout)
        assert len(mp_data) == 2
        assert mp_data[0]["status"] == "SHADOWED"
        assert mp_data[1]["status"] == "ACTIVE"

        # Stop ProviderB -> ProviderA resumes as active provider!
        node_b.stop()
        deadline = time.monotonic() + 3.0
        while time.monotonic() < deadline:
            if node_c.output.count('"Signal": 10') >= 2:
                break
            time.sleep(0.05)
        assert node_c.output.count('"Signal": 10') >= 2

    finally:
        for n in nodes:
            n.stop()
        server.stop()

    log_content = server.log_file.read_text(encoding="utf-8")
    assert "ProviderA/Signal ->" in log_content
    assert "ProviderB/Signal ->" in log_content

