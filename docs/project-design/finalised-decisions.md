# Finalised Decisions

What the bridge is, what it uses and how it is put together: the decisions made and still in force.

[← README](../../README.md)

## How this file is used

- **It holds only decisions in force.** A decision that is dropped is removed from here; one that is replaced has its line rewritten. Nothing is listed as "not used".
- **One short line each.** The choice itself, without its settings or values (those belong in [docs/system/](../system/)) and without its reasons.
- **Not numbered.** The reasons, the alternatives and the full history are in [decision_logs.md](decision_logs.md), which is only ever added to.
- **A choice made, not a statement that it is built.** The [README](../../README.md) says what the bridge does today.

## What the bridge is

- A UART ↔ MQTT bridge, named `uart-mqtt-bridge`
- It passes lines through as they are
- It connects to any MQTT broker over TLS with client certificates
- Storing and resending belong to the board that uses the bridge

## How a project uses the bridge

- A project's settings live in the project's own repository
- A project hands the bridge three config files, each by its path in an environment variable
- The firmware includes one generated header that points at the config file for that build
- `config.sh` holds only the values someone has to choose
- The bridge's scripts make the certificates for the project
- Versions are git tags, and each project names the tag it is built with

## The link to the MCU

- A handshake with the MCU over the UART when the bridge is built for a project
- Only the MCU asks in the handshake; the bridge answers, and counts the MCU as gone after a set time with nothing heard
- The handshake request is answered with the same line type as any other line, and the result says whether the bridge can reach Wi-Fi and the broker
- The handshake's timings come from the project's firmware config
- One message is one line, with one length limit for both directions
- The link's baud rate and line limit come from the config, a project's or the bridge's own
- Uplink routing by a table of first letters in the config
- The config header holds only values, written as lists
- Optional time line to the MCU, from NTP
- Every line from the MCU is answered within a set time: acknowledged once the broker confirms it, or reported as not gone through
- The answer is one line type, carrying the line's letter, its first field and the result
- The bridge's time to answer is worked out from the MCU's own time limit
- A missed confirmation makes the bridge drop its connection to the broker and make it again
- The MCU is told when the bridge loses its connection upstream and when it is back

## Firmware

- ESP-IDF, built with PlatformIO
- Settings compiled in from headers
- Espressif's `espressif/mqtt` package as the MQTT client, pinned to an exact version
- The full `sdkconfig` is committed
- What operates a part of the board or a connection lives in one component, `components/modules/`, a folder per module
- The bridge's own rules are application code in `src/app/`, with their headers in `include/app/`
- A module sets itself up from the board's header and the config, and calls nothing in the application
- The Wi-Fi and MQTT modules each hold their own connection state
- The board's wiring in one header, `include/board.h`
- The application is two files, one for each direction: `downstream` for the MCU, `upstream` for the broker
- Publishing, subscribing and receiving messages are the MQTT module's
- A wait before reconnecting after a Wi-Fi drop
- Flash encryption and secure boot

## Status LED and logging

- The on-board RGB LED for status, driven with ESP-IDF's RMT driver
- It shows the connection to the MCU first, then the Wi-Fi and broker states, and blinks on traffic
- Four colours: red, amber, green, blue
- Logging of the lines themselves is a config setting

## Certificates and secrets

- The certificates are created by a re-runnable script in this repository
- The CA is kept on the PC, outside the repository and outside the VM
- The SSH key and the certificates are kept in folders named for the project
- Every file in `include/secrets/` is written by the setup script
- A pre-commit hook refuses commits that would add secrets, and runs ShellCheck on staged scripts

## Test broker and network

- Mosquitto with TLS and client certificates, in a Multipass Ubuntu VM, as the test broker
- The PC passes the broker's port, on its LAN address, to the VM
- The port rule is set and removed by steps of their own
- The port rule is removed when a run ends unless it is asked to stay
- The setup reaches the VM over SSH
- The setup ends with a TLS connection test using the client certificate

## Scripts

- One script per job in `scripts/steps/`, and `e2e.sh` to run them
- The steps, their order and a switch for each are one list in `build-config.sh`
- Settings are read from the config once and passed into each function as arguments
- The scripts are written for Linux
- Windows, for the bridge standing alone, has its own start script and config in `scripts/windows/`, and runs the same scripts in WSL

## Testing

- A dummy data source in the firmware, looped back with a jumper, for testing the bridge standing alone
- An end-to-end test script, through the broker
- Unit tests on the PC for the application code, with Unity under PlatformIO, and fakes in place of the board
- A firmware build in CI, on GitHub Actions

## Repository and releases

- Work towards a release is collected on a release branch, then merged into `master` and tagged
- Decisions are kept in two files: the log, and this file

**See also:** [decision_logs.md](decision_logs.md) · [risks.md](risks.md) · [architecture.md](../system/architecture.md) · [configuration.md](../system/configuration.md)
