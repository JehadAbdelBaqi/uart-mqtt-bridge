# Architecture

The parts of the bridge, how a line travels up and a message travels down, how
the MCU is answered, and what happens when the link is down.

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
 UP      MCU ─line─► UART link ─► line framing ─► message routing ─► upstream messaging ─► MQTT link ─► broker
                     bytes        bytes to a      a handshake is     first letter → topic
                                  whole line      answered here      payload = the whole line

 ANSWER  MCU ◄─line─ UART link ◄─ downstream messaging ◄─ upstream messaging ◄─ MQTT link ◄─ broker
                                  the answer, with                              the broker confirms,
                                  a status                                      or the time runs out

 DOWN    MCU ◄─line─ UART link ◄──────────────────────── upstream messaging ◄─ MQTT link ◄─ broker
                                                          message on a subscribed topic
                                                          line = the whole payload
```

### Up: a line from the MCU

1. The UART link hands over the bytes that arrive. The line framing collects
   them into a line: a character from the config's list of line ends finishes
   it, and one from its list of skipped characters is left out.
2. A line that starts with the handshake's letter is answered by the bridge
   and goes no further (see [Handshake](#handshake-with-the-mcu)).
3. For any other line, the **first letter only** is looked up in the routing
   table to find its topic.
4. The whole line, unchanged, is published to that topic at QoS 1, and the
   bridge keeps it until the broker confirms it (see
   [Answers](#answers-to-the-mcu)).
5. A line whose first letter is not in the table is not published, and is
   logged. An empty line is ignored.

Example with a routing table of `W` → `weather/nucleo-01/readings`:

```
 UART in:   W,42,1790000000,2140,101325,4520
 Published: topic   = weather/nucleo-01/readings
            payload = W,42,1790000000,2140,101325,4520
```

### Down: a message from the broker

1. Every time it connects to the broker, the bridge subscribes, at QoS 1, to
   the topics listed in the config.
2. Each message that arrives is written to the UART as one line, unchanged,
   with the line end added.
3. An empty message, and one longer than the limit, is not sent down.

### One line, one message

The limit is the same in both directions: a message is one line of at most
`LINE_MAX_LEN` characters, a value in the config (127 as the bridge is tested).
A project that needs more splits its data across several lines in its own
format.

Keeping a line within the limit is the sender's job. The bridge does not check
a line from the MCU: bytes past the limit are not kept, so a longer line goes
up cut short. A message from the broker is checked, because anything can be
published there.

### Pass-through

The bridge never parses, validates or rebuilds a line. It reads the first
letter, to find the topic and to recognise a handshake, and it copies the
start of a line into its answer without knowing what the fields mean. The
message format is defined entirely by the project using the bridge.

## Answers to the MCU

The MCU cannot tell what became of a line once it has left its UART. So the
bridge answers every line it is given, with a line of its own that carries a
status.

```
 W,5,1790000000,2140   up        the MCU's line
 L,W,5,ok              down      the broker has it
```

- **What the answer looks like is the config's.** Its layout
  (`ANSWER_FORMAT`), how much of the line it names (`ANSWER_FIELD_COUNT`
  fields, cut at `FIELD_SEPARATOR`) and the text of each status are settings.
  The examples here use the values the bridge is tested with.
- **The bridge fills in two things:** the start of the line it answers, copied
  as it is written, and its status.
- **One line is held at a time.** The MCU sends a line and waits for its
  answer before the next. A line sent while another is held takes its place.

| What happens | Answer |
|--------------|--------|
| The broker confirms the line | `ok` |
| The broker does not confirm it within `BROKER_CONFIRM_LIMIT_MS` | `err_br`, and the bridge drops its connection to the broker so that it is made afresh |
| Wi-Fi is down when the line arrives | `con_err_w`, at once; the line is not published |
| Wi-Fi is up and the broker is not connected | `con_err_br`, at once; the line is not published |

- **The broker confirms a message by a number**, the ID the MQTT client gave
  it, and never by its content. The bridge keeps the line beside that ID so it
  knows which line the confirmation is for.
- **The time limit is a one-shot timer**, started when a line is published and
  stopped when the broker confirms it.
- **A line with no topic gets no answer.** It is logged and goes nowhere.

## Handshake with the MCU

The bridge and its MCU are two boards joined by wires, which can fail while
Wi-Fi and the broker are fine.

```
 H,request             up        the MCU: "are you there?"
 L,H,request,ok        down      the bridge: "I am here, and upstream is connected"
