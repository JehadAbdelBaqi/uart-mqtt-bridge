# UART-MQTT Bridge

ESP32-S3 firmware that gives any microcontroller an MQTT connection over a
UART. The microcontroller sends plain text lines; the bridge publishes them to
an MQTT broker over Wi-Fi and TLS, and writes incoming MQTT messages back down
the UART. Messages pass through unchanged.

## Overview

```
 [MCU] ──UART lines──► [ESP32-S3 bridge] ──MQTT over TLS──► [broker]
       ◄──────────────                   ◄─────────────────
                              ▲
              include/config.h + include/secrets/
                  from the project using it
```

## Scope

- **UART on one side, MQTT on the other.** That is the whole interface.
- **Wi-Fi is only the transport to the broker.** The bridge is not a general
  Wi-Fi adapter: it makes no HTTP requests and exposes no sockets to the MCU.
- **An extension of the board it is wired to**, not a device of its own. When
  the bridge is offline, the board is offline.

## What it does

- **Passes lines through as-is.** The bridge never parses or converts a
  message; the format belongs to the project using it.
- **Routes by first letter.** A table in the project's config maps the first
  letter of a line to an MQTT topic. The whole line is the payload.
- **Brings messages down.** Every message on a subscribed topic is written to
  the UART as one line.
- **One line is at most 127 characters**, in either direction. Anything longer
  is dropped whole, never cut short.
- **Connects to any MQTT broker over TLS** with a client certificate — a local
  Mosquitto and AWS IoT Core use the same code.
- **Publishes and subscribes at QoS 1.**
- **Reconnects in the background.** Wi-Fi and MQTT recover on their own while
  the UART keeps being read, and the bridge subscribes again on every
  connection.
- **Keeps no backlog.** A line that arrives while the link is down is dropped;
  the board owns any storing and resending.
- **Sends the time** (optional): fetches it via NTP and writes it down the UART
  on connect and at a set interval.
- **Reports the link state** (optional): writes a line down the UART when the
  link goes up or down.
- **Shows its state** on the on-board RGB LED.
- **Protects its secrets** with flash encryption and secure boot.
- **Tests itself without an MCU**: a dummy data source in the firmware stands
  in for one, and one script proves the whole chain end to end.

## Getting started

1. Get what is needed: the board, a jumper wire, and the software listed in
   [docs/system/resources.md](docs/system/resources.md).
2. Switch on the secrets guard, once per clone:
   ```
   git config core.hooksPath .githooks
   ```
3. Copy `scripts/config.example.sh` to `scripts/config.sh` and fill it in
   (Wi-Fi network, adapter names).
4. Plug the board in, jumper TX to RX (GPIO7 to GPIO6), and from `scripts/`, in
   a Git Bash opened as administrator:
   ```
   bash e2e.sh
   ```

That creates the test broker, builds and uploads the firmware, and checks
messages in both directions. The steps in full are in
[docs/how-to/set-up-and-test.md](docs/how-to/set-up-and-test.md).

## Using it in a project

The bridge's code is the same for every project. A project supplies these
files, kept in its own repository:

| File | Holds |
|------|-------|
| `include/config.h` | Routing table, subscribed topics, optional features |
| `include/secrets/wifi.h` | `WIFI_SSID` and `WIFI_PASSWORD` (strings): the network the bridge joins |
| `include/secrets/broker.h` | `BROKER_ADDRESS` (string) and `BROKER_PORT` (number) |
| `include/secrets/ca.crt` | The CA certificate that signed the broker's certificate, in PEM format |
| `include/secrets/client.crt` | The bridge's client certificate, signed by a CA the broker trusts, in PEM format |
| `include/secrets/client.key` | The private key of the client certificate, in PEM format |

With the local test broker, [`scripts/steps/setup-certs.sh`](docs/how-to/set-up-and-test.md)
writes all five files in `include/secrets/` on every run, from the values in
`scripts/config.sh`. For any other broker, put files with that content in
place under the same names.

The `include/config.h` in this repository holds the values the bridge is
tested with: `T` → `bridge/test/up`, `bridge/test/down` subscribed to, and the
dummy data source switched on.

The pins and UART settings for the Genesis Mini are in `include/board.h`;
change that file to run the bridge on a different board.

The firmware is built with those files and flashed to the ESP32-S3. A
project names the bridge version it was built against by git tag, so later
bridge changes do not affect it. See
[docs/system/configuration.md](docs/system/configuration.md).

## Testing it

The bridge is tested on its own, with no MCU: a dummy data source in the
firmware writes numbered lines to the bridge's UART, and a jumper from TX to RX
brings them back in. The broker is a local Mosquitto in a virtual machine.

