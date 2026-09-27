"""
Integration tests for APX Server TLS extension and mutual TLS (mTLS) client verification.
"""

import json
import os
import select
import socket
import ssl
import subprocess
import time
from pathlib import Path
import pytest
from conftest import _set_pdeathsig


def find_free_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]


def run_tls_server(tmp_path: Path, apx_server_bin: str, config_dict: dict):
    config_file = str(tmp_path / "server.json")
    with open(config_file, "w", encoding="utf-8") as f:
        json.dump(config_dict, f)

    r_fd, w_fd = os.pipe()
    os.set_inheritable(w_fd, True)

    cmd = [apx_server_bin, "--ready-fd", str(w_fd), config_file]
    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        pass_fds=(w_fd,),
        preexec_fn=_set_pdeathsig
    )
    os.close(w_fd)

    rlist, _, _ = select.select([r_fd], [], [], 3.0)
    if not rlist:
        os.close(r_fd)
        proc.kill()
        out, err = proc.communicate()
        raise TimeoutError(f"Server failed to start in time. stderr: {err.decode('utf-8', errors='replace')}")
    os.read(r_fd, 1)
    os.close(r_fd)

    return proc


def test_tls_server_connect(tmp_path: Path, apx_server_bin: str):
    """Test connecting to APX server using TLS with server authentication."""
    repo_root = Path(__file__).resolve().parent.parent
    certs_dir = repo_root / "tests" / "certs"
    ca_cert = str(certs_dir / "ca_cert.pem")
    server_cert = str(certs_dir / "server_cert.pem")
    server_key = str(certs_dir / "server_key.pem")

    port = find_free_port()
    config = {
        "tls-server-extension": {
            "tcp-port": port,
            "server-cert": server_cert,
            "server-key": server_key,
            "ca-cert": ca_cert,
            "require-client-cert": False,
            "tag": "TLS"
        }
    }

    proc = run_tls_server(tmp_path, apx_server_bin, config)
    try:
        ctx = ssl.create_default_context(ssl.Purpose.SERVER_AUTH, cafile=ca_cert)
        with socket.create_connection(("127.0.0.1", port), timeout=3.0) as sock:
            with ctx.wrap_socket(sock, server_hostname="localhost") as ssock:
                cert = ssock.getpeercert()
                assert cert is not None
                subject = dict(x[0] for x in cert["subject"])
                assert subject.get("commonName") == "localhost"
    finally:
        proc.terminate()
        proc.wait(timeout=3.0)


def test_tls_server_mtls_success(tmp_path: Path, apx_server_bin: str):
    """Test mutual TLS (mTLS) where server requires client cert and client provides valid cert."""
    repo_root = Path(__file__).resolve().parent.parent
    certs_dir = repo_root / "tests" / "certs"
    ca_cert = str(certs_dir / "ca_cert.pem")
    server_cert = str(certs_dir / "server_cert.pem")
    server_key = str(certs_dir / "server_key.pem")
    client_cert = str(certs_dir / "client_cert.pem")
    client_key = str(certs_dir / "client_key.pem")

    port = find_free_port()
    config = {
        "tls-server-extension": {
            "tcp-port": port,
            "server-cert": server_cert,
            "server-key": server_key,
            "ca-cert": ca_cert,
            "require-client-cert": True,
            "tag": "mTLS"
        }
    }

    proc = run_tls_server(tmp_path, apx_server_bin, config)
    try:
        ctx = ssl.create_default_context(ssl.Purpose.SERVER_AUTH, cafile=ca_cert)
        ctx.load_cert_chain(certfile=client_cert, keyfile=client_key)
        with socket.create_connection(("127.0.0.1", port), timeout=3.0) as sock:
            with ctx.wrap_socket(sock, server_hostname="localhost") as ssock:
                cert = ssock.getpeercert()
                assert cert is not None
                subject = dict(x[0] for x in cert["subject"])
                assert subject.get("commonName") == "localhost"
    finally:
        proc.terminate()
        proc.wait(timeout=3.0)


def test_tls_server_mtls_rejected_without_cert(tmp_path: Path, apx_server_bin: str):
    """Test that mutual TLS server rejects client connection when client does not present a cert."""
    repo_root = Path(__file__).resolve().parent.parent
    certs_dir = repo_root / "tests" / "certs"
    ca_cert = str(certs_dir / "ca_cert.pem")
    server_cert = str(certs_dir / "server_cert.pem")
    server_key = str(certs_dir / "server_key.pem")

    port = find_free_port()
    config = {
        "tls-server-extension": {
            "tcp-port": port,
            "server-cert": server_cert,
            "server-key": server_key,
            "ca-cert": ca_cert,
            "require-client-cert": True,
            "tag": "mTLS-Reject"
        }
    }

    proc = run_tls_server(tmp_path, apx_server_bin, config)
    try:
        ctx = ssl.create_default_context(ssl.Purpose.SERVER_AUTH, cafile=ca_cert)
        # Client does NOT load cert chain
        with pytest.raises(ssl.SSLError):
            with socket.create_connection(("127.0.0.1", port), timeout=3.0) as sock:
                with ctx.wrap_socket(sock, server_hostname="localhost") as ssock:
                    pass
    finally:
        proc.terminate()
        proc.wait(timeout=3.0)


