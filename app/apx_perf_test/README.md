# apx_perf_test

## Synopsis

```text
apx_perf_test [-c --connect connect_path]
              [-s --slave] [-t --time seconds]
              [--version] [--help]
```

## Description

`apx_perf_test` is a benchmark tool that measures APX signal throughput (events per second) across the APX server using a ping-pong round-trip communication pattern between two virtual nodes:

- **Requester (Master / Default mode):**
  Defines `RequestNode` with provide-port `PerfTest_rqst` and require-port `PerfTest_rsp`. When connected, it initiates the benchmark by writing `1` to `PerfTest_rqst`, measures round-trip replies continuously for a specified duration, and calculates average throughput (`events/sec`).

- **Responder (Slave mode, `-s`):**
  Defines `RespondNode` with require-port `PerfTest_rqst` and provide-port `PerfTest_rsp`. It acts as a passive responder, echoing incoming writes back to the requester with an incremented counter.

## Options

```text
-c --connect address_or_path
                Either path to UNIX socket, or TCP address/hostname with
                optional port (e.g. localhost:5000, 127.0.0.1:5000, or
                /tmp/apx.socket).

-s --slave
                Run in responder (slave) mode. When omitted, runs in requester
                (master) mode.

-t --time seconds
                Duration in seconds to run the performance test (master only).

--version
                Print application version and exit.

-h --help
                Print help message and exit.
```

## Option Default Values

### Linux Defaults

```text
--connect  /tmp/apx.socket
--time     5
```

### Windows Defaults

```text
--connect  127.0.0.1:5000
--time     5
```

## Execution Order

Always start the instances in the following order:

1. **APX Server**: Must be running so both nodes can connect and route ports.
2. **Slave / Responder (`-s`)**: Must be started **before** the master so its `RespondNode` ports are registered with the server and ready to echo the first event.
3. **Master / Requester (default)**: Starts the test immediately upon connecting, runs for the configured time (`-t`), and prints results upon completion.

## Example Usage

### 1. Start the APX Server

```bash
build/app/apx_server/apx_server example/config
```

### 2. Start the Responder (Slave)

In a second terminal:

```bash
build/app/apx_perf_test/apx_perf_test -s
```

Output:
```text
Running in responder mode
Connecting to /tmp/apx.socket
Connected to server
```

### 3. Start the Requester (Master)

In a third terminal:

```bash
build/app/apx_perf_test/apx_perf_test -t 5
```

Output:
```text
Running in requester mode
Connecting to /tmp/apx.socket
Connected to server
Test started
Test completed.
Total Events: 58420. Events/s: 11684.00.
```

### TCP Connection Example

```bash
# Slave
build/app/apx_perf_test/apx_perf_test -s -c localhost:5000

# Master (run for 10 seconds)
build/app/apx_perf_test/apx_perf_test -c localhost:5000 -t 10
```
