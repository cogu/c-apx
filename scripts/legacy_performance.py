#!/usr/bin/env python3
"""Compare reconnect latency of legacy apx_server (v0.2.8) vs v0.3.x apx-server."""

import argparse
import hashlib
import json
import os
import random
import select
import socket
import subprocess
import sys
import tempfile
import time
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO_ROOT / "tests"))

from conftest import _set_pdeathsig  # noqa: E402
from test_server_compressed_nodes import read_msg  # noqa: E402
from test_server_definition_events import client, opcode, publish, write  # noqa: E402

RMFP_1_0_GREETING = b"RMFP/1.0\nNumHeader-Format:32\n\n"
RMFP_1_1_GREETING = b"RMFP/1.1\nMessage-Size: 32\n\n"


def find_free_tcp_port() -> int:
    # apx-server's socket-server-extension enforces TCP user ports [1024, 49151].
    for _ in range(100):
        port = random.randint(20000, 49000)
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            try:
                s.bind(("127.0.0.1", port))
                return port
            except OSError:
                continue
    raise RuntimeError("Could not find a free TCP port in [20000, 49000]")


class TcpServerHandle:
    def __init__(self, proc: subprocess.Popen, port: int):
        self.process = proc
        self.port = port

    def connect(self) -> socket.socket:
        sock = socket.create_connection(("127.0.0.1", self.port), timeout=3.0)
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        return sock

    def stop(self) -> None:
        if self.process.poll() is None:
            self.process.terminate()
            try:
                self.process.wait(timeout=3.0)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait(timeout=3.0)
        if self.process.stdout is not None:
            self.process.stdout.close()
        if self.process.stderr is not None:
            self.process.stderr.close()


def start_legacy_server(server_bin: str, port: int) -> TcpServerHandle:
    proc = subprocess.Popen(
        [server_bin, f"-p{port}"],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        preexec_fn=_set_pdeathsig,
    )
    # Wait briefly without sending a raw probe connection (which v0.2.8 treats as a client)
    time.sleep(0.25)
    if proc.poll() is not None:
        raise RuntimeError(f"Legacy apx_server exited prematurely with code {proc.returncode}")
    return TcpServerHandle(proc, port)


def start_modern_tcp_server(tmp_path: Path, server_bin: str, port: int, cache_level: int) -> TcpServerHandle:
    config_file = tmp_path / "server.json"
    config = {
        "apx-server": {"cache": {"level": cache_level, "path": str(tmp_path / "cache")}},
        "socket-server-extension": {"tcp-port": port},
    }
    config_file.write_text(json.dumps(config, indent=2), encoding="utf-8")

    r_fd, w_fd = os.pipe()
    os.set_inheritable(w_fd, True)
    proc = subprocess.Popen(
        [server_bin, "--ready-fd", str(w_fd), str(config_file)],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
        pass_fds=(w_fd,),
        preexec_fn=_set_pdeathsig,
    )
    os.close(w_fd)
    rlist, _, _ = select.select([r_fd], [], [], 3.0)
    if not rlist:
        os.close(r_fd)
        proc.kill()
        raise TimeoutError("Modern apx-server failed to signal readiness")
    os.read(r_fd, 1)
    os.close(r_fd)
    return TcpServerHandle(proc, port)


def build_apx12_definition(num_ports: int) -> bytes:
    lines = ["APX/1.2", 'N"BenchNode"', 'T"Rec_T"{"a"S"b"C"c"L}']
    for i in range(num_ports):
        if i % 3 == 0:
            lines.append(f'R"Port{i:04d}"T[0]:={{1,2,3}}')
        else:
            lines.append(f'R"Port{i:04d}"S:={i}')
    return ("\n".join(lines) + "\n").encode("ascii")


def send_chunked(sock: socket.socket, data: bytes, chunk_size: int = 4096) -> None:
    off = 0
    while off < len(data):
        chunk = data[off : off + chunk_size]
        write(sock, chunk, offset=off, more=(off + len(chunk)) < len(data))
        off += len(chunk)


