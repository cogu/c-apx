#!/usr/bin/env python3
"""Benchmark full APX network startup and routing using real .apx node files."""

import argparse
import hashlib
import json
import os
import random
import select
import socket
import struct
import subprocess
import sys
import tempfile
import threading
import time
from dataclasses import dataclass
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO_ROOT / "tests"))

from conftest import _set_pdeathsig  # noqa: E402
from test_server_compressed_nodes import encode_numheader32, read_msg, sign_data_with_openssl  # noqa: E402
from test_server_definition_events import opcode, write  # noqa: E402

RMFP_1_0_GREETING = b"RMFP/1.0\nNumHeader-Format:32\n\n"
RMFP_1_1_GREETING = b"RMFP/1.1\nMessage-Size: 32\n\n"
OUT_FILE_ADDR = 0x00000000
APX_FILE_ADDR = 0x02000000


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
    time.sleep(0.25)
    if proc.poll() is not None:
        raise RuntimeError(f"Legacy apx_server exited prematurely with code {proc.returncode}")
    return TcpServerHandle(proc, port)


def start_modern_tcp_server(
    tmp_path: Path,
    server_bin: str,
    port: int,
    cache_level: int,
    *,
    trusted_pubkey: Path | None = None,
) -> TcpServerHandle:
    config_file = tmp_path / "server.json"
    server_cfg: dict = {"cache": {"level": cache_level, "path": str(tmp_path / "cache")}}
    if trusted_pubkey is not None:
        server_cfg["security"] = {
            "require-signed-nodes": True,
            "trusted-keys": [str(trusted_pubkey)],
        }
    config = {
        "apx-server": server_cfg,
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


def send_chunked(sock: socket.socket, data: bytes, chunk_size: int = 4096) -> None:
    off = 0
    while off < len(data):
        chunk = data[off : off + chunk_size]
        write(sock, chunk, offset=off, more=(off + len(chunk)) < len(data))
        off += len(chunk)


def send_file_info_no_wait(
    sock: socket.socket,
    *,
    name: str,
    address: int,
    size: int,
    digest: bytes | None = None,
    signature: bytes | None = None,
) -> None:
    if signature is not None:
        assert digest is not None and len(digest) == 32
        assert len(signature) == 64
        header = struct.pack("<IIIHH", 21, address, size, 0, 1)
        command = header + digest + signature + f"{name}\0".encode("ascii")
    else:
        digest_type = 2 if digest is not None else 0
        digest_bytes = digest if digest is not None else (b"\x00" * 32)
        header = struct.pack("<IIIHH", 3, address, size, 0, digest_type)
        command = header + digest_bytes + f"{name}\0".encode("ascii")
    sock.sendall(encode_numheader32(struct.pack(">I", 0xBFFFFC00) + command))


def file_info_name(msg: bytes) -> str:
    if len(msg) <= 52:
        return ""
    return msg[52:].split(b"\0", 1)[0].decode("ascii", errors="replace")


def drain_socket(sock: socket.socket) -> None:
    """Drain any queued unsolicited server notifications without blocking."""
    while True:
        rlist, _, _ = select.select([sock], [], [], 0.0)
        if not rlist:
            break
        data = sock.recv(65536)
        if not data:
            break


@dataclass
class NodeSpec:
    path: Path
    name: str
    apx_bytes: bytes
    digest: bytes
    signature: bytes | None
    out_size: int
    num_provide: int
    num_require: int


def probe_apx_file(apx_node_bin: str, lsock: socket.socket, sock_path: str, apx_path: Path) -> NodeSpec:
    """Use apx-node against a local mock socket to obtain exact node name and .out size."""
    apx_bytes = apx_path.read_bytes()
    raw_text = apx_bytes.decode("utf-8", errors="replace")
    num_provide = sum(1 for ln in raw_text.splitlines() if ln.startswith('P"'))
    num_require = sum(1 for ln in raw_text.splitlines() if ln.startswith('R"'))

    proc = subprocess.Popen(
        [apx_node_bin, "--no-bind", "--no-signature", "-c", sock_path, str(apx_path)],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    out_size = 0
    node_name = None
    try:
        conn, _ = lsock.accept()
        with conn:
            _ = read_msg(conn, timeout=3.0)
            ack = struct.pack(">I", 0xBFFFFC00) + struct.pack("<I", 0)
            conn.sendall(encode_numheader32(ack))
            while True:
                msg = read_msg(conn, timeout=1.0)
                if not msg:
                    break
                if opcode(msg) == 3:
                    size = int.from_bytes(msg[12:16], "little")
                    fname = file_info_name(msg)
                    if fname.endswith(".out"):
                        out_size = size
                    elif fname.endswith(".apx"):
                        node_name = fname[:-4]
                        break
    finally:
        proc.terminate()
        try:
            proc.wait(timeout=2.0)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait(timeout=2.0)

    if node_name is None:
        raise RuntimeError(f"Failed to probe node metadata from {apx_path}")

    return NodeSpec(
        path=apx_path,
        name=node_name,
        apx_bytes=apx_bytes,
        digest=hashlib.sha256(apx_bytes).digest(),
        signature=None,
        out_size=out_size,
        num_provide=num_provide,
        num_require=num_require,
    )


def load_network(apx_node_bin: str, network_dir: Path) -> list[NodeSpec]:
    files = sorted(network_dir.glob("*.apx"))
    if not files:
        raise RuntimeError(f"No .apx files found in {network_dir}")
    nodes: list[NodeSpec] = []
    with tempfile.TemporaryDirectory() as td:
        sock_path = os.path.join(td, "probe.sock")
        lsock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        lsock.bind(sock_path)
        lsock.listen(4)
        lsock.settimeout(5.0)
        try:
            for p in files:
                nodes.append(probe_apx_file(apx_node_bin, lsock, sock_path, p))
        finally:
            lsock.close()
    return nodes


def connect_node(
    srv: TcpServerHandle,
    node: NodeSpec,
    greeting: bytes,
    *,
    signed: bool = False,
    timeout_s: float = 30.0,
) -> tuple[socket.socket, float]:
    """Connect one node, publish its .out/.apx, and wait until server publishes .in."""
    out_data = b"\x00" * node.out_size
    t0 = time.perf_counter()
    sock = srv.connect()
    try:
        sock.sendall(encode_numheader32(greeting))
        ack = read_msg(sock, timeout=timeout_s)
        if not ack or opcode(ack) not in (0, 20):
            raise RuntimeError(f"{node.name}: greeting failed (opcode={opcode(ack) if ack else None})")

        if node.out_size > 0:
            send_file_info_no_wait(sock, name=f"{node.name}.out", address=OUT_FILE_ADDR, size=node.out_size)
        send_file_info_no_wait(
            sock,
            name=f"{node.name}.apx",
            address=APX_FILE_ADDR,
            size=len(node.apx_bytes),
            digest=node.digest,
            signature=node.signature if signed else None,
        )

        sent_out = node.out_size == 0
        got_in = node.num_require == 0
        deadline = time.monotonic() + timeout_s
        while not (sent_out and got_in):
            remaining = max(0.1, deadline - time.monotonic())
            msg = read_msg(sock, timeout=remaining)
            if not msg:
                raise RuntimeError(f"{node.name}: connection closed unexpectedly")
            code = opcode(msg)
            if code == 10:  # RMF_CMD_OPEN_FILE
                addr = int.from_bytes(msg[8:12], "little")
                if addr == APX_FILE_ADDR:
                    send_chunked(sock, node.apx_bytes)
                elif addr == OUT_FILE_ADDR:
                    off = 0
                    while off < len(out_data):
                        chunk = out_data[off : off + 4096]
                        write(sock, chunk, address=OUT_FILE_ADDR, offset=off, more=(off + len(chunk)) < len(out_data))
                        off += len(chunk)
                    sent_out = True
            elif code == 3:  # RMF_CMD_FILE_INFO
                if file_info_name(msg) == f"{node.name}.in":
                    got_in = True
            elif code == 1:  # RMF_CMD_NACK
                err = int.from_bytes(msg[8:12], "little") if len(msg) >= 12 else -1
                raise RuntimeError(f"{node.name}: server returned NACK error={err}")

        elapsed_ms = (time.perf_counter() - t0) * 1000.0
        return sock, elapsed_ms
    except Exception:
        sock.close()
        raise


def bench_sequential_boot(
    srv: TcpServerHandle,
    nodes: list[NodeSpec],
    greeting: bytes,
    *,
    signed: bool = False,
) -> tuple[float, list[tuple[str, float]]]:
    """Connect all nodes one by one (keeping all open), returning total and per-node ms."""
    sockets: list[socket.socket] = []
    per_node: list[tuple[str, float]] = []
    try:
        t_start = time.perf_counter()
        for node in nodes:
            sock, dt_ms = connect_node(srv, node, greeting, signed=signed)
            sockets.append(sock)
            per_node.append((node.name, dt_ms))
            for s in sockets:
                drain_socket(s)
        total_ms = (time.perf_counter() - t_start) * 1000.0
        return total_ms, per_node
    finally:
        for s in sockets:
            s.close()


def bench_parallel_boot(
    srv: TcpServerHandle,
    nodes: list[NodeSpec],
    greeting: bytes,
    *,
    signed: bool = False,
) -> tuple[float, list[tuple[str, float]]]:
    """Connect all nodes simultaneously using a barrier, returning wall-clock total ms."""
    barrier = threading.Barrier(len(nodes))
    sockets: list[socket.socket | None] = [None] * len(nodes)
    per_node: list[tuple[str, float]] = [("", 0.0)] * len(nodes)
    errors: list[Exception] = []
    err_lock = threading.Lock()
    start_times: list[float] = []
    end_times: list[float] = []
    time_lock = threading.Lock()

    def worker(idx: int, node: NodeSpec) -> None:
        try:
            barrier.wait()
            t0 = time.perf_counter()
            with time_lock:
                start_times.append(t0)
            sock, dt_ms = connect_node(srv, node, greeting, signed=signed)
            t1 = time.perf_counter()
            sockets[idx] = sock
            per_node[idx] = (node.name, dt_ms)
            with time_lock:
                end_times.append(t1)
        except Exception as exc:
            with err_lock:
                errors.append(exc)

    threads = [threading.Thread(target=worker, args=(i, n)) for i, n in enumerate(nodes)]
    for t in threads:
        t.start()
    for t in threads:
        t.join()

    try:
        if errors:
            raise errors[0]
        wall_ms = (max(end_times) - min(start_times)) * 1000.0
        return wall_ms, per_node
    finally:
        for s in sockets:
            if s is not None:
                s.close()


def run_benchmark_target(
    label: str,
    make_server,
    greeting: bytes,
    nodes: list[NodeSpec],
    runs: int,
    *,
    signed: bool = False,
    warmup_cache: bool = False,
    show_per_node: bool = False,
) -> None:
    with tempfile.TemporaryDirectory() as td:
        tmp_path = Path(td)
        if warmup_cache:
            # Populate persistent disk cache once, then stop server so timed runs test fresh server boots.
            srv = make_server(tmp_path)
            try:
                bench_sequential_boot(srv, nodes, greeting, signed=signed)
            finally:
                srv.stop()

        seq_totals: list[float] = []
        par_totals: list[float] = []
        last_seq_nodes: list[tuple[str, float]] = []

        for _ in range(runs):
            srv = make_server(tmp_path)
            try:
                seq_ms, per_node = bench_sequential_boot(srv, nodes, greeting, signed=signed)
                seq_totals.append(seq_ms)
                last_seq_nodes = per_node
            finally:
                srv.stop()

            srv = make_server(tmp_path)
            try:
                par_ms, _ = bench_parallel_boot(srv, nodes, greeting, signed=signed)
                par_totals.append(par_ms)
            finally:
                srv.stop()

        avg_seq = sum(seq_totals) / len(seq_totals)
        avg_par = sum(par_totals) / len(par_totals)
        print(
            f"  {label:<32} "
            f"sequential: {avg_seq:6.2f} ms (min {min(seq_totals):5.2f}) | "
            f"parallel: {avg_par:6.2f} ms (min {min(par_totals):5.2f})"
        )
        if show_per_node:
            top = sorted(last_seq_nodes, key=lambda kv: kv[1], reverse=True)[:5]
            top_str = ", ".join(f"{name}={ms:.2f}ms" for name, ms in top)
            print(f"    slowest nodes (seq): {top_str}")


def main() -> int:
    default_legacy = Path.home() / "bin" / "apx_server"
    default_modern = REPO_ROOT / "build" / "clang-release" / "app" / "apx_server" / "apx-server"
    default_apx_node = REPO_ROOT / "build" / "clang-release" / "app" / "apx_node" / "apx-node"

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("networks", type=Path, nargs="+", help="One or more directories containing .apx files")
    parser.add_argument("--legacy-server", type=Path, default=default_legacy, help="Path to v0.2.9 apx_server")
    parser.add_argument("--modern-server", type=Path, default=default_modern, help="Path to v0.3.x apx-server")
    parser.add_argument("--apx-node", type=Path, default=default_apx_node, help="Path to apx-node (for probing .out sizes)")
    parser.add_argument("--runs", type=int, default=5, help="Number of full network boot runs per mode")
    parser.add_argument("--per-node", action="store_true", help="Print top 5 slowest nodes for each mode")
    parser.add_argument("--legacy-only", action="store_true", help="Benchmark only the legacy v0.2.9 server")
    parser.add_argument("--signed", action="store_true", help="Also benchmark ECDSA-signed node publications")
    args = parser.parse_args()

    apx_node_bin = str(args.apx_node.expanduser().resolve())
    if not Path(apx_node_bin).is_file():
        print(f"Error: apx-node not found at {apx_node_bin}", file=sys.stderr)
        return 1

    legacy_bin = str(args.legacy_server.expanduser().resolve())
    run_legacy = Path(legacy_bin).is_file()
    if args.legacy_only and not run_legacy:
        print(f"Error: legacy apx_server not found at {legacy_bin}", file=sys.stderr)
        return 1

    modern_bin = str(args.modern_server.expanduser().resolve())
    run_modern = (not args.legacy_only) and Path(modern_bin).is_file()
    if not run_legacy and not run_modern:
        print(f"Error: neither modern ({modern_bin}) nor legacy ({legacy_bin}) server found", file=sys.stderr)
        return 1

    certs_dir = REPO_ROOT / "tests" / "certs"
    privkey_path = certs_dir / "client_key.pem"
    pubkey_path = certs_dir / "client_pubkey.pem"

    for net_dir in args.networks:
        net_path = net_dir.expanduser().resolve()
        nodes = load_network(apx_node_bin, net_path)
        if args.signed:
            for node in nodes:
                node.signature = sign_data_with_openssl(privkey_path, node.apx_bytes)
        total_apx = sum(len(n.apx_bytes) for n in nodes)
        total_p = sum(n.num_provide for n in nodes)
        total_r = sum(n.num_require for n in nodes)
        print(
            f"\n=== {net_path.name}: {len(nodes)} nodes, {total_apx} B APX, "
            f"{total_p} P-ports, {total_r} R-ports ({args.runs} fresh-server runs over TCP) ==="
        )

        if run_legacy:
            run_benchmark_target(
                "v0.2.9 legacy",
                lambda _tp: start_legacy_server(legacy_bin, find_free_tcp_port()),
                RMFP_1_0_GREETING,
                nodes,
                args.runs,
                show_per_node=args.per_node,
            )

        if run_modern:
            run_benchmark_target(
                "v0.3.x level=0",
                lambda tp: start_modern_tcp_server(tp, modern_bin, find_free_tcp_port(), 0),
                RMFP_1_1_GREETING,
                nodes,
                args.runs,
                show_per_node=args.per_node,
            )
            run_benchmark_target(
                "v0.3.x level=2 (cold)",
                lambda _tp: start_modern_tcp_server(
                    Path(tempfile.mkdtemp(dir=_tp)), modern_bin, find_free_tcp_port(), 2
                ),
                RMFP_1_1_GREETING,
                nodes,
                args.runs,
                show_per_node=args.per_node,
            )
            run_benchmark_target(
                "v0.3.x level=2 (warm)",
                lambda tp: start_modern_tcp_server(tp, modern_bin, find_free_tcp_port(), 2),
                RMFP_1_1_GREETING,
                nodes,
                args.runs,
                warmup_cache=True,
                show_per_node=args.per_node,
            )
            if args.signed:
                run_benchmark_target(
                    "v0.3.x level=0 (signed)",
                    lambda tp: start_modern_tcp_server(
                        tp, modern_bin, find_free_tcp_port(), 0, trusted_pubkey=pubkey_path
                    ),
                    RMFP_1_1_GREETING,
                    nodes,
                    args.runs,
                    signed=True,
                    show_per_node=args.per_node,
                )
                run_benchmark_target(
                    "v0.3.x level=2 (signed, cold)",
                    lambda _tp: start_modern_tcp_server(
                        Path(tempfile.mkdtemp(dir=_tp)),
                        modern_bin,
                        find_free_tcp_port(),
                        2,
                        trusted_pubkey=pubkey_path,
                    ),
                    RMFP_1_1_GREETING,
                    nodes,
                    args.runs,
                    signed=True,
                    show_per_node=args.per_node,
                )
                run_benchmark_target(
                    "v0.3.x level=2 (signed, warm)",
                    lambda tp: start_modern_tcp_server(
                        tp, modern_bin, find_free_tcp_port(), 2, trusted_pubkey=pubkey_path
                    ),
                    RMFP_1_1_GREETING,
                    nodes,
                    args.runs,
                    signed=True,
                    warmup_cache=True,
                    show_per_node=args.per_node,
                )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

