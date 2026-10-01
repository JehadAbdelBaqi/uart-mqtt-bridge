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
                  config + secrets headers
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

The bridge's code is the same for every project. A project supplies two
headers, kept in its own repository:

| File | Holds |
|------|-------|
| Config header | Routing table, subscribed topics, UART settings, optional features |
| Secrets header | Wi-Fi credentials, broker address and port, certificates |

The firmware is built with those two files and flashed to the ESP32-S3. A
project names the bridge version it was built against by git tag, so later
bridge changes do not affect it. See
[docs/system/configuration.md](docs/system/configuration.md).

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
| [docs/project-design/decisions.md](docs/project-design/decisions.md) | The design decisions and why |
