"""
Integration tests verifying NDJSON streaming support in apx-node's JSON message server and apx-control.
Verifies Roadmap Item 10: Exclusive Newline-Delimited JSON (NDJSON) framing.
"""

import socket
import time
from pathlib import Path
import pytest
from conftest import ApxServerInstance, ApxControl


def test_apx_node_ndjson_single_line(apx_server: ApxServerInstance, spawn_apx_node):
    """Verifies that sending a single newline-terminated JSON object via raw socket updates port values."""
    repo_root = Path(__file__).resolve().parent.parent
    listener_apx = repo_root / "example" / "nodes" / "unsigned_listener.apx"
    sender_apx = repo_root / "example" / "nodes" / "unsigned_sender.apx"

    listener = spawn_apx_node(listener_apx, bind=False)
    assert listener.wait_for_output("[APX-CONNECTION] connected to APX server", timeout=3.0)

    sender = spawn_apx_node(sender_apx, bind=True)
    assert sender.bind_path is not None
    assert sender.wait_for_output("[APX-CONNECTION] connected to APX server", timeout=3.0)

    # Connect raw UNIX socket to sender's JSON server and send raw NDJSON
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
        client.connect(sender.bind_path)
        client.sendall(b'{"VehicleSpeed": 75}\n')

    assert listener.wait_for_output('"VehicleSpeed": 75', timeout=3.0), (
        f"Listener did not output '\"VehicleSpeed\": 75'.\nOutput:\n{listener.output}"
    )


def test_apx_node_ndjson_stream_multiple_lines(apx_server: ApxServerInstance, spawn_apx_node):
    """Verifies that sending multiple NDJSON lines sequentially over a single socket connection updates ports."""
    repo_root = Path(__file__).resolve().parent.parent
    listener_apx = repo_root / "example" / "nodes" / "unsigned_listener.apx"
    sender_apx = repo_root / "example" / "nodes" / "unsigned_sender.apx"

    listener = spawn_apx_node(listener_apx, bind=False)
    assert listener.wait_for_output("[APX-CONNECTION] connected to APX server", timeout=3.0)

    sender = spawn_apx_node(sender_apx, bind=True)
    assert sender.bind_path is not None
    assert sender.wait_for_output("[APX-CONNECTION] connected to APX server", timeout=3.0)

    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
        client.connect(sender.bind_path)
        client.sendall(b'{"VehicleSpeed": 40}\n{"VehicleSpeed": 50}\n{"VehicleSpeed": 60}\n')

    assert listener.wait_for_output('"VehicleSpeed": 60', timeout=3.0), (
        f"Listener did not output '\"VehicleSpeed\": 60'.\nOutput:\n{listener.output}"
    )


def test_apx_node_ndjson_crlf(apx_server: ApxServerInstance, spawn_apx_node):
    """Verifies that NDJSON lines terminated with CRLF (\\r\\n) are properly parsed."""
    repo_root = Path(__file__).resolve().parent.parent
    listener_apx = repo_root / "example" / "nodes" / "unsigned_listener.apx"
    sender_apx = repo_root / "example" / "nodes" / "unsigned_sender.apx"

    listener = spawn_apx_node(listener_apx, bind=False)
    assert listener.wait_for_output("[APX-CONNECTION] connected to APX server", timeout=3.0)

    sender = spawn_apx_node(sender_apx, bind=True)
    assert sender.bind_path is not None
    assert sender.wait_for_output("[APX-CONNECTION] connected to APX server", timeout=3.0)

    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
        client.connect(sender.bind_path)
        client.sendall(b'{"VehicleSpeed": 85}\r\n')

    assert listener.wait_for_output('"VehicleSpeed": 85', timeout=3.0), (
        f"Listener did not output '\"VehicleSpeed\": 85'.\nOutput:\n{listener.output}"
    )


def test_apx_node_ndjson_leading_blank_lines(apx_server: ApxServerInstance, spawn_apx_node):
    """Verifies that leading whitespace and empty lines before valid JSON are skipped."""
    repo_root = Path(__file__).resolve().parent.parent
    listener_apx = repo_root / "example" / "nodes" / "unsigned_listener.apx"
    sender_apx = repo_root / "example" / "nodes" / "unsigned_sender.apx"

    listener = spawn_apx_node(listener_apx, bind=False)
    assert listener.wait_for_output("[APX-CONNECTION] connected to APX server", timeout=3.0)

    sender = spawn_apx_node(sender_apx, bind=True)
    assert sender.bind_path is not None
    assert sender.wait_for_output("[APX-CONNECTION] connected to APX server", timeout=3.0)

    payload = b'\r\n\n   {"VehicleSpeed": 99}\r\n'
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
        client.connect(sender.bind_path)
        client.sendall(payload)

    assert listener.wait_for_output('"VehicleSpeed": 99', timeout=3.0), (
        f"Listener did not output '\"VehicleSpeed\": 99'.\nOutput:\n{listener.output}"
    )


def test_apx_node_ndjson_with_apx_control(
    apx_server: ApxServerInstance, spawn_apx_node, apx_control: ApxControl
):
    """Verifies that apx-control sending NDJSON updates works seamlessly with apx-node."""
    repo_root = Path(__file__).resolve().parent.parent
    listener_apx = repo_root / "example" / "nodes" / "unsigned_listener.apx"
    sender_apx = repo_root / "example" / "nodes" / "unsigned_sender.apx"

    listener = spawn_apx_node(listener_apx, bind=False)
    assert listener.wait_for_output("[APX-CONNECTION] connected to APX server", timeout=3.0)

    sender = spawn_apx_node(sender_apx, bind=True)
    assert sender.bind_path is not None
    assert sender.wait_for_output("[APX-CONNECTION] connected to APX server", timeout=3.0)

    # 1. Send via apx-control (NDJSON)
    res1 = apx_control.set_signal("VehicleSpeed", 101, connect_path=sender.bind_path)
    assert res1.returncode == 0
    assert listener.wait_for_output('"VehicleSpeed": 101', timeout=3.0)

    # 2. Send via raw NDJSON on a separate raw socket connection
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
        client.connect(sender.bind_path)
        client.sendall(b'{"VehicleSpeed": 102}\n')
    assert listener.wait_for_output('"VehicleSpeed": 102', timeout=3.0)

    # 3. Send via apx-control again
    res2 = apx_control.set_signal("VehicleSpeed", 103, connect_path=sender.bind_path)
    assert res2.returncode == 0
    assert listener.wait_for_output('"VehicleSpeed": 103', timeout=3.0)
