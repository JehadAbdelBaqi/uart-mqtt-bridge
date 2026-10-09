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
- **One line has a length limit**, the same in either direction and set in the
  config (127 characters as the bridge is tested). Keeping within it is the
  sender's job.
- **Answers every line.** The MCU is told what became of each line it sends:
  the broker has it, the broker did not confirm it in time, or Wi-Fi or the
  broker is down. What the answer looks like is set in the config.
- **Connects to an MQTT broker over TLS** with a client certificate.
- **Publishes and subscribes at QoS 1.**
- **Reconnects in the background.** Wi-Fi and MQTT recover on their own while
  the UART keeps being read, and the bridge subscribes again on every
  connection.
- **Shows its state** on the on-board RGB LED.
- **Answers the MCU's handshake**, with how far it can reach: one exchange
  tells the MCU that the bridge is there and whether its lines can be
  published.
- **Knows whether its MCU is there**, from how long it has been silent, shown
  on the LED.
- **Tests itself without an MCU**: unit tests on the PC for what it does with
  a message, and a dummy data source in the firmware with one script that
  proves the whole chain end to end.

## Getting started

1. Get what is needed: the board, a jumper wire, and the software listed in
   [docs/system/resources.md](docs/system/resources.md).
2. Switch on the pre-commit hook, once per clone:
   ```
   git config core.hooksPath .githooks
   ```
3. Copy `scripts/config.example.sh` to `scripts/config.sh` and fill it in
   (project name, VM name, Wi-Fi network).
4. Plug the board in, jumper TX to RX (GPIO7 to GPIO6), and from `scripts/`:
   ```
   bash e2e.sh
   ```