def bench_server(
    srv: TcpServerHandle,
    apx_def: bytes,
    digest: bytes,
    greeting: bytes,
    iters: int,
    cooldown_s: float = 0.005,
) -> float:
    # 1 warmup connection
    with client(srv, greeting=greeting) as sock:
        payload, resp = publish(sock, apx_def, name="BenchNode", compressed=False, digest=digest)
        if opcode(resp) == 10:
            send_chunked(sock, payload)
            resp = read_msg(sock)
        if opcode(resp) != 3:
            raise RuntimeError(f"Warmup failed: expected opcode 3, got {opcode(resp)}")
    if cooldown_s > 0.0:
        time.sleep(cooldown_s)

    # Time each reconnect individually; untimed cooldown lets server worker threads
    # finish detaching the previous BenchNode instance (required for v0.2.8).
    elapsed = 0.0
    for _ in range(iters):
        t0 = time.perf_counter()
        with client(srv, greeting=greeting) as sock:
            payload, resp = publish(sock, apx_def, name="BenchNode", compressed=False, digest=digest)
            if opcode(resp) == 10:
                send_chunked(sock, payload)
                resp = read_msg(sock)
            code = opcode(resp)
            if code != 3:
                raise RuntimeError(f"Got opcode {code}, expected 3")
        elapsed += time.perf_counter() - t0
        if cooldown_s > 0.0:
            time.sleep(cooldown_s)
    return elapsed * 1000.0 / iters


def main() -> int:
    default_legacy = Path.home() / "bin" / "apx_server"
    default_modern = REPO_ROOT / "build" / "clang-release" / "app" / "apx_server" / "apx-server"

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--legacy-server", type=Path, default=default_legacy, help="Path to v0.2.8 apx_server")
    parser.add_argument("--modern-server", type=Path, default=default_modern, help="Path to v0.3.x apx-server")
    parser.add_argument("--ports", type=int, default=1000, help="Number of require ports in APX/1.2 benchmark node")
    parser.add_argument("--iters", type=int, default=50, help="Number of timed reconnect iterations")
    parser.add_argument("--legacy-only", action="store_true", help="Benchmark only the legacy v0.2.8 server")
    args = parser.parse_args()

    legacy_bin = str(args.legacy_server.expanduser().resolve())
    if not Path(legacy_bin).is_file():
        print(f"Error: legacy apx_server not found at {legacy_bin}", file=sys.stderr)
        return 1

    apx_def = build_apx12_definition(args.ports)
    digest = hashlib.sha256(apx_def).digest()
    print(f"Benchmarking APX/1.2 node ({args.ports} ports, {len(apx_def)} bytes, {args.iters} reconnects over TCP):")

    legacy_port = find_free_tcp_port()
    legacy_srv = start_legacy_server(legacy_bin, legacy_port)
    try:
        legacy_ms = bench_server(legacy_srv, apx_def, digest, RMFP_1_0_GREETING, args.iters)
        print(f"  v0.2.8 legacy:         {legacy_ms:.2f} ms / reconnect")
    finally:
        legacy_srv.stop()

    if args.legacy_only:
        return 0

    modern_bin = str(args.modern_server.expanduser().resolve())
    if not Path(modern_bin).is_file():
        print(f"Skipping v0.3.x comparison (binary not found at {modern_bin})", file=sys.stderr)
        return 0

    for level in (0, 1, 2):
        with tempfile.TemporaryDirectory() as td:
            port = find_free_tcp_port()
            srv = start_modern_tcp_server(Path(td), modern_bin, port, level)
            try:
                dt_ms = bench_server(srv, apx_def, digest, RMFP_1_1_GREETING, args.iters)
                ratio = legacy_ms / dt_ms if dt_ms > 0 else 0.0
                print(f"  v0.3.x cache level={level}:  {dt_ms:.2f} ms / reconnect  ({ratio:.2f}x vs v0.2.8)")
            finally:
                srv.stop()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
