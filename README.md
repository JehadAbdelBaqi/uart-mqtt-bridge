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
                config header + include/secrets/
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
- **Connects to any MQTT broker over TLS** with a client certificate — a local
  Mosquitto and AWS IoT Core use the same code.
- **Publishes and subscribes at QoS 1.**
- **Reconnects in the background.** Wi-Fi and MQTT recover on their own while
  the UART keeps being read.
- **Keeps no backlog.** A line that arrives while the link is down is dropped;
  the board owns any storing and resending.
- **Sends the time** (optional): fetches it via NTP and writes it down the UART
  on connect and at a set interval.
- **Reports the link state** (optional): writes a line down the UART when the
  link goes up or down.
- **Shows its state** on the on-board RGB LED.
- **Protects its secrets** with flash encryption and secure boot.

## Using it in a project

The bridge's code is the same for every project. A project supplies these
files, kept in its own repository:

| File | Holds |
|------|-------|
| Config header | Routing table, subscribed topics, optional features |
| `include/secrets/wifi.h` | `WIFI_SSID` and `WIFI_PASSWORD` (strings): the network the bridge joins |
| `include/secrets/broker.h` | `BROKER_ADDRESS` (string) and `BROKER_PORT` (number) |
| `include/secrets/ca.crt` | The CA certificate that signed the broker's certificate, in PEM format |
| `include/secrets/client.crt` | The bridge's client certificate, signed by a CA the broker trusts, in PEM format |
| `include/secrets/client.key` | The private key of the client certificate, in PEM format |

With the local test broker, [`scripts/setup-certs.sh`](scripts/README.md)
writes all five files in `include/secrets/` on every run, from the values in
`scripts/config.sh`. For any other broker, put files with that content in
place under the same names.

The pins and UART settings for the Genesis Mini are in `include/board.h`;
change that file to run the bridge on a different board.

The firmware is built with those files and flashed to the ESP32-S3. A
project names the bridge version it was built against by git tag, so later
bridge changes do not affect it. See
[docs/system/configuration.md](docs/system/configuration.md).

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

## Components

| Component | Job |
|-----------|-----|
| Genesis Mini (ESP32-S3) | Runs the bridge firmware |
| ESP-IDF, built with PlatformIO | Framework and build system |
| UART line reader | Collects characters from the MCU into lines |
| Routing table | First letter of a line → MQTT topic |
| Wi-Fi station | Joins the network, reconnects in the background |
| MQTT client over TLS | Publishes lines, receives messages on subscribed topics |
| Time line / link-status line | Optional lines the bridge writes to the MCU |
| Status LED | Link state and traffic |
| GitHub Actions | Compiles the firmware on every push |

## Documentation

| Doc | Covers |
|-----|--------|
| [docs/system/architecture.md](docs/system/architecture.md) | The parts of the firmware, how a line travels up and a message travels down, behaviour when the link is down |
| [docs/system/configuration.md](docs/system/configuration.md) | What a project supplies, and how the bridge is versioned |
| [docs/system/testing.md](docs/system/testing.md) | Manual checks on the bench: Wi-Fi, the broker connection, recovery, the secrets guard |
| [docs/project-design/decisions.md](docs/project-design/decisions.md) | The design decisions and why |