```

- **Only the MCU asks.** The bridge answers and never asks.
- **The answer is the same kind as any other**, and its status says how far
  the bridge can reach: `ok`, `con_err_w` or `con_err_br`. One exchange tells
  the MCU that the bridge is there and whether its lines can be published.
- **Never published.** A line starting with `HANDSHAKE_LETTER` stops at the
  bridge.
- **The bridge works out whether the MCU is there from its silence**, when the
  config sets `MCU_QUIET_LIMIT_MS`:

| State | LED |
|-------|-----|
| No handshake answered yet | Red, blinking |
| Handshake answered, lines arriving | Wi-Fi and broker state |
| Nothing heard for the quiet limit | Green, flashing fast |
| Nothing heard for twice the quiet limit: the MCU counts as gone | Red, blinking |

- **Any line shows the MCU is still there.** Only an answered handshake makes
  the connection, and only one brings back an MCU that has gone.
- **It covers the UART only.** Lines pass whether or not the connection is
  made.
- **A config that does not set the limit expects no MCU.** Nothing is watched,
  and the LED shows only Wi-Fi and the broker. That is how the bridge's own
  config is.

## Parts of the firmware

| Part | Job |
|------|-----|
| UART link | Sets up the UART to the MCU; reads bytes from it and writes text to it |
| Wi-Fi link | Joins the network; reconnects in the background; says whether Wi-Fi is up |
| MQTT link | TLS to the broker with a client certificate, reconnecting; publishes, subscribes, and reports each connection, message and confirmation; says whether the broker is connected |
| Status LED | On-board RGB LED: shows the link state |
| Line framing | Bytes from the MCU to a whole line |
| Message routing | A handshake is answered, every other line is sent up |
| Upstream messaging | The topic for a line, the one line held for the broker and its timer, messages from the broker passed down |
| Downstream messaging | The task that reads the UART, the bridge's answer to a line, the handshake's answer |
| Status | Which status is the case, and the text of each |
| Downstream connection | Whether the MCU is there, shown on the LED |
| Dummy data source | Testing only: writes numbered lines to the bridge's own UART in place of an MCU |

The first four are **modules**: they operate a part of the board or a
connection and know nothing of what the bridge does with it. The rest is the
**application**: what the bridge does with a message. A module calls nothing
in the application; where it has something to report, it calls a function it
was handed.

The firmware is written on ESP-IDF and built with PlatformIO.

```
 src/main.c                     starts NVS, the network layer and the event loop, then the modules and the application
 components/modules/
   led/  uart_link/  wifi_link/  mqtt_link/               one folder per module: its .c and .h
 src/app/
   messaging/                   upstream_messaging, downstream_messaging, message_routing, line_framing, helpers
   status.c                     the statuses and their texts
   downstream_connection.c      whether the MCU is there
   dummy_source.c               test lines in place of an MCU; off unless the config switches it on
 include/app/                   the application's headers, following src/app/ folder for folder
 include/board.h                how the board is wired (pins, UART)
 include/config.h               the bridge's own config
 include/secrets/               Wi-Fi, broker address and certificates (generated, not committed)
 test/                          unit tests for the application, run on the PC
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
- **Nothing is kept for later.** A line that arrives while Wi-Fi or the broker
  is down is not published and not held: the MCU is answered at once with
  which of the two is down, and keeps the line itself.
- **The MCU learns that upstream is back by asking.** The answer to its next
  handshake request says `ok` again.

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
 broker ─► MQTT link ─► upstream messaging ─► UART TX ─┐ jumper
 broker ◄─ MQTT link ◄─ upstream messaging ◄─ UART RX ◄┘
```

The bridge's answers come back in through the jumper too. Their letter has no
topic in the bridge's own config, so they are logged as having no topic and go
no further.

It is off when the interval is `0` or not set. With it off, the bridge only
ever writes to the UART what arrived from the broker and its own answers. The
end-to-end test built on it, and the unit tests that run on the PC, are in
[testing.md](testing.md).

## See also

[configuration.md](configuration.md) · [testing.md](testing.md) · [decision_logs.md](../project-design/decision_logs.md) · [risks.md](../project-design/risks.md)
