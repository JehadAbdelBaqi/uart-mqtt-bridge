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
| `unit-tests.sh` | PlatformIO |
| `nuke.sh` | Multipass, SSH (forgetting the VM's identity), Linux network settings |
| `clean-build.sh` | None: it deletes the build folder, `.pio/build` |
| `create-ssh-key.sh` | SSH (creating the key) |
| `create-vm.sh` | Multipass |
| `setup-certs.sh` | OpenSSL, Linux network settings (finding the PC's address) |
| `setup-broker.sh` | Multipass (finding the VM's address), Ansible |
| `set-port-rule.sh` | Multipass, Linux network settings, OpenSSL (testing the connection) |
| `remove-port-rule.sh` | Linux network settings |
| `build-and-upload.sh` | PlatformIO |
| `test-bridge.sh` | MQTT, Linux network settings (finding the PC's address) |

The commands are given as they run on Linux.

## Multipass — the VM

| Purpose | Command |
|---------|---------|
| Create a VM | `multipass launch --name <vm>` |
| Create it with a cloud-init file, read from the command's input | `<the file's contents> \| multipass launch --name <vm> --cloud-init -` |
| List the VMs with their state and address | `multipass list` |
| The same as comma-separated text, for a script to read | `multipass list --format csv` |
| Everything about one VM (state, address, disk, memory) | `multipass info <vm>` |
| Open a shell inside the VM | `multipass shell <vm>` |
| Run one command inside the VM | `multipass exec <vm> -- bash -c "<command>"` |
| Delete the VM for good | `multipass delete --purge <vm>` |

## Ansible — setting the VM up

Run on the PC. Ansible logs into the VM over SSH and makes each thing the
playbook describes true there. `<ansible-playbook>` is `ANSIBLE_PLAYBOOK` in
`helpers/settings.sh`.

| Purpose | Command |
|---------|---------|
| Run the broker's playbook on the VM | `<ansible-playbook> ansible/broker.yml --inventory "<vm address>," --user deploy --private-key ~/.ssh/<project>/id_ed25519 --ssh-common-args "-o IdentitiesOnly=yes -o StrictHostKeyChecking=accept-new" --extra-vars "broker_port=<port>" --extra-vars "cert_dir=<certs>"` |
| Check a playbook is written correctly, without running it | `<ansible-playbook> --syntax-check --inventory "localhost," ansible/broker.yml` |
| See what a run would change, without changing it | The first command, with `--check` added |

| Part | Meaning |
|------|---------|
| `--inventory "<vm address>,"` | The machines to run on. The comma marks it as a list of machines, not the name of a file |
| `--user deploy` | The user to log in as: the one the cloud-init file created |
| `--private-key` | The project's key |
| `--ssh-common-args` | Settings handed to SSH: offer that key only, and trust a VM seen for the first time |
| `--extra-vars` | A value the playbook uses, as `name=value` |

What the playbook does in the VM:

| Task | Makes true |
|------|------------|
| Install Mosquitto | The broker and its client tools are installed |
| Write Mosquitto's TLS settings | The settings file below exists |
| Put the CA certificate, the broker's certificate and its private key in place | Each is where Mosquitto reads it; the key is readable by Mosquitto only |
| Have Mosquitto running | It is running, and starts with the VM |
| Restart Mosquitto | Only when the settings or a certificate changed |

## Inside the VM — Mosquitto

For looking at the broker by hand: after `multipass shell <vm>`, or through
`multipass exec` or `ssh`.

| Purpose | Command |
|---------|---------|
| Check that it is running (prints `active`) | `systemctl is-active mosquitto` |
| Restart it | `sudo systemctl restart mosquitto` |
| Stop it / start it | `sudo systemctl stop mosquitto` / `sudo systemctl start mosquitto` |

Mosquitto's TLS settings are one file, `/etc/mosquitto/conf.d/tls.conf`, written by the playbook:

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

## SSH — reaching the VM

The project has its own key, in `~/.ssh/<project>/`. The VM's `deploy` user,
created by the cloud-init file, is the one it logs in as.

| Purpose | Command |
|---------|---------|
| Create the project's key pair, with no passphrase | `ssh-keygen -t ed25519 -N '' -C '<project>' -f ~/.ssh/<project>/id_ed25519` |
| Show the public key | `cat ~/.ssh/<project>/id_ed25519.pub` |
| Log in to the VM | `ssh -i ~/.ssh/<project>/id_ed25519 -o IdentitiesOnly=yes deploy@<vm address>` |
| Run one command on the VM | The same, with `'<command>'` after the address |
| Forget a VM's identity, after it was deleted and its address is reused | `ssh-keygen -R <vm address>` |

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
(`~/.platformio/penv/bin/platformio`); `<repo>` is the repository's folder.

| Purpose | Command |
|---------|---------|
| Build the firmware | `<platformio> run --project-dir <repo>` |
| Build it and upload it to the board | `<platformio> run --project-dir <repo> --target upload` |
| Run the unit tests on the PC | `<platformio> test --project-dir <repo> --environment native` |

## Git

| Purpose | Command |
|---------|---------|
| Switch on the pre-commit hook that refuses secrets and checks the shell scripts, once per clone | `git config core.hooksPath .githooks` |
| Check every shell script with ShellCheck, as the hook does for the staged ones (from the repository's root) | `shellcheck --shell=bash --external-sources --source-path=scripts $(git ls-files '*.sh')` |

## See also

[set-up-and-test.md](../how-to/set-up-and-test.md) · [testing.md](testing.md) · [decision_logs.md](../project-design/decision_logs.md)