`scripts/e2e.sh` runs five steps, each a script of its own in `scripts/steps/`:

| Step | Script | Job |
|------|--------|-----|
| 1 | `nuke.sh` | Deletes everything the other steps created |
| 2 | `create-vm.sh` | Creates the VM with Mosquitto |
| 3 | `setup-certs.sh` | Certificates, the firmware's files in `include/secrets/`, the PC's port rule and firewall rule, a TLS connection test |
| 4 | `build-and-upload.sh` | Builds the firmware and uploads it to the board |
| 5 | `test-bridge.sh` | Checks through the broker that a line comes up from the bridge and a message sent down comes back |

Everything is run from the `scripts/` folder: `bash e2e.sh` for all of it,
`bash steps/<script>` for one step. See
[docs/how-to/set-up-and-test.md](docs/how-to/set-up-and-test.md) for how to
run them and [docs/system/testing.md](docs/system/testing.md) for the checks
done by hand.

## Status LED

The on-board RGB LED shows what the bridge is doing.

| LED | Meaning |
|-----|---------|
| Red, amber, green, blue in turn — two quick passes, one slow (about 6 s), then off | Starting up |
| Amber, flashing fast | Connecting to Wi-Fi |
| Red | Not connected to Wi-Fi; tries again every 5 s |
| Green, blinking once a second | On Wi-Fi, not connected to the broker |
| Solid green | On Wi-Fi and connected to the broker |

## Keeping secrets out of the repo

`include/secrets/` and `scripts/config.sh` are gitignored. As a second guard, a pre-commit hook in `.githooks/` refuses any
commit that would add a secrets file, `scripts/config.sh`, or a private key.
Switch it on once after cloning:

```
git config core.hooksPath .githooks
```

## Repository layout

```
 src/main.c                 starts the shared services, then each module
 components/modules/        the firmware's modules, one folder each:
   uart_link/                 the UART and the line reader
   router/                    routing table, publish, subscribe, messages down to the UART
   mqtt_link/                 the TLS connection to the broker
   wifi_link/                 the Wi-Fi connection
   led/                       the status LED
   dummy_source/              test lines in place of an MCU
 include/
   board.h                    how the board is wired
   config.h                   routing table, subscribed topics, dummy data source
   secrets/                   Wi-Fi, broker address, certificates (generated, not committed)
 scripts/
   e2e.sh                     everything, from nothing to a tested bridge
   steps/                     the five steps e2e.sh runs, each runnable by itself
   helpers/                   the functions the scripts are built from
   config.example.sh          template for your own config.sh
 docs/                      see Documentation below
```

## Components

| Component | Job |
|-----------|-----|
| Genesis Mini (ESP32-S3) | Runs the bridge firmware |
| ESP-IDF, built with PlatformIO | Framework and build system |
| UART line reader | Collects characters from the MCU into lines |
| Router | First letter of a line → MQTT topic; publishes lines, subscribes, writes received messages to the UART |
| Wi-Fi station | Joins the network, reconnects in the background |
| MQTT link | Keeps the TLS connection to the broker |
| Time line / link-status line | Optional lines the bridge writes to the MCU |
| Dummy data source | Test lines in place of an MCU; off unless the config switches it on |
| Status LED | Link state and traffic |
| Mosquitto in a Multipass VM | Local test broker with TLS and client certificates |
| Scripts | The test broker, building and uploading, the end-to-end test |
| GitHub Actions | Compiles the firmware on every push |

## Documentation

| Doc | Covers |
|-----|--------|
| **How to** | |
| [docs/how-to/set-up-and-test.md](docs/how-to/set-up-and-test.md) | How to use the scripts: the test broker, building and uploading, the end-to-end test |
| **The system** | |
| [docs/system/architecture.md](docs/system/architecture.md) | The parts of the firmware, how a line travels up and a message travels down, behaviour when the link is down |
| [docs/system/configuration.md](docs/system/configuration.md) | What a project supplies, and how the bridge is versioned |
| [docs/system/testing.md](docs/system/testing.md) | The end-to-end test, and checks by hand on the bench: Wi-Fi, the broker connection, messages in both directions, recovery, the secrets guard |
| [docs/system/commands.md](docs/system/commands.md) | Every command the scripts run, by tool, with its purpose — for running one by hand |
| [docs/system/resources.md](docs/system/resources.md) | The hardware, software and reference documentation needed |
| **Project design** | |
| [docs/project-design/decisions.md](docs/project-design/decisions.md) | The design decisions and why |
| [docs/project-design/risks.md](docs/project-design/risks.md) | What could go wrong, and what is done about each |
