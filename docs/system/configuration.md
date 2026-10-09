# Configuration

The bridge's settings: the config header, the board header and the secrets
files.

[← README](../../README.md)

## Settings are compiled in

```
 include/config.h     UART link settings, routing table,    committed
                      subscribed topics, the answer
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
| `FIRMWARE_CONFIG` | Its UART link settings, routing table, subscribed topics, what the answer looks like, line logging | `include/config.h` |
| `BUILD_CONFIG` | The steps `e2e.sh` runs | `scripts/build-config.sh` |

```
cd <bridge>/scripts
SCRIPT_CONFIG=<path> FIRMWARE_CONFIG=<path> BUILD_CONFIG=<path> bash e2e.sh
```

The project's firmware config has to set everything the bridge's own config
sets (see [Config header](#config-header)), with its own values: the link, the
routes, and what a handshake line and an answer look like in its protocol.
The project states them because its MCU is the other end of the link.

A project with an MCU also sets one value the bridge's own config leaves out:

| Setting | What it sets |
|---------|--------------|
| `MCU_QUIET_LIMIT_MS` | How long with no line from the MCU before the LED shows it as quiet. After as long again, the MCU counts as gone. Longer than the longest silence the MCU keeps in ordinary running |

With it set, the bridge shows on its LED whether the MCU is there (see
[architecture.md](architecture.md#handshake-with-the-mcu)). Left out, the
bridge expects no MCU and watches for none.

`BROKER_CONFIRM_LIMIT_MS` has to be shorter than the time the MCU itself waits
for an answer, so that the bridge's answer always arrives first. A project
works it out from its MCU's own limit.

The firmware holds no fallback for a setting it needs: a build stops with an
error naming the one that is missing.

The certificates are still made by the bridge's scripts, under the project
name the script config gives. How the scripts use these files is in
[scripts/README.md](../../scripts/README.md).

## Config header

Not secret; committed.

| Setting | What it sets |
|---------|--------------|
| `UART_BAUD` | Baud rate of the UART link to the MCU. The frame is always 8 data bits, no parity, 1 stop bit |
| `LINE_MAX_LEN` | Longest line in either direction, in characters, not counting the line end |
| `LINE_END_CHARACTERS` | The characters that end a line from the MCU, as text. The bridge ends the lines it sends with the first |
| `LINE_SKIPPED_CHARACTERS` | The characters left out of a line from the MCU, as text |
| Routing table (`UPLINK_ROUTES`) | First letter of a line → the MQTT topic it is published to |
| Subscribed topics (`DOWNLINK_TOPICS`) | Topics whose messages are written down the UART |
| `HANDSHAKE_LETTER` | The letter a handshake line from the MCU starts with. The bridge answers such a line itself and never publishes it |
| `ANSWER_FORMAT` | What the bridge's answer to a line looks like: a text with a `%s` for the start of the line answered, then a `%s` for the status |
| `ANSWER_FIELD_COUNT` | How many fields of the line make its start, counting its letter as one |
| `FIELD_SEPARATOR` | The character between the fields of a line |
| `STATUS_TEXT_OK` | The status when the broker has the line; for a handshake, when Wi-Fi is up and the broker is connected |
| `STATUS_TEXT_CON_ERR_WIFI` | The status when there is no Wi-Fi |
| `STATUS_TEXT_CON_ERR_BROKER` | The status when Wi-Fi is up and the broker is not connected |
| `STATUS_TEXT_ERR_BROKER` | The status when the broker did not confirm a line in time |
| `STATUS_TEXT_ANS_ERR_LONG` | Has to be set; the bridge does not send it yet |
| `BROKER_CONFIRM_LIMIT_MS` | How long the bridge waits for the broker to confirm a line before it answers that it has not gone through |
| `LOG_LINES` | Line logging: on (`1`) or off (`0`, or left out) |
| `DUMMY_LINE_INTERVAL_MS` | Testing only: how often the bridge writes a test line to its own UART; off when `0` or left out |

With the bridge's own values, a line and its answer look like this:

```
 W,5,1790000000,2140     the MCU's line
 L,W,5,ok                ANSWER_FORMAT "L,%s,%s", with the first 2 fields and the status
```

Example routing table and subscriptions:

| Direction | First letter / topic |
|-----------|----------------------|
| Up | `W` → `weather/nucleo-01/readings` |
| Up | `R` → `weather/nucleo-01/replies` |
| Down | `weather/nucleo-01/acks` |
| Down | `weather/nucleo-01/commands` |

The file is `include/config.h`. It holds values only; the two lists of the
example above look like this:

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
tested with: 115200 baud, a line limit of 127, `\n` ending a line and `\r`
left out, `T` → `bridge/test/up`, `bridge/test/down` subscribed to, `H` for
the handshake and `L,%s,%s` for the answer.

Both ends of the link have to use the same baud rate, line limit and line end.

It also sets `DUMMY_LINE_INTERVAL_MS`, which switches on the dummy data source
used for testing without an MCU: at that interval the bridge writes a numbered
line to its own UART, which a jumper from TX to RX brings back in. For use with
a real MCU, leave it out or set it to `0`; the bridge then writes nothing of
its own.

`LOG_LINES` switches line logging on (`1`) or off (`0`, or left out). When it
is on, the log shows every line received from the MCU as `message: up: <line>`
and every line written to it as `message: down: <line>`. Warnings, about a
line with no topic for instance, are shown whatever it is set to.

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
