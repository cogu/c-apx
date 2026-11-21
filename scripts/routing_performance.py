#!/usr/bin/env python3
"""Benchmark routing table construction on reconnect: v0.2.8 vs v0.3.x."""

import argparse
import hashlib
import random
import select
import socket
import struct
import sys
import tempfile
import time
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO_ROOT / "tests"))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from legacy_performance import (  # noqa: E402
    RMFP_1_0_GREETING,
    RMFP_1_1_GREETING,
    TcpServerHandle,
    find_free_tcp_port,
    send_chunked,
    start_legacy_server,
    start_modern_tcp_server,
)
from test_server_compressed_nodes import encode_numheader32, read_msg  # noqa: E402
from test_server_definition_events import client, opcode, publish, write  # noqa: E402

OUT_FILE_ADDR = 0x00000000
APX_FILE_ADDR = 0x02000000


def port_order(num_ports: int, shuffled: bool, seed: int = 42) -> list[int]:
    indices = list(range(num_ports))
    if shuffled:
        random.Random(seed).shuffle(indices)
    return indices


def build_node_definition(
    node_name: str,
    port_indices: list[int],
    *,
    is_provider: bool,
    add_ready_require_port: bool = False,
) -> tuple[bytes, bytes]:
    """Build an APX/1.2 definition and (for providers) matching initial .out bytes."""
    direction = "P" if is_provider else "R"
    lines = ["APX/1.2", f'N"{node_name}"', 'T"Rec_T"{"a"S"b"C"c"L}']
    out_buf = bytearray()
    for i in port_indices:
        if i % 3 == 0:
            lines.append(f'{direction}"Port{i:04d}"T[0]:={{1,2,3}}')
            if is_provider:
                out_buf.extend(struct.pack("<HBI", 1, 2, 3))
        else:
            lines.append(f'{direction}"Port{i:04d}"S:={i & 0xFFFF}')
            if is_provider:
                out_buf.extend(struct.pack("<H", i & 0xFFFF))
    if add_ready_require_port:
        lines.append('R"_Ready"C:=0')
    return ("\n".join(lines) + "\n").encode("ascii"), bytes(out_buf)


def send_file_info_no_wait(
    sock: socket.socket,
    *,
    name: str,
    address: int,
    size: int,
    digest: bytes | None = None,
) -> None:
    digest_type = 2 if digest is not None else 0
    digest_bytes = digest if digest is not None else (b"\x00" * 32)
    header = struct.pack("<IIIHH", 3, address, size, 0, digest_type)
    command = header + digest_bytes + f"{name}\0".encode("ascii")
    sock.sendall(encode_numheader32(struct.pack(">I", 0xBFFFFC00) + command))


def file_info_name(msg: bytes) -> str:
    # 4-byte RMF addr + 48-byte RMF_CMD_FILE_INFO header
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


def connect_provider(
    srv: TcpServerHandle,
    apx_def: bytes,
    out_data: bytes,
    greeting: bytes,
    name: str = "Provider",
) -> socket.socket:
    """Connect and synchronize a Provider node, keeping its socket open."""
    sock = srv.connect()
    try:
        sock.sendall(encode_numheader32(greeting))
        ack = read_msg(sock, timeout=10.0)
        if not ack or opcode(ack) not in (0, 20):
            raise RuntimeError(f"Provider greeting failed: opcode={opcode(ack) if ack else None}")

        digest = hashlib.sha256(apx_def).digest()
        send_file_info_no_wait(sock, name=f"{name}.out", address=OUT_FILE_ADDR, size=len(out_data))
        send_file_info_no_wait(sock, name=f"{name}.apx", address=APX_FILE_ADDR, size=len(apx_def), digest=digest)

        sent_out = False
        got_in = False
        deadline = time.monotonic() + 30.0
        while not (sent_out and got_in):
            remaining = max(0.1, deadline - time.monotonic())
            msg = read_msg(sock, timeout=remaining)
            if not msg:
                raise RuntimeError("Provider connection closed unexpectedly during setup")
            code = opcode(msg)
            if code == 10:  # RMF_CMD_OPEN_FILE
                addr = int.from_bytes(msg[8:12], "little")
                if addr == APX_FILE_ADDR:
                    send_chunked(sock, apx_def)
                elif addr == OUT_FILE_ADDR:
                    off = 0
                    while off < len(out_data):
                        chunk = out_data[off : off + 4096]
                        write(sock, chunk, address=OUT_FILE_ADDR, offset=off, more=(off + len(chunk)) < len(out_data))
                        off += len(chunk)
                    sent_out = True
            elif code == 3:  # RMF_CMD_FILE_INFO
                if file_info_name(msg) == f"{name}.in":
                    got_in = True
        drain_socket(sock)
        return sock
    except Exception:
        sock.close()
        raise


