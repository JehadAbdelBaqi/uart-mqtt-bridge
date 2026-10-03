# Test broker setup scripts

Scripts that set up a local MQTT broker for testing the bridge: a Mosquitto
broker in a virtual machine on the development PC, reachable from the local
network over TLS with client certificates.

This covers the **connection to the broker only** — not building or flashing
the bridge firmware.

## What they do

```
 device on the network ──TLS, port 8883──► PC's LAN address ──► VM "broker" (Mosquitto)
```

| Script | Job |
|--------|-----|
| `standalone/create-broker-vm.sh` | Creates the VM and installs Mosquitto, set to require TLS and a client certificate |
| `setup-certs.sh` | Creates the certificates, installs them on the broker, points the PC's port at the VM, then tests the connection |
| `standalone/self-destruct.sh` | Deletes everything the other two created |
| `config.sh` | The settings all three read: project name, VM name, port, adapter names. Your own copy, not in the repo |
| `config.example.sh` | The template `config.sh` is copied from |
| `helpers/` | The functions `setup-certs.sh` is built from, grouped by subject |

`setup-certs.sh` in order:

1. **Access to the VM** — finds the VM's address, creates an SSH key for this project and adds it to the VM.
2. **Certificates** — creates a CA, a client certificate, and a server certificate named for the PC's current LAN address, then copies the CA certificate, client certificate and client key into `include/secrets/` for the firmware.
3. **Broker** — copies the CA certificate and the server certificate and key to the VM and restarts Mosquitto.
4. **PC** — passes the broker's port on the PC's LAN address to the VM, and lets the local network through the firewall on that port.
5. **Test** — opens a TLS connection to the PC's LAN address with the client certificate and checks the broker's certificate. Afterwards the two rules from step 4 are removed, unless `--keep-alive` was given.

## Why it's done this way

- **A script, not commands typed by hand.** The setup has many steps and has to be repeated: the VM's address and the PC's LAN address both change, and each change breaks the port rule or the server certificate. One command puts it all right again.
- **Safe to run again.** The SSH key, the CA and the client certificate are created only if they are missing, so they stay the same from run to run. The server certificate and the port rule are remade every run, because they depend on addresses that change.
- **The CA stays on the PC, outside the VM.** The VM can be deleted and recreated without losing the CA, so the client certificate keeps working with the new broker.
- **Nothing typed in.** The scripts find the VM's address and the PC's LAN address themselves; everything else comes from `config.sh`.
- **Settings in one file.** Names and ports are set once in `config.sh` and passed into the functions, so the functions hold no project-specific values.
- **A way to start over.** `self-destruct.sh` removes the VM, the certificates, the SSH key and the PC's rules, so the setup can be proven from a clean slate.
- **The PC is left as it was found.** Passing traffic between WSL's network and the VM's network is switched on only while the certificates are copied to the VM. The port rule and the firewall rule are removed after the test unless they are asked for with `--keep-alive`.

## Windows and Linux

**The scripts currently work on Windows only.** Linux support is planned and will be worked on shortly; the Linux branches in the scripts are written but have not been run.

On Windows the work is split across two places, because the tools live in different ones:

| Where | What runs there |
|-------|-----------------|
| **Git Bash** (on Windows) | The scripts themselves, Multipass, and the Windows network settings (port rule, firewall rule, adapter forwarding) |
| **WSL** | Everything that uses Linux tools: creating the keys and certificates, SSH to the VM, the connection test |

The scripts are started in Git Bash and hand the Linux parts to WSL themselves. The keys and certificates therefore live in the WSL home folder, not on the Windows side.

`IS_WINDOWS` in `config.sh` is the switch between the two: on Linux there is no WSL step, and everything runs in the one shell.

## How to set up the connection

### Before you start

- **Windows** with **Multipass** installed and working (`multipass list` answers).
- **WSL** installed and working, with `openssl` and `ssh` available in it.
- **Git Bash**.

### Steps

Run everything **from the `scripts/` folder**.

1. Make your own config file. The scripts read `config.sh`, which is not in the
   repo; `config.example.sh` is the template.
   ```
   cp config.example.sh config.sh
   ```
   Then fill in `config.sh`:

   | Setting | What to put |
   |---------|-------------|
   | `PROJECT_NAME` | A plain name (letters, numbers, `-`, `_`); names the folders the SSH key and certificates are kept in |
   | `VM_NAME` | The name to give the broker's VM |
   | `BROKER_PORT` | The port the broker listens on; `8883` unless you need another |
   | `IS_WINDOWS` | `true` on Windows, `false` on Linux |
   | `WSL_ADAPTER` | Windows: the PC's adapter for WSL's network |
   | `VM_ADAPTER` | Windows: the PC's adapter for the VM's network |
   | `LAN_ADAPTER` | Windows: the PC's adapter on the network the device connects through (Wi-Fi or Ethernet) |

   To list the adapter names on your PC: `netsh interface ipv4 show interfaces`
2. Create the VM (ordinary Git Bash):
   ```
   bash standalone/create-broker-vm.sh
   ```
3. Set up the certificates and the connection (**Git Bash opened as administrator**):
   ```
   bash setup-certs.sh
   ```
   It ends with `Verify return code: 0 (ok)` and `TLS connection works.`

The client certificate and key a device connects with are in
`~/certs/<PROJECT_NAME>/` in WSL (`client.crt`, `client.key`), next to the CA
certificate (`ca.crt`).

### When something changes

| What changed | What to do |
|--------------|------------|
| The PC's LAN address or the VM's address | Run `bash setup-certs.sh` again |
| The VM was deleted | `bash setup-certs.sh --create-vm` — creates the VM, then does the setup |
| Start again from nothing | `bash setup-certs.sh --nuke` — deletes everything (after asking twice), creates the VM, then does the setup |

With no option, `setup-certs.sh` does the setup only and expects the VM to exist.
The two standalone scripts can still be run by themselves.

### Keeping the broker reachable

By default the port rule and the firewall rule are removed again once the test
has passed, so the PC is left closed. To leave them in place — which a device
needs in order to connect — add `--keep-alive`:

```
bash setup-certs.sh --keep-alive
```

The options combine in any order, e.g. `bash setup-certs.sh --nuke --keep-alive`.

### If Multipass hangs

If `create-broker-vm.sh` stops at `Creating VM ...` and `multipass list` no
longer answers, antivirus may be stalling the Multipass service. This was seen
with Avast: with its shields switched off, the service worked again.
