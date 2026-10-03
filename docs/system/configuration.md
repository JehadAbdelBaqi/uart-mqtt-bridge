# Configuration

What a project supplies to use the bridge, and how the bridge is versioned.

[← README](../../README.md)

## One codebase, per-project settings

```
 bridge repo (same for everyone)        supplied per project
 ───────────────────────────────        ────────────────────
 firmware source                        config header (committed)
 scripts/ for the test broker           include/secrets/ (never committed)
              │                                  │
              └────────────► build ◄─────────────┘
                               │
                               ▼
                    firmware for that project
```

- **The bridge repo holds no project's settings.**
- **Each project supplies a config header and the files in `include/secrets/`.**
- **Settings are compiled in.** Changing a setting means rebuilding and
  flashing.

## Config header

Not secret; committed in the project's repo.

| Setting | What it sets |
|---------|--------------|
| Routing table | First letter of a line → the MQTT topic it is published to |
| Subscribed topics | Topics whose messages are written down the UART |
| Time line | On or off; the line's prefix; how often it is sent |
| Link-status line | On or off; the line's prefix |

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

With the local test broker, `scripts/setup-certs.sh` writes all five files on
every run from the values in `scripts/config.sh` (see
[scripts/README.md](../../scripts/README.md)). For any other broker, the files
are put in place under the same names.

## Versions

- **Working versions of the bridge are marked with git tags.**
- **A project names the tag it was built against** in its own docs: "build
  `uart-mqtt-bridge` at this tag with these files".
- **A project stays on its tag** until it is deliberately moved to a newer one,
  so later changes to the bridge do not affect it.

## See also

[architecture.md](architecture.md) · [decisions.md](../project-design/decisions.md)
