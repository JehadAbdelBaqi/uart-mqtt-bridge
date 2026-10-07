# Configuration

The bridge's settings: the config header, the board header and the secrets
files.

[← README](../../README.md)

## Settings are compiled in

```
 include/config.h     UART link settings, routing table,    committed
                      subscribed topics
 include/board.h      pins                                  committed
 include/secrets/     Wi-Fi, broker address, certificates   never committed
          │
          ▼
        build ──► firmware
```

Changing a setting means rebuilding and flashing.

The firmware does not include `config.h` by name. It includes
`include/generated/config_in_use.h`, which `scripts/steps/build-and-upload.sh`
writes before every build, holding one `#include` of the config file for that
build: this repository's `include/config.h`, or a project's.

## From a project

A project that uses the bridge edits nothing here. It keeps its own config
files in its own repository and names them, as full paths, when it runs the
scripts from `scripts/`:

| Variable | The project's file | In place of |
|----------|--------------------|-------------|
| `SCRIPT_CONFIG` | Project name, VM name, port, Wi-Fi | `scripts/config.sh` |
| `FIRMWARE_CONFIG` | Its UART link settings, routing table, subscribed topics, line logging | `include/config.h` |
| `BUILD_CONFIG` | The steps `e2e.sh` runs | `scripts/build-config.sh` |

```
cd <bridge>/scripts
SCRIPT_CONFIG=<path> FIRMWARE_CONFIG=<path> BUILD_CONFIG=<path> bash e2e.sh
```

The project's firmware config has to set the two values of the UART link, as
every config does (see [Config header](#config-header)): `UART_BAUD` and
`LINE_MAX_LEN`. The project states them because its MCU is the other end of
the link.

Built this way, the firmware also runs the handshake with the MCU (see
[architecture.md](architecture.md#handshake-with-the-mcu)), and the project's
firmware config has to set its three values:

| Setting | What it sets |
|---------|--------------|
| `HANDSHAKE_REQUEST_INTERVAL_MS` | How often `H,request` is repeated while it goes unanswered |
| `HANDSHAKE_QUIET_LIMIT_MS` | Once connected: how long with no line from the MCU before the bridge asks |
| `HANDSHAKE_MISSED_LIMIT` | Unanswered requests in a row before the connection counts as lost |

The firmware holds no fallback for any of these: a build stops with an
error if one is missing. The bridge's own `include/config.h` has none, because
a bridge standing alone runs no handshake.

The certificates are still made by the bridge's scripts, under the project
name the script config gives. How the scripts use these files is in
[scripts/README.md](../../scripts/README.md).

## Config header

Not secret; committed.

| Setting | What it sets |
|---------|--------------|
| `UART_BAUD` | Baud rate of the UART link to the MCU. The frame is always 8 data bits, no parity, 1 stop bit |
| `LINE_MAX_LEN` | Longest line in either direction, in characters, not counting the `\n` |
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
tested with: 115200 baud, a line limit of 127, `T` → `bridge/test/up`, and
`bridge/test/down` subscribed to.

`UART_BAUD` and `LINE_MAX_LEN` have to be in every config, and both ends of
the link have to use the same values. The firmware holds no fallback for
them: a build stops with an error naming the one that is missing.

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
port and pins of the link to the MCU. It is set for the Genesis Mini; change
it to run the bridge on a different board.

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

[architecture.md](architecture.md) · [set-up-and-test.md](../how-to/set-up-and-test.md) · [decision_logs.md](../project-design/decision_logs.md)
