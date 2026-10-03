# Architecture

The parts of the bridge, how a line travels up and a message travels down, and
what happens when the link is down.

[← README](../../README.md)

## Where it sits

```
 [MCU] ──UART──► [ESP32-S3 bridge] ──Wi-Fi──► [router] ──TLS──► [MQTT broker]
       ◄───────                    ◄────────           ◄──────
```

- **The MCU speaks plain text lines over UART.** Wi-Fi, TLS and MQTT live in
  the bridge; the MCU never drives them.
- **The bridge and the MCU act as one device.** The bridge is an extension of
  the board it is wired to. Bridge offline means the board is offline.

## Data flow

```
 UP    MCU ─UART line─► line reader ─► routing table ─► MQTT publish ─► broker
                                       first letter → topic
                                       payload = the whole line

 DOWN  MCU ◄─UART line─ bridge ◄─ MQTT message ◄─ broker
                                  on a subscribed topic
                                  line = the whole payload
```

### Up: a line from the MCU

1. The line reader collects characters until a line ending (`\n`, or `\r\n`).
2. The bridge looks at the **first letter only** and finds its topic in the
   routing table.
3. The whole line, unchanged, is published to that topic at QoS 1.
4. A line whose first letter is not in the table is not published, and is logged.
5. An empty line is ignored. A line longer than 127 characters is dropped whole
   and logged.

Example with a routing table of `W` → `weather/nucleo-01/readings`:

```
 UART in:   W,42,1790000000,2140,101325,4520
 Published: topic   = weather/nucleo-01/readings
            payload = W,42,1790000000,2140,101325,4520
```

### Down: a message from the broker

1. Every time it connects to the broker, the bridge subscribes, at QoS 1, to
   the topics listed in the project's config.
2. Each message that arrives is written to the UART as one line, unchanged,
   with `\n` added at the end.
3. An empty message is ignored. A message longer than 127 characters, or one
   containing `\n`, is dropped whole and logged.

### One line, one message

The limit is the same in both directions: a message is one line of at most 127
characters. The MCU always receives exactly one whole line per message. A
project that needs more splits its data across several lines in its own format.

### Pass-through

The bridge never parses, validates or rebuilds a message. The first letter is
the only part of a line it reads. The message format is defined entirely by the
project using the bridge.

## Parts of the firmware

| Part | Job |
|------|-----|
| UART line reader | Reads the MCU's UART, splits it into lines |
| Router | Everything about messages: first letter → topic from the project's config, publish and subscribe at QoS 1, received messages written to the UART |
| Wi-Fi station | Joins the network; reconnects in the background |
| MQTT link | The connection only: TLS to the broker with a client certificate, reconnecting; hands its client to the router |
| Time line (optional) | Gets the time via NTP; writes it down the UART on connect and at a set interval |
| Link-status line (optional) | Writes a line down the UART when the link goes up or down |
| Status LED | On-board RGB LED: link state, a blink on traffic |

The firmware is written on ESP-IDF and built with PlatformIO.

```
 src/main.c                     starts NVS, the network layer and the event loop, then each module
 components/modules/
   led/  uart_link/  wifi_link/  mqtt_link/  router/      one folder per module: its .c and .h
   dummy_source/                                           test lines in place of an MCU; off unless the config switches it on
 include/board.h                how the board is wired (pins, UART)
 include/config.h               routing table and subscribed topics
 include/secrets/               Wi-Fi, broker address and certificates (generated, not committed)
```

## Broker connection

- **MQTT over TLS, with a client certificate.** The bridge checks the broker's
  certificate against a CA certificate, and proves its own identity with a
  client certificate and private key.
- **Any broker.** The address, port and certificates come from the files in
  `include/secrets/`, so a local Mosquitto and AWS IoT Core use the same code.

## When the link is down

- **Lines are dropped.** A line that arrives while Wi-Fi or the broker is
  unreachable is not published and not kept. The bridge holds no backlog.
- **The MCU owns reliability.** A project that cannot lose data stores it on
  the MCU and resends it; the MCU keeps working while the bridge is offline.
- **The UART keeps being read.** Wi-Fi and MQTT reconnect in the background;
  the bridge resubscribes after every reconnect.
- **The MCU can be told.** With the link-status line switched on, the bridge
  writes a line down the UART each time the link goes up or down.

## Optional lines written by the bridge

Both are off unless the project's config switches them on. With both off, the
bridge only ever writes to the UART what arrived from the broker.

| Line | When | Content |
|------|------|---------|
| Time | On connect, then at a set interval | The current time, fetched via NTP |
| Link status | Each time the link goes up or down | The new link state |

The prefix of each line, and the time line's interval, are set in the project's
config.

## Protecting the secrets

The Wi-Fi password and the client private key are compiled into the firmware.
Flash encryption keeps them unreadable on the flash chip; secure boot lets only
signed firmware run.

## See also

[configuration.md](configuration.md) · [decisions.md](../project-design/decisions.md)