def test_tls_server_rmfp_greeting_exchange(tmp_path: Path, apx_server_bin: str):
    """Test full RemoteFile protocol greeting exchange over TLS transport."""
    repo_root = Path(__file__).resolve().parent.parent
    certs_dir = repo_root / "tests" / "certs"
    ca_cert = str(certs_dir / "ca_cert.pem")
    server_cert = str(certs_dir / "server_cert.pem")
    server_key = str(certs_dir / "server_key.pem")

    port = find_free_port()
    config = {
        "tls-server-extension": {
            "tcp-port": port,
            "server-cert": server_cert,
            "server-key": server_key,
            "ca-cert": ca_cert,
            "require-client-cert": False,
            "tag": "TLS-Greeting"
        }
    }

    proc = run_tls_server(tmp_path, apx_server_bin, config)
    try:
        ctx = ssl.create_default_context(ssl.Purpose.SERVER_AUTH, cafile=ca_cert)
        with socket.create_connection(("127.0.0.1", port), timeout=3.0) as sock:
            with ctx.wrap_socket(sock, server_hostname="localhost") as ssock:
                greeting = b"RMFP/1.1\nMessage-Size: 32\n\n"
                ssock.sendall(bytes([len(greeting)]) + greeting)

                # Read server greeting response
                first_byte = ssock.recv(1)
                assert len(first_byte) == 1
                c = first_byte[0]
                if c & 0x80:
                    rem_header = ssock.recv(3)
                    msg_len = int.from_bytes(first_byte + rem_header, "big") & 0x7FFFFFFF
                else:
                    msg_len = c

                resp = b""
                while len(resp) < msg_len:
                    chunk = ssock.recv(msg_len - len(resp))
                    assert chunk
                    resp += chunk

                # Server replies with RMF_CMD_ACCEPT_HEADER (opcode 20 = 0x14)
                assert resp[:8] == b"\xbf\xff\xfc\x00\x14\x00\x00\x00"
    finally:
        proc.terminate()
        proc.wait(timeout=3.0)


def test_apx_node_connect_tls_server(tmp_path: Path, apx_server_bin: str, apx_node_bin: str):
    """Test apx-node connecting to APX server using --tls and --ca-cert."""
    repo_root = Path(__file__).resolve().parent.parent
    certs_dir = repo_root / "tests" / "certs"
    ca_cert = str(certs_dir / "ca_cert.pem")
    server_cert = str(certs_dir / "server_cert.pem")
    server_key = str(certs_dir / "server_key.pem")
    node_file = repo_root / "example" / "nodes" / "small_unsigned_sender.apx"

    port = find_free_port()
    config = {
        "tls-server-extension": {
            "tcp-port": port,
            "server-cert": server_cert,
            "server-key": server_key,
            "ca-cert": ca_cert,
            "require-client-cert": False,
            "tag": "TLS"
        }
    }

    server_proc = run_tls_server(tmp_path, apx_server_bin, config)
    try:
        cmd = [
            apx_node_bin,
            "--tls",
            "--ca-cert", ca_cert,
            "-c", "127.0.0.1",
            "-r", str(port),
            "--no-bind",
            str(node_file)
        ]
        node_proc = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            preexec_fn=_set_pdeathsig
        )
        time.sleep(0.8)
        node_proc.terminate()
        stdout, stderr = node_proc.communicate(timeout=2.0)
        assert "connected to APX server" in stdout, f"Node output: {stdout}, stderr: {stderr}"
    finally:
        server_proc.terminate()
        server_proc.wait(timeout=3.0)


def test_apx_node_connect_mtls_server(tmp_path: Path, apx_server_bin: str, apx_node_bin: str):
    """Test apx-node connecting to APX server with Mutual TLS (mTLS) client certificate."""
    repo_root = Path(__file__).resolve().parent.parent
    certs_dir = repo_root / "tests" / "certs"
    ca_cert = str(certs_dir / "ca_cert.pem")
    server_cert = str(certs_dir / "server_cert.pem")
    server_key = str(certs_dir / "server_key.pem")
    client_cert = str(certs_dir / "client_cert.pem")
    client_key = str(certs_dir / "client_key.pem")
    node_file = repo_root / "example" / "nodes" / "small_unsigned_sender.apx"

    port = find_free_port()
    config = {
        "tls-server-extension": {
            "tcp-port": port,
            "server-cert": server_cert,
            "server-key": server_key,
            "ca-cert": ca_cert,
            "require-client-cert": True,
            "tag": "TLS"
        }
    }

    server_proc = run_tls_server(tmp_path, apx_server_bin, config)
    try:
        cmd = [
            apx_node_bin,
            "--tls",
            "--ca-cert", ca_cert,
            "--client-cert", client_cert,
            "--client-key", client_key,
            "-c", "127.0.0.1",
            "-r", str(port),
            "--no-bind",
            str(node_file)
        ]
        node_proc = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            preexec_fn=_set_pdeathsig
        )
        time.sleep(0.8)
        node_proc.terminate()
        stdout, stderr = node_proc.communicate(timeout=2.0)
        assert "connected to APX server" in stdout, f"Node output: {stdout}, stderr: {stderr}"
    finally:
        server_proc.terminate()
        server_proc.wait(timeout=3.0)
