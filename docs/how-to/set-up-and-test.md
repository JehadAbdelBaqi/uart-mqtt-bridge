# How to set up and test the bridge

How to use the scripts in `scripts/`: they set up a local MQTT broker for
testing the bridge, build and upload the firmware, and test the bridge end to end. The broker is Mosquitto in
a virtual machine on the development PC, reachable from the local network over
TLS with client certificates.

[← README](../../README.md)

```
 device on the network ──TLS, port 8883──► PC's LAN address ──► VM "broker" (Mosquitto)
```

## The scripts

Each script does one job and can be run by itself. `e2e.sh` runs them all in
order.

| Script | Job | Run as administrator |
|--------|-----|----------------------|
| `nuke.sh` | Deletes everything the other scripts created | Yes |
| `create-vm.sh` | Creates the VM and installs Mosquitto, set to require TLS and a client certificate | No |
| `setup-certs.sh` | Creates the certificates, installs them on the broker, writes the firmware's files in `include/secrets/`, points the PC's port at the VM, then tests the TLS connection | Yes |
| `build-and-upload.sh` | Builds the firmware and uploads it to the board | No |
| `test-bridge.sh` | Tests the running bridge through the broker, in both directions | No |
| `e2e.sh` | All of the above, in that order: from nothing to a tested bridge | Yes |

| File | Holds |
|------|-------|
| `config.sh` | The settings every script reads: project name, VM name, port, Wi-Fi network, adapter names. Your own copy, not in the repo |
| `config.example.sh` | The template `config.sh` is copied from |
| `helpers/` | The functions the scripts are built from, grouped by subject |

Run everything **from the `scripts/` folder**, in **Git Bash**.

## Before you start

The full list of what is needed, with versions, is in [resources.md](../system/resources.md).

- **Windows** with **Multipass** installed and working (`multipass list` answers).
- **WSL** installed and working, with `openssl` and `ssh` available in it.
- **Git Bash**.
- For `test-bridge.sh` and `e2e.sh`: `mosquitto_pub` and `mosquitto_sub` in WSL (`sudo apt install mosquitto-clients`).
- For `build-and-upload.sh` and `e2e.sh`: **PlatformIO** installed (the VS Code extension is enough).

### Your config file

The scripts read `config.sh`, which is not in the repo; `config.example.sh` is
the template.

```
cp config.example.sh config.sh
```

Then fill in `config.sh`:

| Setting | What to put |
|---------|-------------|
| `PROJECT_NAME` | A plain name (letters, numbers, `-`, `_`); names the folders the SSH key and certificates are kept in |
| `VM_NAME` | The name to give the broker's VM |
| `BROKER_PORT` | The port the broker listens on; `8883` unless you need another |
| `WIFI_SSID` / `WIFI_PASSWORD` | The Wi-Fi network the bridge joins; written into the firmware's `wifi.h` |
| `IS_WINDOWS` | `true` on Windows, `false` on Linux |
| `WSL_ADAPTER` | Windows: the PC's adapter for WSL's network |
| `VM_ADAPTER` | Windows: the PC's adapter for the VM's network |
| `LAN_ADAPTER` | Windows: the PC's adapter on the network the device connects through (Wi-Fi or Ethernet) |

To list the adapter names on your PC: `netsh interface ipv4 show interfaces`.
`setup-certs.sh` checks the three names before it changes anything.

## Everything at once: `e2e.sh`

```
bash e2e.sh
```

Before running it:

- The board is plugged in, and no serial monitor is open on its port.
- The UART's TX pin is jumpered to its RX pin (GPIO7 to GPIO6 on the Genesis Mini).
- `DUMMY_LINE_INTERVAL_MS` in `include/config.h` is not `0`.

It runs, in order:

1. `nuke.sh` — deletes the old setup, after asking twice.
2. `create-vm.sh` — creates the VM with Mosquitto.
3. `setup-certs.sh --keep-alive` — certificates, the firmware's files, the port rule and firewall rule, the TLS test.
4. `build-and-upload.sh` — the firmware, built with the files step 3 has just written.
5. `test-bridge.sh` — the bridge, through the broker.

It ends with `End-to-end test passed.`, and then removes the port rule and the
firewall rule again.

| Option | Effect |
|--------|--------|
| `--skip-nuke` | Keeps the VM and certificates that exist: steps 1 and 2 are left out |
| `--keep-alive` | Leaves the port rule and the firewall rule in place at the end, so the bridge stays connected |

The options combine in any order, e.g. `bash e2e.sh --skip-nuke --keep-alive`.

## One at a time

### `nuke.sh`

```
bash nuke.sh
```

Deletes, if they exist: the VM, the certificates (including the CA), the SSH
key, the firmware's files in `include/secrets/`, the port rule and the firewall
rule. It lists them, asks `y/n`, then asks for the project name to be typed
before anything is deleted.

### `create-vm.sh`

```
bash create-vm.sh
```

Creates the VM named in `config.sh`, installs Mosquitto, and writes its TLS
settings. It stops if a VM of that name already exists. Mosquitto keeps running
without TLS until `setup-certs.sh` has installed the certificates.

### `setup-certs.sh`

```
bash setup-certs.sh
```

The VM must exist. In order:

0. **Checks** — starts WSL (its network adapter only exists while WSL is running); stops straight away if an adapter name in `config.sh` isn't found on the PC.
1. **Access to the VM** — finds the VM's address, creates an SSH key for this project and adds it to the VM.
2. **Certificates** — creates a CA, a client certificate, and a server certificate named for the PC's current LAN address.
3. **Firmware files** — copies the CA certificate, client certificate and client key into `include/secrets/`, and writes `include/secrets/broker.h` (the broker's address — the PC's LAN address — and port) and `include/secrets/wifi.h` (the Wi-Fi network from `config.sh`).
4. **Broker** — copies the CA certificate and the server certificate and key to the VM and restarts Mosquitto.
5. **PC** — passes the broker's port on the PC's LAN address to the VM, and lets the local network through the firewall on that port.
6. **Test** — opens a TLS connection to the PC's LAN address with the client certificate and checks the broker's certificate.

It ends with `Verify return code: 0 (ok)` and `TLS connection works.`

By default the port rule and the firewall rule are removed again when the
script ends, so the PC is left closed. To leave them in place — which a device
needs in order to connect — add `--keep-alive`:

```
bash setup-certs.sh --keep-alive
```

The client certificate and key a device connects with are in
`~/certs/<PROJECT_NAME>/` in WSL (`client.crt`, `client.key`), next to the CA
certificate (`ca.crt`).

### `build-and-upload.sh`

```
bash build-and-upload.sh
```

Builds the firmware with the files in `include/secrets/` and uploads it to the
board. The board must be plugged in, with no serial monitor open on its port.
Run it after `setup-certs.sh` whenever that has written new files: the
certificates and the broker's address are built into the firmware.

### `test-bridge.sh`

```
bash test-bridge.sh
```

Tests the running bridge, only through the broker. It needs the firmware
running with the test config, the jumper from TX to RX, and the broker
reachable (`setup-certs.sh --keep-alive`).

| Check | Proves |
|-------|--------|
| A line from the bridge arrives on `bridge/test/up` within 60 s | The bridge started, joined Wi-Fi, connected to the broker over TLS, and published a line read from its UART |
| `T,e2e-<number>` published to `bridge/test/down` comes back on `bridge/test/up` within 10 s | The bridge is subscribed, wrote the message to its UART, and routed the returning line by its first letter |

It ends with `Bridge test passed.`

## When something changes

| What changed | What to do |
|--------------|------------|
| The PC's LAN address or the VM's address | `bash setup-certs.sh --keep-alive`, then `bash build-and-upload.sh` if the PC's address changed (it is built into the firmware) |
| The VM was deleted | `bash create-vm.sh`, then `bash setup-certs.sh` |
| The firmware's code or `include/config.h` | `bash build-and-upload.sh`, then `bash test-bridge.sh` |
| Start again from nothing | `bash e2e.sh` |

## Why it's done this way

- **Scripts, not commands typed by hand.** The setup has many steps and has to be repeated: the VM's address and the PC's LAN address both change, and each change breaks the port rule or the server certificate. One command puts it all right again. The commands themselves are listed in [commands.md](../system/commands.md) for when one is needed by hand.
- **One job per script.** Each step can be run again by itself, and `e2e.sh` stays a short list of the steps.
- **Safe to run again.** The SSH key, the CA and the client certificate are created only if they are missing, so they stay the same from run to run. The server certificate and the port rule are remade every run, because they depend on addresses that change.
- **The CA stays on the PC, outside the VM.** The VM can be deleted and recreated without losing the CA, so the client certificate keeps working with the new broker.
- **Nothing typed in.** The scripts find the VM's address and the PC's LAN address themselves; everything else comes from `config.sh`.
- **Settings in one file.** Names and ports are set once in `config.sh` and passed into the functions, so the functions hold no project-specific values.
- **A way to start over.** `nuke.sh` removes everything the scripts created, so the setup can be proven from a clean slate.
- **The PC is left as it was found.** Passing traffic between WSL's network and the VM's network is switched on only while the certificates are copied to the VM. The port rule and the firewall rule are removed at the end unless they are asked for with `--keep-alive`.
- **The bridge is tested through the broker only.** A message arriving on a topic is a clear pass or fail; the board's serial port isn't read.

## Windows and Linux

**The scripts currently work on Windows only.** Linux support is planned and will be worked on shortly; the Linux branches in the scripts are written but have not been run.

On Windows the work is split across two places, because the tools live in different ones:

| Where | What runs there |
|-------|-----------------|
| **Git Bash** (on Windows) | The scripts themselves, Multipass, PlatformIO, and the Windows network settings (port rule, firewall rule, adapter forwarding) |
| **WSL** | Everything that uses Linux tools: creating the keys and certificates, SSH to the VM, the connection test, the MQTT client tools |

The scripts are started in Git Bash and hand the Linux parts to WSL themselves. The keys and certificates therefore live in the WSL home folder, not on the Windows side.

`IS_WINDOWS` in `config.sh` is the switch between the two: on Linux there is no WSL step, and everything runs in the one shell.

## If Multipass hangs

If `create-vm.sh` stops at `Creating VM ...` and `multipass list` no longer
answers, antivirus may be stalling the Multipass service. This was seen with
Avast: with its shields switched off, the service worked again. Restarting the
service is in [commands.md](../system/commands.md).

---

**See also:** [resources.md](../system/resources.md) · [commands.md](../system/commands.md) · [testing.md](../system/testing.md) · [decisions.md](../project-design/decisions.md)
