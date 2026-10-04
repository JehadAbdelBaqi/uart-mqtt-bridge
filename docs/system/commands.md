# Commands

The commands the scripts run, grouped by tool, with what each one is for — so
any of them can be lifted out and run by hand.

[← README](../../README.md)

Words in `<angle brackets>` are placeholders. The values the scripts use come
from `scripts/config.sh`, and the ones that are the same for every setup from
`scripts/helpers/settings.sh`:

| Placeholder | Value |
|-------------|-------|
| `<project>` | `PROJECT_NAME` |
| `<vm>` | `VM_NAME` |
| `<port>` | `BROKER_PORT` (8883) |
| `<vm address>` | The VM's address, from `multipass list` |
| `<lan address>` | The PC's address on the local network; also `BROKER_ADDRESS` in `include/secrets/broker.h` |
| `<certs>` | `~/certs/<project>` |

**Which script runs what** — the scripts are in `scripts/steps/`:

| Script | Sections it draws on |
|--------|----------------------|
| `nuke.sh` | Multipass, SSH (forgetting the VM's identity), Linux network settings |
| `create-vm.sh` | Multipass, Inside the VM |
| `setup-certs.sh` | Multipass, SSH, OpenSSL, Inside the VM, Linux network settings |
| `build-and-upload.sh` | PlatformIO |
| `test-bridge.sh` | MQTT, Linux network settings (finding the PC's address) |
| `windows/windows.sh` | WSL, Windows network settings (adapters) |

The commands are given as they run on Linux. On Windows the scripts run in
WSL, where the same commands apply; the two Windows-only sections say where
theirs run, and the Linux network settings are replaced by the Windows ones.

## Multipass — the VM

On Windows the command is `multipass.exe`.

| Purpose | Command |
|---------|---------|
| Create a VM | `multipass launch --name <vm>` |
| List the VMs with their state and address | `multipass list` |
| The same as comma-separated text, for a script to read | `multipass list --format csv` |
| Everything about one VM (state, address, disk, memory) | `multipass info <vm>` |
| Open a shell inside the VM | `multipass shell <vm>` |
| Run one command inside the VM | `multipass exec <vm> -- bash -c "<command>"` |
| Delete the VM for good | `multipass delete --purge <vm>` |

The Multipass service, when `multipass list` stops answering (admin PowerShell):

| Purpose | Command |
|---------|---------|
| Restart the service | `Restart-Service Multipass` |
| Stop it by force when a restart hangs | `Stop-Process -Name multipassd -Force` |
| Start it again | `Start-Service Multipass` |

## Inside the VM — Mosquitto

Runs inside the VM: after `multipass shell <vm>`, or through `multipass exec`
or `ssh`.

| Purpose | Command |
|---------|---------|
| Install the broker and its client tools | `sudo apt-get update && sudo apt-get install -y mosquitto mosquitto-clients` |
| Put the CA certificate where Mosquitto reads it | `sudo install -o root -g root -m 644 ~/ca.crt /etc/mosquitto/ca_certificates/ca.crt` |
| Put the server certificate in place | `sudo install -o root -g root -m 644 ~/server.crt /etc/mosquitto/certs/server.crt` |
| Put the server key in place, readable by Mosquitto only | `sudo install -o mosquitto -g mosquitto -m 600 ~/server.key /etc/mosquitto/certs/server.key` |
| Restart Mosquitto so it reads new settings or certificates | `sudo systemctl restart mosquitto` |
| Check that it is running (prints `active`) | `systemctl is-active mosquitto` |
| Stop it / start it | `sudo systemctl stop mosquitto` / `sudo systemctl start mosquitto` |

Mosquitto's TLS settings are one file, `/etc/mosquitto/conf.d/tls.conf`:

```
listener <port>
cafile /etc/mosquitto/ca_certificates/ca.crt
certfile /etc/mosquitto/certs/server.crt
keyfile /etc/mosquitto/certs/server.key
require_certificate true
use_identity_as_username true
```

| Line | Meaning |
|------|---------|
| `listener` | The port Mosquitto listens on |
| `cafile` | The CA that client certificates are checked against |
| `certfile` / `keyfile` | The broker's own certificate and its private key |
| `require_certificate true` | A client without a certificate is refused |
| `use_identity_as_username true` | The name in the client's certificate is its username |

## WSL

Windows only. Runs in Git Bash or PowerShell.

| Purpose | Command |
|---------|---------|
| Start WSL without doing anything in it (its network adapter only exists while it runs) | `wsl -e true` |
| Run one of the scripts in WSL, from `scripts/` | `wsl bash <script> <options>` |

## SSH — reaching the VM

The project has its own key, in `~/.ssh/<project>/`.

| Purpose | Command |
|---------|---------|
| Create the project's key pair, with no passphrase | `ssh-keygen -t ed25519 -N '' -C '<project>' -f ~/.ssh/<project>/id_ed25519` |
| Show the public key | `cat ~/.ssh/<project>/id_ed25519.pub` |
| Let that key log in to the VM | `multipass exec <vm> -- bash -c "echo '<public key>' >> ~/.ssh/authorized_keys"` |
| Check the login works (prints the VM's name) | `ssh <options> ubuntu@<vm address> hostname` |
| Run one command on the VM | `ssh <options> ubuntu@<vm address> '<command>'` |
| Copy files to the VM's home folder | `scp <options> <file>... ubuntu@<vm address>:` |
| Forget a VM's identity, after it was deleted and its address is reused | `ssh-keygen -R <vm address>` |

`<options>` is `SSH_OPTIONS` from `helpers/settings.sh`:

| Option | Meaning |
|--------|---------|
| `-i ~/.ssh/<project>/id_ed25519` | Use the project's key |
| `-o IdentitiesOnly=yes` | Offer that key only, not every key the PC has |
| `-o BatchMode=yes` | Never stop to ask a question; fail instead |
| `-o ConnectTimeout=5` | Give up after 5 s if the VM doesn't answer |
| `-o StrictHostKeyChecking=accept-new` | Trust a VM seen for the first time; refuse one whose identity has changed |

On Windows, WSL can only reach the VM while forwarding is switched on between
their two network adapters (see Windows network settings).

## OpenSSL — certificates

Run with the files in `<certs>`. Every key is RSA 2048; every
certificate lasts 3650 days.

**The CA** — signs the other two certificates:

| Purpose | Command |
|---------|---------|
| Create the CA's private key | `openssl genrsa -out <certs>/ca.key 2048` |
| Create the CA's certificate, signed by its own key | `openssl req -x509 -new -key <certs>/ca.key -sha256 -days 3650 -subj '/CN=<project> test CA' -out <certs>/ca.crt` |

**The client certificate** — the bridge's identity, also used by `mosquitto_pub` / `mosquitto_sub`:

| Purpose | Command |
|---------|---------|
| Create the client's private key | `openssl genrsa -out <certs>/client.key 2048` |
| Create a signing request for it | `openssl req -new -key <certs>/client.key -subj '/CN=<project> client' -out <certs>/client.csr` |
| Have the CA sign it | `openssl x509 -req -in <certs>/client.csr -CA <certs>/ca.crt -CAkey <certs>/ca.key -CAcreateserial -sha256 -days 3650 -out <certs>/client.crt` |

**The server certificate** — the broker's identity, named for the address clients connect to:

| Purpose | Command |
|---------|---------|
| Create the server's private key | `openssl genrsa -out <certs>/server.key 2048` |
| Create a signing request for it | `openssl req -new -key <certs>/server.key -subj '/CN=<lan address>' -out <certs>/server.csr` |
| Have the CA sign it, with the address as a subject alternative name | `openssl x509 -req -in <certs>/server.csr -CA <certs>/ca.crt -CAkey <certs>/ca.key -CAcreateserial -sha256 -days 3650 -extfile <(printf 'subjectAltName=IP:<lan address>') -out <certs>/server.crt` |

The server certificate has to be remade whenever the PC's LAN address changes:
a client checks that the address it connected to is the one in the certificate.

**Testing the connection:**

| Purpose | Command |
|---------|---------|
| Open a TLS connection to the broker with the client certificate, checking the broker's certificate against the CA and the address | `openssl s_client -connect <lan address>:<port> -CAfile <certs>/ca.crt -cert <certs>/client.crt -key <certs>/client.key -verify_ip <lan address> -verify_return_error < /dev/null` |

A good connection prints `Verify return code: 0 (ok)`.

## Linux network settings

The port rule needs `sudo`. `<tag>` is `broker-port-<port>`: a comment put on
each rule so the scripts can find their own rules again.

| Purpose | Command |
|---------|---------|
| Find the PC's LAN address (the `src` address in the answer) | `ip -4 route get 1.1.1.1` |
| Let the PC pass traffic on to another network | `sudo sysctl -w net.ipv4.ip_forward=1` |
| Pass `<port>` on the PC's LAN address to the same port on the VM, for connections from other devices | `sudo iptables -t nat -A PREROUTING -d <lan address> -p tcp --dport <port> -m comment --comment <tag> -j DNAT --to-destination <vm address>:<port>` |
| The same, for connections made from the PC itself | `sudo iptables -t nat -A OUTPUT -d <lan address> -p tcp --dport <port> -m comment --comment <tag> -j DNAT --to-destination <vm address>:<port>` |
| Let that traffic through to the VM | `sudo iptables -I FORWARD -d <vm address> -p tcp --dport <port> -m comment --comment <tag> -j ACCEPT` |
| List the rules in a chain, as the commands that added them | `sudo iptables -t nat -S PREROUTING` · `sudo iptables -t nat -S OUTPUT` · `sudo iptables -S FORWARD` |
| Remove a rule: the command that added it, with `-A` or `-I` changed to `-D` | `sudo iptables -t nat -D PREROUTING ...` |

A device can connect to the broker only while the rules are in place.

## Windows network settings

Windows only. Needs a terminal opened as administrator. In Git Bash the
command is `netsh`; inside WSL it is `netsh.exe`.

**Adapters:**

| Purpose | Command |
|---------|---------|
| List the PC's network adapters by name | `netsh interface ipv4 show interfaces` |
| Check one adapter exists | `netsh interface ipv4 show interface "<adapter>"` |
| Show an adapter's address (how the PC's LAN address is found) | `netsh interface ipv4 show addresses "<adapter>"` |
| Let traffic pass through an adapter to another network (done on WSL's adapter and the VM's adapter, so WSL can reach the VM) | `netsh interface ipv4 set interface "<adapter>" forwarding=enabled` |
| Switch that off again | `netsh interface ipv4 set interface "<adapter>" forwarding=disabled` |

**The port rule** — passes connections arriving at the PC to the VM:

| Purpose | Command |
|---------|---------|
| List the port rules | `netsh interface portproxy show v4tov4` |
| Pass `<port>` on the PC's LAN address to the same port on the VM | `netsh interface portproxy add v4tov4 listenport=<port> listenaddress=<lan address> connectport=<port> connectaddress=<vm address>` |
| Remove that rule | `netsh interface portproxy delete v4tov4 listenport=<port> listenaddress=<lan address>` |

**The firewall rule** — lets devices on the local network reach that port:

| Purpose | Command |
|---------|---------|
| Check whether the rule exists | `netsh advfirewall firewall show rule name="broker-port-<port>"` |
| Create it, for the local network only | `netsh advfirewall firewall add rule name="broker-port-<port>" dir=in action=allow protocol=TCP localport=<port> remoteip=localsubnet` |
| Remove it | `netsh advfirewall firewall delete rule name="broker-port-<port>"` |

A device can connect to the broker only while both rules are in place.

| Purpose | Command |
|---------|---------|
| Check the terminal has administrator rights (fails without them) | `net session` |

## MQTT — sending and watching messages

Every command takes the same four connection options:

```
-h <lan address> -p <port> --cafile <certs>/ca.crt --cert <certs>/client.crt --key <certs>/client.key
```

Written below as `<connection>`.

| Purpose | Command |
|---------|---------|
| Watch a topic; prints `<topic> <message>` for each message and waits for more | `mosquitto_sub <connection> -t <topic> -v` |
| Wait for one message, giving up after a number of seconds | `mosquitto_sub <connection> -t <topic> -C 1 -W <seconds>` |
| Publish a message at QoS 1 | `mosquitto_pub <connection> -t <topic> -m "<message>" -q 1` |

`mosquitto_sub` prints nothing until a message arrives. With the bridge's test
config, the bridge publishes to `bridge/test/up` and is subscribed to
`bridge/test/down`.

## PlatformIO — the firmware

`<platformio>` is PlatformIO's own program, `PLATFORMIO` in `helpers/settings.sh`
(`~/.platformio/penv/bin/platformio`; on Windows it is set in `windows/config.sh`); `<repo>` is the repository's folder.

| Purpose | Command |
|---------|---------|
| Build the firmware | `<platformio> run --project-dir <repo>` |
| Build it and upload it to the board | `<platformio> run --project-dir <repo> --target upload` |

## Git

| Purpose | Command |
|---------|---------|
| Switch on the pre-commit hook that refuses secrets and checks the shell scripts, once per clone | `git config core.hooksPath .githooks` |
| Check every shell script with ShellCheck, as the hook does for the staged ones (from the repository's root) | `shellcheck --shell=bash --external-sources --source-path=scripts $(git ls-files '*.sh')` |

## See also

[set-up-and-test.md](../how-to/set-up-and-test.md) · [testing.md](testing.md) · [decision_logs.md](../project-design/decision_logs.md)
