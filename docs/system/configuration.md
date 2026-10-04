# Configuration

The bridge's settings: the config header, the board header and the secrets
files.

[← README](../../README.md)

## Settings are compiled in

```
 include/config.h     routing table, subscribed topics      committed
 include/board.h      pins, UART settings                   committed
 include/secrets/     Wi-Fi, broker address, certificates   never committed
          │
          ▼
        build ──► firmware
```

Changing a setting means rebuilding and flashing.

## Config header

Not secret; committed.

| Setting | What it sets |
|---------|--------------|
| Routing table | First letter of a line → the MQTT topic it is published to |
| Subscribed topics | Topics whose messages are written down the UART |
| Dummy data source | Testing only: how often the bridge writes a test line to its own UART; off when `0` or left out |

Example routing table and subscriptions:

| Direction | First letter / topic |
|-----------|----------------------|
| Up | `W` → `weather/nucleo-01/readings` |
| Up | `R` → `weather/nucleo-01/replies` |
| Down | `weather/nucleo-01/acks` |
| Down | `weather/nucleo-01/commands` |

The file is `include/config.h`. It holds values only, written as two lists; the
example above looks like this:

```c
// Uplink: a line's first letter picks the topic it is published to
#define UPLINK_ROUTES \
    { 'W', "weather/nucleo-01/readings" }, \
    { 'R', "weather/nucleo-01/replies" }

// Downlink: every message on these topics is written to the MCU as one line
#define DOWNLINK_TOPICS \
    "weather/nucleo-01/acks", \
    "weather/nucleo-01/commands"
```

Every line of a list but the last ends in `, \`.

The `include/config.h` in the bridge repository holds the values the bridge is
tested with: `T` → `bridge/test/up`, and `bridge/test/down` subscribed to.

It also sets `DUMMY_LINE_INTERVAL_MS`, which switches on the dummy data source
used for testing without an MCU: at that interval the bridge writes a numbered
line to its own UART, which a jumper from TX to RX brings back in. For use with
a real MCU, leave it out or set it to `0`; the bridge then writes nothing of
its own.

`LOG_LINES` switches line logging on (`1`) or off (`0`, or left out). When it
is on, the log shows every line received from the MCU as `uart: up: <line>`
and every line written to it as `uart: down: <line>`. Warnings about a dropped
or unroutable line are shown whatever it is set to.

## Board

`include/board.h` holds how the bridge is wired: the LED pin, and the UART
port, pins and baud rate of the link to the MCU (115200, 8N1). It is set for
the Genesis Mini; change it to run the bridge on a different board.

## `include/secrets/`

Never committed: the folder is gitignored, and a pre-commit hook refuses any
file from it.

| File | Holds |
|------|-------|
| `wifi.h` | `WIFI_SSID` and `WIFI_PASSWORD` (strings): the network the bridge joins |
| `broker.h` | `BROKER_ADDRESS` (string) and `BROKER_PORT` (number) |
| `ca.crt` | The CA certificate the broker's own certificate is checked against (PEM) |
| `client.crt` | The bridge's identity, presented to the broker (PEM) |
| `client.key` | The private key that proves the client certificate belongs to this bridge (PEM) |

With the local test broker, `scripts/steps/setup-certs.sh` writes all five files on
every run from the values in `scripts/config.sh` (see
[set-up-and-test.md](../how-to/set-up-and-test.md)). For any other broker, the files
are put in place under the same names.

## See also

[architecture.md](architecture.md) · [set-up-and-test.md](../how-to/set-up-and-test.md) · [decisions.md](../project-design/decisions.md)
