#!/usr/bin/env python3
"""Benchmark apx-server reconnect latency across cache levels 0, 1, and 2."""

import argparse
import hashlib
import sys
import tempfile
import time
import zlib
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO_ROOT / "tests"))

from test_server_compressed_nodes import read_msg, start_server  # noqa: E402
from test_server_definition_events import client, opcode, publish, write  # noqa: E402


def build_definition(num_ports: int) -> bytes:
    lines = ["APX/1.3", 'N"BenchNode"', 'T"Rec_T"{"a"S"b"C"c"L}']
    for i in range(num_ports):
        if i % 3 == 0:
            lines.append(f'R"Port{i:04d}"T["Rec_T"]:={{1,2,3}}')
        else:
            lines.append(f'R"Port{i:04d}"S:={i}')
    return ("\n".join(lines) + "\n").encode("ascii")


def send_chunked(sock, data: bytes, chunk_size: int = 4096) -> None:
    off = 0
    while off < len(data):
        chunk = data[off : off + chunk_size]
        write(sock, chunk, offset=off, more=(off + len(chunk)) < len(data))
        off += len(chunk)


def main() -> int:
    default_server = REPO_ROOT / "build" / "clang-release" / "app" / "apx_server" / "apx-server"
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server", type=Path, default=default_server, help="Path to apx-server executable")
    parser.add_argument("--ports", type=int, default=1000, help="Number of require ports in benchmark node")
    parser.add_argument("--iters", type=int, default=50, help="Number of timed reconnect iterations per level")
    parser.add_argument("--compressed", action="store_true", help="Publish compressed .apx definitions")
    args = parser.parse_args()

    server_bin = str(args.server.resolve())
    if not Path(server_bin).is_file():
        print(f"Error: apx-server executable not found at {server_bin}", file=sys.stderr)
        return 1

    big_def = build_definition(args.ports)
    digest = hashlib.sha256(big_def).digest()
    compressed_payload = zlib.compress(big_def) if args.compressed else None

    for level in (0, 1, 2):
        with tempfile.TemporaryDirectory() as td:
            tp = Path(td)
            srv = start_server(
                tp,
                server_bin,
                {
                    "apx-server": {"cache": {"level": level, "path": str(tp / "cache")}},
                    "socket-server-extension": {"unix-file": str(tp / "apx.socket")},
                },
            )
            try:
                # 1 warmup connection to populate cache
                with client(srv) as sock:
                    payload, resp = publish(
                        sock,
                        big_def,
                        name="BenchNode",
                        compressed=args.compressed,
                        payload=compressed_payload,
                        digest=digest,
                    )
                    if opcode(resp) == 10:
                        send_chunked(sock, payload)
                        read_msg(sock)

                # Timed reconnect loop
                t0 = time.perf_counter()
                for _ in range(args.iters):
                    with client(srv) as sock:
                        payload, resp = publish(
                            sock,
                            big_def,
                            name="BenchNode",
                            compressed=args.compressed,
                            payload=compressed_payload,
                            digest=digest,
                        )
                        if opcode(resp) == 10:
                            send_chunked(sock, payload)
                            resp = read_msg(sock)
                        code = opcode(resp)
                        if code != 3:
                            raise RuntimeError(f"Got code: {code}, expected 3")
                dt_ms = (time.perf_counter() - t0) * 1000.0 / args.iters
                print(f"level={level}: {dt_ms:.2f} ms / reconnect")
            finally:
                srv.stop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
