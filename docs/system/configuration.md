# Configuration

What a project supplies to use the bridge, and how the bridge is versioned.

[← README](../../README.md)

## One codebase, per-project settings

```
 bridge repo (same for everyone)        project's own repo
 ───────────────────────────────        ──────────────────
 firmware source                        config header
 example config + secrets headers       secrets header (not committed)
              │                                  │
              └────────────► build ◄─────────────┘
                               │
                               ▼
                    firmware for that project
```

- **The bridge repo holds no project's settings.** It ships an example config
  and secrets header only.
- **Each project keeps its own two headers** in its own repository.
- **Settings are compiled in.** The build is pointed at the project's folder;
  changing a setting means rebuilding and flashing.

## Config header

Not secret; committed in the project's repo.

| Setting | What it sets |
|---------|--------------|
| Routing table | First letter of a line → the MQTT topic it is published to |
| Subscribed topics | Topics whose messages are written down the UART |
| UART | Pins and baud rate of the link to the MCU (115200, 8N1 by default) |
| Time line | On or off; the line's prefix; how often it is sent |
| Link-status line | On or off; the line's prefix |

Example routing table and subscriptions:

| Direction | First letter / topic |
|-----------|----------------------|
| Up | `W` → `weather/nucleo-01/readings` |
| Up | `R` → `weather/nucleo-01/replies` |
| Down | `weather/nucleo-01/acks` |
| Down | `weather/nucleo-01/commands` |

## Secrets header

Secret; never committed. The bridge repo's example file shows the layout.

| Setting | What it sets |
|---------|--------------|
| Wi-Fi | Network name and password |
| Broker | Address and port |
| CA certificate | The certificate the broker's own certificate is checked against |
| Client certificate | The bridge's identity, presented to the broker |
| Client private key | Proves the client certificate belongs to this bridge |

## Versions

- **Working versions of the bridge are marked with git tags.**
- **A project names the tag it was built against** in its own docs: "build
  `uart-mqtt-bridge` at this tag with these two headers".
- **A project stays on its tag** until it is deliberately moved to a newer one,
  so later changes to the bridge do not affect it.

## See also

[architecture.md](architecture.md) · [decisions.md](../project-design/decisions.md)
