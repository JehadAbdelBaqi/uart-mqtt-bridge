# Finalised Decisions

The design decisions in force today, by subject. This file is rewritten as
decisions change; [decision_logs.md](decision_logs.md) is the full log, in the order the
decisions were made, with the alternatives each one was chosen over.

[← README](../../README.md)

The **Log** column gives the numbers of the log entries a row comes from. A
decision listed here is a choice that has been made, not a statement that it is
built: the [README](../../README.md) says what the bridge does today.

## What the bridge is

| Decision | Why | Log |
|----------|-----|-----|
| **Named `uart-mqtt-bridge`; its scope is UART ↔ MQTT only** | The name states the two interfaces it has; Wi-Fi is only the transport to the broker | 15 |
| **It passes lines through as they are** — it never parses or converts a message | Usable by any project without code changes; the message format belongs to the project | 1 |
| **No backlog in the bridge** — a line that arrives while Wi-Fi or the broker is down is dropped. *Under review for `v1.0.0`: what the bridge does with such lines is being settled with its first project* | The bridge is an extension of the board using it; that board owns any storing and resending | 11 |
| **Local only** — no AWS resources here; a first AWS IoT Core connection is made by the project that uses the bridge | IoT Core is one more TLS broker with client certificates, already covered by the local test | 14 |
| **Connects to any MQTT broker over TLS with client certificates** | One code path covers a local Mosquitto and AWS IoT Core | 7 |

## How a project uses the bridge

| Decision | Why | Log |
|----------|-----|-----|
| **A project's settings live in the project's own repository; the bridge's repository holds none of them** | One branch of bridge code for every project; a project's topics stay with that project | 2 |
| **A project hands the bridge's scripts one folder holding `config.sh` (for the scripts) and `config.h` (for the firmware)**; with no folder given, the bridge uses its own two files and runs standing alone | A project uses the bridge as it is and writes nothing into its repository; one location covers both files | 8, 73 |
| **`config.sh` holds only values someone has to choose**; values that are the same for every setup are in the scripts | A project's config is short and cannot get a fixed value wrong | 74 |
| **The certificates are the bridge's job** — its scripts make them for whichever project name the config gives; a project's repository holds no certificate | The work is the same for every project and already written here; a project cannot commit a key it never holds | 75 |
| **No git submodule** — a project states which version of the bridge it is built with | The settings sit outside the bridge's code, so nothing needs nesting inside the project's repository | 3 |
| **Versions are git tags; each project names the tag it was built against** | Later bridge changes cannot break a project until it deliberately moves | 4 |

## Messages and routing

| Decision | Why | Log |
|----------|-----|-----|
| **Uplink routing by a first-letter table in the config** — the whole line is published unchanged to the topic its first letter picks | The bridge reads one character and nothing else, while each line type still gets its own topic | 5 |
| **The config header holds only values, written as lists** (`UPLINK_ROUTES`, `DOWNLINK_TOPICS`) | A project's config contains no types and no code, and reads as a table | 55 |
| **One rule for both directions: a message is one line of at most 127 characters with no `\n`**; anything longer, or a downlink message containing `\n`, is dropped whole and logged | The MCU always receives exactly one whole line per message; a cut-off line would look like a valid message | 24, 53 |
| **The line limit is one value, `LINE_MAX_LEN`, used for both directions** | Written once, it cannot drift apart | 58 |
| **Optional time line, off by default** — when on, the bridge gets the time over NTP and writes it down the UART | The bridge is the part with network access next to the MCU; off by default keeps it pure pass-through | 6 |
| **Optional link-status line to the MCU, off by default** | The MCU can show the link state or stop sending without the bridge understanding the project | 16 |

## Firmware

| Decision | Why | Log |
|----------|-----|-----|
| **Written on ESP-IDF, built with PlatformIO** | QoS 1 publishing with a queue, background reconnects, flash encryption and secure boot; the same build workflow as the MCU projects | 9, 10 |
| **Settings are compiled in from headers** — no file system, parser or provisioning code on the ESP32 | Certificates never sit in a readable file on the board | 8 |
| **The MQTT client is Espressif's `espressif/mqtt` package, pinned to an exact version, with `dependencies.lock` committed** | Nothing changes unless the version is moved deliberately | 49 |
| **The full `sdkconfig.genesis-mini` is committed** | A clean checkout builds with exactly the settings the firmware was tested with | 23 |
| **The modules live in one component, `components/modules/`, a folder per module; `src/` holds only `main.c`** | Each module's files sit together, with a single build file | 52 |
| **The board's wiring lives in one header, `include/board.h`** | Moving to another board means changing one file | 51 |
| **Routing is its own module, `router`, between the UART link and the MQTT link; the MQTT link only keeps the connection** | Each module has one job: connection changes touch the MQTT link, topic and message changes touch the router | 56, 57 |
| **After a Wi-Fi drop, the bridge waits 5 s before connecting again** | Retries do not run back to back, and the red LED is on long enough to be seen | 45 |
| **Flash encryption and secure boot are part of the core build** | The Wi-Fi password and the client key are compiled into the firmware | 19 |

