# apx_node

## Synopsis

```text
apx_node   [-b --bind bind_path] [-p --bind-port port] [--no-bind]
           [-c --connect connect_path] [-r --connect-port connect_port]
           [--version] [--help]
           file
```

## Description

Starts a console application that takes these initial actions:

- Creates dynamic APX client based on definition given by *file* argument.
- Creates a socket server (UNIX or TCP) based on the *bind* options given. (Defaults are used otherwise.)
- Connects to an APX server based on the *connect* options given. (Defaults are used otherwise.)

Once these preparatory steps have been taken it provides the following functionality.

- It listens for JSON messages on the bound socket.
- Connected clients can use this to control values
of provide ports of the node by sending JSON.
- It also listens for APX messages from the APX server.
- When any require ports (of the APX node) change value
the new value is printed to stdout.

## Mandatory Arguments

```text
file     path to an APX definition file (.apx)
```

## Options

```text
-b --bind address_or_path
                Either TCP address or UNIX socket to bind to for receiving
                provide-port data updates.

-p --bind-port port
                Port number for server socket (not applicable when path is
                UNIX socket).

-c --connect address_or_path
                Either TCP address or path to UNIX socket to connect to for receiving
                require-port data updates.

-r --connect-port connect_port
                Port number for APX client socket (not applicable when path is
                UNIX socket).

```

## Option Default Values

### Linux Defaults

```text
--bind-path     /tmp/apx_node.socket
--connect-path  /tmp/apx_server.socket
--bind-port     5100
--connect-port  5000
```

### Windows Defaults

```text
--bind-path     127.0.0.1
--connect-path  127.0.0.1
--bind-port     5100
--connect-port  5000
```

## Example Usage

```bash
apx_node -b /tmp/vehicle.socket -c /tmp/apx_server.socket vehicle.apx
apx_node -b /tmp/vehicle.socket -c 192.168.1.19 vehicle.apx
apx_node -p 5101 -r 5001 vehicle.apx
```