The scripts are written for Linux. On Windows they run in WSL through
`scripts/windows/windows.sh`, for the bridge standing alone only: see
[set-up-and-test.md](docs/how-to/set-up-and-test.md#on-windows).

That creates the test broker, builds and uploads the firmware, and checks
messages in both directions. The steps in full are in
[docs/how-to/set-up-and-test.md](docs/how-to/set-up-and-test.md).

## Using it in a project

The bridge's code is the same for every project. What changes is these files:

| File | Holds |
|------|-------|
| `include/config.h` | Baud rate, line limit and line end of the UART link, routing table and subscribed topics, what a handshake line and the bridge's answer look like |
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
tested with: `T` → `bridge/test/up`, `bridge/test/down` subscribed to, `H` for
the handshake, `L,<start of the line>,<status>` for the answer, and the dummy
data source and line logging switched on.

The pins for the Genesis Mini are in `include/board.h`;
change that file to run the bridge on a different board.

To use the bridge from a project with a real MCU, nothing in this repository
is edited. The project keeps its own three config files (script config,
firmware config with its routes and topics, and the list of steps to run) and
names them when it runs `scripts/e2e.sh`. See
[scripts/README.md](scripts/README.md) and
[docs/system/configuration.md](docs/system/configuration.md).

## Testing it

The bridge is tested on its own, with no MCU, in two ways.

**On the PC:** unit tests for what the bridge does with a message, with
stand-ins for the board. From the top of the repository, `pio test -e native`
(see [test/README.md](test/README.md)).

**On the board:** a dummy data source in the firmware writes numbered lines to
the bridge's UART, and a jumper from TX to RX brings them back in. The broker
is a local Mosquitto in a virtual machine.

`scripts/e2e.sh` runs these steps, each a script of its own in
`scripts/steps/`, switched on or off in `scripts/build-config.sh`:

| Step | Script | Job |
|------|--------|-----|
| 1 | `unit-tests.sh` | Runs the unit tests on the PC |
| 2 | `nuke.sh` | Deletes everything the other steps created |
| 3 | `clean-build.sh` | Deletes the firmware's build output |
| 4 | `create-vm.sh` | Creates the VM with Mosquitto |
| 5 | `setup-certs.sh` | Certificates, the firmware's files in `include/secrets/` |
| 6 | `set-port-rule.sh` | The PC's port rule, a TLS connection test |
| 7 | `build-and-upload.sh` | Builds the firmware and uploads it to the board |
| 8 | `test-bridge.sh` | Checks through the broker that a line comes up from the bridge and a message sent down comes back |

Everything is run from the `scripts/` folder: `bash e2e.sh` for all of it,
`bash steps/<script>` for one step. See
[docs/how-to/set-up-and-test.md](docs/how-to/set-up-and-test.md) for how to
run them and [docs/system/testing.md](docs/system/testing.md) for the checks
done by hand.

## Status LED

The on-board RGB LED shows what the bridge is doing.

| LED | Meaning |
|-----|---------|
| Red, amber, green, blue in turn — two quick passes (about 2 s), then off | Starting up |
| Amber, flashing fast | Connecting to Wi-Fi |
| Red | Not connected to Wi-Fi; tries again every 5 s |
| Green, blinking once a second | On Wi-Fi, not connected to the broker |
| Solid green | On Wi-Fi and connected to the broker |

With a config that expects an MCU (one that sets `MCU_QUIET_LIMIT_MS`), two
more states come first, whatever Wi-Fi and the broker are doing:

| LED | Meaning |
|-----|---------|
| Red, blinking once a second | The MCU has not shaken hands yet, or has been silent for twice the quiet limit and counts as gone |
| Green, flashing fast | The MCU has been silent for the quiet limit |

## Keeping secrets out of the repo

`include/secrets/` and `scripts/config.sh` are gitignored. As a second guard, a pre-commit hook in `.githooks/` refuses any
commit that would add a secrets file, `scripts/config.sh`, or a private key.
Switch it on once after cloning:

```
git config core.hooksPath .githooks
```

## Checking the shell scripts

The same hook runs [ShellCheck](https://www.shellcheck.net) on every shell
script staged for a commit, and refuses the commit if one doesn't pass. On a PC
without ShellCheck the hook says the scripts were not checked and lets the
commit through. To check them all by hand, from the repository's root:

```
shellcheck --shell=bash --external-sources --source-path=scripts $(git ls-files '*.sh')
```

## Repository layout

```
 src/main.c                 starts the shared services, then the modules and the application
 components/modules/        what operates the board and the connections, one folder each:
   uart_link/                 the UART: bytes in, text out
   mqtt_link/                 the TLS connection to the broker; publish, subscribe, receive
   wifi_link/                 the Wi-Fi connection
   led/                       the status LED
 src/app/                   what the bridge does with a message:
   messaging/                 a line up, a message down, the answer to the MCU, routing, line framing
   status.c                   the statuses and their texts
   downstream_connection.c    whether the MCU is there
   dummy_source.c             test lines in place of an MCU
 include/
   app/                       the application's headers
   board.h                    how the board is wired
   config.h                   the bridge's own config: the UART link, routes and topics, the answer, testing
   secrets/                   Wi-Fi, broker address, certificates (generated, not committed)
 test/                      unit tests for the application, run on the PC
 scripts/
   README.md                  how the scripts work, and why
   e2e.sh                     everything, from nothing to a tested bridge
   steps/                     the steps e2e.sh runs, each runnable by itself
   build-config.sh            which steps e2e.sh runs
   helpers/                   the functions the scripts are built from
   config.example.sh          template for your own config.sh
   windows/                   Windows only: start script, config and helpers for running the scripts in WSL
 docs/                      see Documentation below
```

## Components

| Component | Job |
|-----------|-----|
| Genesis Mini (ESP32-S3) | Runs the bridge firmware |
| ESP-IDF, built with PlatformIO | Framework and build system |
| UART link | Reads bytes from the MCU and writes text to it |
| Wi-Fi link | Joins the network, reconnects in the background |
| MQTT link | Keeps the TLS connection to the broker; publishes, subscribes, receives |
| Status LED | Shows the link state |
| Messaging | Bytes to a line, first letter of a line → MQTT topic, the answer to the MCU, messages from the broker written to the UART |
| Status | How far the bridge can reach, as the text an answer carries |
| Downstream connection | Whether the MCU on the UART is there |
| Dummy data source | Test lines in place of an MCU; off unless the config switches it on |
| Unit tests | What the bridge does with a message, checked on the PC |
| Mosquitto in a Multipass VM | Local test broker with TLS and client certificates |
| Scripts | The test broker, building and uploading, the end-to-end test |

## Documentation

| Doc | Covers |
|-----|--------|
| **How to** | |
| [docs/how-to/set-up-and-test.md](docs/how-to/set-up-and-test.md) | How to use the scripts: the test broker, building and uploading, the end-to-end test |
| **The system** | |
| [docs/system/architecture.md](docs/system/architecture.md) | The parts of the firmware, how a line travels up and a message travels down, how the MCU is answered, behaviour when the link is down |
| [docs/system/configuration.md](docs/system/configuration.md) | The bridge's settings: the config header, the board header, the secrets files |
| [docs/system/testing.md](docs/system/testing.md) | The end-to-end test, and checks by hand on the bench: Wi-Fi, the broker connection, messages in both directions, recovery, the secrets guard |
| [test/README.md](test/README.md) | The unit tests: what is tested on the PC, how the stand-ins for the board work, adding a test |
| [docs/system/commands.md](docs/system/commands.md) | Every command the scripts run, by tool, with its purpose — for running one by hand |
| [docs/system/resources.md](docs/system/resources.md) | The hardware, software and reference documentation needed |
| **Project design** | |
| [docs/project-design/decision_logs.md](docs/project-design/decision_logs.md) | The log of design decisions, in the order they were made, and why |
| [docs/project-design/finalised-decisions.md](docs/project-design/finalised-decisions.md) | The decisions in force today, by subject |
| [docs/project-design/risks.md](docs/project-design/risks.md) | What could go wrong, and what is done about each |

## How this was built

No vibes were coded in the making of this project.

This project was built with heavy use of AI, specifically Claude Code. I
directed the architecture and made every design decision, each one recorded
with its reasoning in [decision_logs.md](docs/project-design/decision_logs.md). Claude
was used to find information, write and refactor code under my direction, and
explain anything I didn't yet understand, so that I could review it, question
it and test it on the hardware myself.

## Licence

MIT — see [LICENSE](LICENSE). Use it, change it, build on it.