## Status LED and logging

| Decision | Why | Log |
|----------|-----|-----|
| **A status LED shows the link state and blinks on traffic** — the on-board RGB LED, driven with ESP-IDF's built-in RMT driver | The state is visible on the bench with no serial monitor; no outside dependency for one LED | 17, 21 |
| **Four colours: red, amber, green, blue** | A small fixed set is told apart at a glance | 22 |
| **Red = Wi-Fi down; amber flashing = connecting; green blinking = Wi-Fi up, broker not connected; green solid = both up** | Each state is read at a glance, and a broker fault does not look like everything working | 44, 50 |
| **Logging of the lines themselves is a config setting, `LOG_LINES`** — on, every line from the MCU is logged as `uart: up: ...` and every line to it as `uart: down: ...`; off or left out, neither | A project sees what crosses the UART by changing a config value; a deployed bridge can run with a quiet log | 68 |

## Certificates and secrets

| Decision | Why | Log |
|----------|-----|-----|
| **The certificates are created by a script in this repository, which is re-runnable and finds the PC's current address itself** | The same result every time; a changed address is fixed by one command | 25, 30 |
| **The CA is kept on the PC, outside the repository and outside the VM** | A VM can be deleted without losing the CA; the firmware's certificates stay the same from one test broker to the next | 27 |
| **The SSH key and the certificates are kept in folders named for the project** — `~/.ssh/<project>/` and `~/certs/<project>/` | Nothing is mixed in with the developer's own keys; removing a project's setup means removing those two folders | 36 |
| **Every file in `include/secrets/` is written by the setup script** — the certificates as the PEM files themselves, `broker.h` with the address and port, `wifi.h` from the values in `config.sh` | Nothing there is edited by hand, and the firmware's copies always match the broker's | 46, 47, 48 |
| **A pre-commit hook kept in the repository refuses commits that would add secrets, and runs ShellCheck on staged shell scripts** | A second guard behind `.gitignore`, in plain Bash; script mistakes are caught before they run | 43, 66 |

## Test broker and network

| Decision | Why | Log |
|----------|-----|-----|
| **The standalone test broker is Mosquitto with TLS and client certificates in a Multipass Ubuntu VM** | No cloud account or cost | 13 |
| **The PC passes the broker's port, on its LAN address only, to the VM** — `iptables` on Linux, `netsh` on Windows | Multipass does not offer the PC's Wi-Fi adapter for bridging; only connections arriving on the local network's address are passed on | 20, 38, 64 |
| **The port rule is removed again after the test unless `--keep-alive` is given** | The PC is left closed by default | 40 |
| **The setup reaches the VM over SSH** | Works with any Linux host, not only a Multipass VM | 26 |
| **The setup ends with a TLS connection test through the PC's LAN address, using the client certificate** | It takes the path a device takes, so everything is proven before any firmware is involved | 41 |

## Scripts

| Decision | Why | Log |
|----------|-----|-----|
| **One script per job in `scripts/steps/`, and `e2e.sh` to run them all**; helper files of functions grouped by subject; everything is run from `scripts/` | Any one step can be run again by itself; one run location keeps every path the same | 42, 61 |
| **Settings are read from the config once and passed into each function as arguments** | A function's argument list shows everything it depends on | 37 |
| **The scripts are written for Linux and hold no Windows checks; Windows gets its own start script, `windows.sh`**, which does the Windows-only parts and runs the same scripts in WSL. *Under review: how the Windows and Linux settings are kept apart in the config* | Each script reads as one straight list of steps | 62, 63, 64 |
| **On Windows: a working WSL is a stated prerequisite; the start script starts WSL itself, and keeps forwarding between WSL's network and the VM's switched on for the whole run** | WSL's adapter only exists while WSL runs; the PC is left as it was found | 34, 54, 65 |

## Testing

| Decision | Why | Log |
|----------|-----|-----|
| **Testable on its own with dummy data** — a module in the firmware writes numbered lines to the bridge's own UART, looped back with a jumper; switched on by an interval in the config, off when that is 0 or absent | The whole uplink is exercised with no MCU attached; a project's config never switches it on by accident | 12, 59 |
| **Tested end to end by a script, through the broker only** | A message on a topic is a clear pass or fail and proves the whole chain at once | 60 |
| **Firmware build in CI** — GitHub Actions compiles every push | A tagged version is known to build from a clean checkout | 18 |

## Repository and releases

| Decision | Why | Log |
|----------|-----|-----|
| **Work towards `v1.0.0` is collected on a `release/v1.0.0` branch**, merged into `master` and tagged once the bridge has been run with a microcontroller | `master` always matches the last release | 67 |
| **Two decision files: the log, and this file of decisions in force** | The log shows how the design got here; this file states the current design directly | 76 |

**See also:** [decision_logs.md](decision_logs.md) · [risks.md](risks.md) · [architecture.md](../system/architecture.md) · [configuration.md](../system/configuration.md)
