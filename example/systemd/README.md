# Systemd Socket Activation for APX Server

This directory contains sample unit files and configuration to run `apx-server` under systemd with socket activation and permission gating.

## Overview

When using systemd socket activation:
1. `systemd` creates, binds, and configures permissions on `/run/apx/apx.socket` according to `apx-server.socket`.
2. When the first client connects (or at boot), `systemd` spawns `apx-server` and passes the pre-bound file descriptor on file descriptor 3 (`SD_LISTEN_FDS_START`).
3. `apx-server` reads the inherited socket descriptor via zero-dependency `sd_listen_fds()` support and adopts it without creating or modifying the socket file itself.

## Files

* `apx-server.socket`: Systemd socket definition configuring socket path `/run/apx/apx.socket` with permissions (`SocketMode=0660`, `SocketUser=apx`, `SocketGroup=apx-users`).
* `apx-server.service`: Companion service unit specifying sandboxing options and launch command.
* `server.json`: APX server configuration activating `socket-server-extension` with `"unix-systemd": true`.

## Installation

1. Copy configuration:
   ```bash
   sudo mkdir -p /etc/apx
   sudo cp server.json /etc/apx/server.json
   ```

2. Copy systemd units:
   ```bash
   sudo cp apx-server.socket /etc/systemd/system/
   sudo cp apx-server.service /etc/systemd/system/
   sudo systemctl daemon-reload
   ```

3. Enable and start the socket unit:
   ```bash
   sudo systemctl enable --now apx-server.socket
   ```

4. Verify status:
   ```bash
   systemctl status apx-server.socket
   ls -la /run/apx/apx.socket
   ```
