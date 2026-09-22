"""
End-to-end integration test automating the Getting Started guide:
https://github.com/cogu/apx/blob/master/guides/index.md

Verifies the full pipeline:
apx_control -> apx_node (JSON router) -> apx_server -> apx_node (listener) -> stdout
"""

from pathlib import Path
from conftest import ApxServerInstance, ApxControl


def test_getting_started_guide_flow(
    apx_server: ApxServerInstance,
    spawn_apx_node,
    apx_control: ApxControl,
):
    """
    Automates the scenario described in the official APX Getting Started guide:
    1. Start APX server.
    2. Start unsigned_listener node with --no-bind.
    3. Start unsigned_sender node with JSON router socket bound in tmp_path.
    4. apx_control sends 'VehicleSpeed 100' to the sender's JSON socket.
    5. unsigned_listener prints '"VehicleSpeed": 100' on stdout.
    """
    repo_root = Path(__file__).resolve().parent.parent
    listener_apx = repo_root / "example" / "nodes" / "unsigned_listener.apx"
    sender_apx = repo_root / "example" / "nodes" / "unsigned_sender.apx"

    assert listener_apx.is_file(), f"Missing APX definition file: {listener_apx}"
    assert sender_apx.is_file(), f"Missing APX definition file: {sender_apx}"

    # 1. Start pure listener node (no-bind)
    listener = spawn_apx_node(listener_apx, bind=False)
    assert listener.wait_for_output("[APX-CONNECTION] connected to APX server", timeout=3.0), (
        f"Listener failed to connect to APX server.\nOutput:\n{listener.output}"
    )

    # 2. Start sender node acting as JSON-to-APX router (bind=True auto-allocates an isolated socket)
    sender = spawn_apx_node(sender_apx, bind=True)
    assert sender.bind_path is not None, "Sender should have an assigned bind socket path"
    assert sender.wait_for_output("[APX-CONNECTION] connected to APX server", timeout=3.0), (
        f"Sender failed to connect to APX server.\nOutput:\n{sender.output}"
    )

    # 3. Send "VehicleSpeed 100" via apx_control connected to sender's JSON socket
    result = apx_control.set_signal("VehicleSpeed", 100, connect_path=sender.bind_path)
    assert result.returncode == 0, f"apx_control failed:\nStdout: {result.stdout}\nStderr: {result.stderr}"

    # 4. Verify that listener node receives and prints '"VehicleSpeed": 100'
    assert listener.wait_for_output('"VehicleSpeed": 100', timeout=3.0), (
        f"Listener did not output '\"VehicleSpeed\": 100'.\nOutput:\n{listener.output}"
    )