def bench_requester(
    srv: TcpServerHandle,
    req_def: bytes,
    digest: bytes,
    greeting: bytes,
    iters: int,
    *,
    provider_sock: socket.socket | None = None,
    cooldown_s: float = 0.005,
    timeout_s: float = 30.0,
) -> float:
    def one_reconnect() -> None:
        with client(srv, greeting=greeting) as sock:
            payload, resp = publish(sock, req_def, name="Requester", compressed=False, digest=digest)
            if opcode(resp) == 10:
                send_chunked(sock, payload)
                resp = read_msg(sock, timeout=timeout_s)
            # On v0.3.x, the server may publish .in and .cin; wait for Requester.in.
            while opcode(resp) != 3 or file_info_name(resp) != "Requester.in":
                resp = read_msg(sock, timeout=timeout_s)
                if not resp:
                    raise RuntimeError("Requester disconnected before receiving Requester.in")

    # 1 warmup reconnect (also populates v0.3.x cache)
    one_reconnect()
    if provider_sock is not None:
        drain_socket(provider_sock)
    if cooldown_s > 0.0:
        time.sleep(cooldown_s)

    elapsed = 0.0
    for _ in range(iters):
        t0 = time.perf_counter()
        one_reconnect()
        elapsed += time.perf_counter() - t0
        if provider_sock is not None:
            drain_socket(provider_sock)
        if cooldown_s > 0.0:
            time.sleep(cooldown_s)

    return elapsed * 1000.0 / iters


def run_server_suite(
    label: str,
    srv: TcpServerHandle,
    greeting: bytes,
    num_ports: int,
    iters: int,
    prov_def: bytes,
    prov_out: bytes,
    req_ordered: bytes,
    req_shuffled: bytes,
) -> dict[str, float]:
    results: dict[str, float] = {}
    ordered_digest = hashlib.sha256(req_ordered).digest()
    shuffled_digest = hashlib.sha256(req_shuffled).digest()

    # 1. No provider (baseline: parse/compile/cache only, zero port routes)
    results["no-provider"] = bench_requester(srv, req_ordered, ordered_digest, greeting, iters)

    # 2. Live provider + ordered / shuffled requester (full N-port routing)
    prov_sock = connect_provider(srv, prov_def, prov_out, greeting)
    try:
        results["ordered"] = bench_requester(
            srv, req_ordered, ordered_digest, greeting, iters, provider_sock=prov_sock
        )
        results["shuffled"] = bench_requester(
            srv, req_shuffled, shuffled_digest, greeting, iters, provider_sock=prov_sock
        )
    finally:
        prov_sock.close()

    print(
        f"  {label:<22} "
        f"no-provider: {results['no-provider']:6.2f} ms | "
        f"ordered: {results['ordered']:6.2f} ms | "
        f"shuffled: {results['shuffled']:6.2f} ms"
    )
    return results


def main() -> int:
    default_legacy = Path.home() / "bin" / "apx_server"
    default_modern = REPO_ROOT / "build" / "clang-release" / "app" / "apx_server" / "apx-server"

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--legacy-server", type=Path, default=default_legacy, help="Path to v0.2.8 apx_server")
    parser.add_argument("--modern-server", type=Path, default=default_modern, help="Path to v0.3.x apx-server")
    parser.add_argument("--ports", type=int, nargs="+", default=[1000, 5000], help="Port counts to benchmark")
    parser.add_argument("--iters", type=int, default=20, help="Timed reconnect iterations per scenario")
    parser.add_argument("--legacy-only", action="store_true", help="Benchmark only the legacy v0.2.8 server")
    args = parser.parse_args()

    legacy_bin = str(args.legacy_server.expanduser().resolve())
    if not Path(legacy_bin).is_file():
        print(f"Error: legacy apx_server not found at {legacy_bin}", file=sys.stderr)
        return 1

    modern_bin = str(args.modern_server.expanduser().resolve())
    run_modern = (not args.legacy_only) and Path(modern_bin).is_file()
    if not args.legacy_only and not run_modern:
        print(f"Warning: modern apx-server not found at {modern_bin}, skipping v0.3.x", file=sys.stderr)

    for num_ports in args.ports:
        ordered_ids = port_order(num_ports, shuffled=False)
        shuffled_ids = port_order(num_ports, shuffled=True)
        prov_def, prov_out = build_node_definition(
            "Provider", ordered_ids, is_provider=True, add_ready_require_port=True
        )
        req_ordered, _ = build_node_definition("Requester", ordered_ids, is_provider=False)
        req_shuffled, _ = build_node_definition("Requester", shuffled_ids, is_provider=False)

        print(
            f"\n=== {num_ports} ports ({len(req_ordered)} B APX/1.2, {len(prov_out)} B .out, "
            f"{args.iters} reconnects over TCP) ==="
        )

        port = find_free_tcp_port()
        legacy_srv = start_legacy_server(legacy_bin, port)
        try:
            run_server_suite(
                "v0.2.8 legacy",
                legacy_srv,
                RMFP_1_0_GREETING,
                num_ports,
                args.iters,
                prov_def,
                prov_out,
                req_ordered,
                req_shuffled,
            )
        finally:
            legacy_srv.stop()

        if run_modern:
            for level in (0, 2):
                with tempfile.TemporaryDirectory() as td:
                    mport = find_free_tcp_port()
                    srv = start_modern_tcp_server(Path(td), modern_bin, mport, level)
                    try:
                        run_server_suite(
                            f"v0.3.x level={level}",
                            srv,
                            RMFP_1_1_GREETING,
                            num_ports,
                            args.iters,
                            prov_def,
                            prov_out,
                            req_ordered,
                            req_shuffled,
                        )
                    finally:
                        srv.stop()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
