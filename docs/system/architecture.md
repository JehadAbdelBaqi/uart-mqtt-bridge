# Architecture

The parts of the bridge, how a line travels up and a message travels down, and
what happens when the link is down.

[← README](../../README.md)

## Where it sits

```
 [MCU] ──UART──► [ESP32-S3 bridge] ──Wi-Fi──► [access point] ──TLS──► [MQTT broker]
       ◄───────                    ◄────────                 ◄──────
```

- **The MCU speaks plain text lines over UART.** Wi-Fi, TLS and MQTT live in
  the bridge; the MCU never drives them.
- **The bridge and the MCU act as one device.** The bridge is an extension of
  the board it is wired to. Bridge offline means the board is offline.

## Data flow

```
 UP    MCU ─UART line─► UART link ─► router ─► MQTT link ─► broker
                        line reader   first letter → topic
                                      payload = the whole line

 DOWN  MCU ◄─UART line─ UART link ◄─ router ◄─ MQTT link ◄─ broker
                                     message on a subscribed topic
                                     line = the whole payload
```

### Up: a line from the MCU

1. The line reader collects characters until a line ending (`\n`, or `\r\n`).
2. The router looks at the **first letter only** and finds its topic in the
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
| Status LED | On-board RGB LED: shows the link state |
| Dummy data source | Testing only: writes numbered lines to the bridge's own UART in place of an MCU |

The firmware is written on ESP-IDF and built with PlatformIO.

```
 src/main.c                     starts NVS, the network layer and the event loop, then each module
 components/modules/
   led/  uart_link/  wifi_link/  mqtt_link/  router/      one folder per module: its .c and .h
   dummy_source/                                           test lines in place of an MCU; off unless the config switches it on
 include/board.h                how the board is wired (pins, UART)
 include/config.h               routing table, subscribed topics, dummy data source, line logging
 include/secrets/               Wi-Fi, broker address and certificates (generated, not committed)
```

## Broker connection

- **MQTT over TLS, with a client certificate.** The bridge checks the broker's
  certificate against a CA certificate, and proves its own identity with a
  client certificate and private key.
- **Set by files, not code.** The address, port and certificates come from the
  files in `include/secrets/`.

## When the link is down

- **The UART keeps being read.** Wi-Fi and MQTT reconnect in the background;
  the bridge subscribes again on every connection to the broker.
- **Lines are held, then sent late.** A line read while the broker isn't
  connected is kept by the MQTT client and published once it connects. Such
  lines can arrive late and out of order, so a project puts a sequence number
  or a timestamp in its messages (see [risks.md](../project-design/risks.md)).

## Testing without an MCU

The dummy data source stands in for an MCU. At the interval set by
`DUMMY_LINE_INTERVAL_MS` in `include/config.h` it writes one line to the
bridge's own UART:

```
 T,<sequence number>,<milliseconds since start>
```

With the UART's TX pin jumpered to its RX pin, the line comes back in and
travels up like a line from a real MCU. The same jumper turns every downlink
message into an uplink line, so one message proves both directions:

```
 broker ─► MQTT link ─► router ─► UART TX ─┐ jumper
 broker ◄─ MQTT link ◄─ router ◄─ UART RX ◄┘
```

It is off when the interval is `0` or not set. With it off, the bridge only
ever writes to the UART what arrived from the broker. The end-to-end test built
on it is in [testing.md](testing.md).

## See also

[configuration.md](configuration.md) · [testing.md](testing.md) · [decision_logs.md](../project-design/decision_logs.md) · [risks.md](../project-design/risks.md)
