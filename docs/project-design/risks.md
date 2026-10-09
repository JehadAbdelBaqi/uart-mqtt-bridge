# Risks & Unknowns

What could go wrong with the bridge or its test setup, and what is done about
each.

[← README](../../README.md)

**Status:** **Handled** — dealt with, nothing left to do · **Accepted** — known
and lived with · **Open** — still to be settled.

## The bridge

| # | Risk / unknown | Mitigation | Status |
|---|----------------|------------|--------|
| 1 | **Secrets are readable on the flash chip** — the Wi-Fi password and the client's private key are compiled into the firmware, and the flash is not encrypted | Use only the test broker's certificates and a test network; `include/secrets/` is never committed | Open |
| 2 | **Lines read while the broker is unreachable were held, then sent late** — the MQTT client keeps a QoS 1 message and delivers it after connecting, late and not always in order (seen on the bench: the second dummy line arrived before the first) | A line that arrives while Wi-Fi or the broker is down is no longer published: the MCU is answered at once with which is down, and keeps the line itself. Checked by the unit tests; seen on the board as the first dummy lines after a start not arriving | Handled |
| 3 | **Behaviour not yet seen on the board** — a line ending in `\r\n`, a line over 127 characters, a downlink message over 127 characters, subscribing again after a real broker drop, the handshake's three answers, the answer when the broker does not confirm a line, and the MCU shown as quiet and gone on the LED are written but not tried on the board | The message handling is covered by the unit tests on the PC. To be checked on the board with a microcontroller; the checks are in [testing.md](../system/testing.md) | Open |
| 4 | **The dummy data source is on in the repository's config** — a build for a real MCU that keeps `include/config.h` as it is would have the bridge writing test lines to that MCU | The dummy source is off when `DUMMY_LINE_INTERVAL_MS` is `0` or absent, and logs a warning at startup when it is on | Accepted |
| 5 | **The certificate files can only be embedded from `src/CMakeLists.txt`** — listed in the modules component, PlatformIO builds the embedded object twice | The embedding stays in `src/CMakeLists.txt`, with a comment in the modules' build file pointing to it | Handled |
| 6 | **The MQTT client is an outside package** — ESP-IDF 6 no longer includes it | Pinned to an exact version, with `dependencies.lock` committed | Handled |
| 7 | **ESP-IDF under PlatformIO can lag behind Espressif's own release** | The full `sdkconfig` is committed, so a build uses the settings it was tested with whichever version is installed | Accepted |
| 14 | **A confirmation can arrive before the bridge has noted which line it is for** — a line is published first and its ID noted a moment after, by one task, while the broker's confirmation is handled by another. A confirmation in that moment finds nothing held, and the MCU later gets the answer that the broker did not confirm | The gap is microseconds against a reply that takes milliseconds over Wi-Fi. To close it: hold the line before publishing it, and fill its ID in after | Open |
| 15 | **A line the broker did not confirm in time may still be delivered, and then twice** — the MQTT client keeps an unconfirmed QoS 1 message and may send it again itself once the connection is made afresh, while the MCU sends the same line again too. How the client behaves here has not been checked | A project puts a sequence number in its lines, so a repeated one can be told | Open |
| 16 | **A message from the broker that contains a line end reaches the MCU as two lines** — the bridge checks a downlink message's length and no longer what is in it | Whoever publishes to a downlink topic sends one line per message | Open |
| 17 | **A line from the MCU that is over the limit goes up cut short**, and looks like a whole line at the broker — the bridge does not check the MCU's lines | The MCU keeps its lines within the limit; the cut is there so that noise on the UART cannot overrun the bridge's memory | Accepted |
| 18 | **A line with no topic gets no answer** — the MCU waits, hears nothing, and takes the bridge for gone | A project routes every letter its MCU sends, and the bridge logs each such line | Open |

## The test setup

| # | Risk / unknown | Mitigation | Status |
|---|----------------|------------|--------|
| 8 | **The PC's address and the VM's address change** — each change breaks the server certificate, the port rule, or the address built into the firmware | `setup-certs.sh` finds both addresses on every run and remakes all three; the firmware is then rebuilt | Handled |
| 9 | **Antivirus can stall the Multipass service** — seen with Avast: `multipass launch` and `multipass list` stop answering | Shields off while the VM is created; restarting the service is in [commands.md](../system/commands.md) | Accepted |
| 10 | **The broker's port is open on the PC while a test runs** | The port rule listens on the PC's LAN address only, the broker refuses any client without a certificate, and `e2e.sh` removes the rule at the end unless `KEEP_PORT_RULE` is `1`; set by hand, it stays until `remove-port-rule.sh` is run or the PC restarts | Handled |
| 11 | **The end-to-end test needs the board** — it can't run on a hosted CI machine | It is run on the bench with `e2e.sh` | Accepted |
| 12 | **The Windows start script was written without a Windows run** — the scripts were reworked on Linux, and `windows.sh` with the Windows network functions was written there | Run on Windows afterwards: `bash windows.sh e2e.sh` passed from a clean slate, and `bash windows.sh steps/nuke.sh` by itself | Handled |
| 13 | **The Windows files were moved into `scripts/windows/` without a Windows run** — the start script now hands the scripts a settings file of its own (`SYSTEM_SETTINGS`), and the Windows values left the shared `config.sh`; this was done on a Linux PC, with no Windows PC to run it on | Run `bash windows/windows.sh e2e.sh` on Windows before a release that claims Windows support. The earlier layout passed there (see 12) | Open |

## See also

[decision_logs.md](decision_logs.md) · [testing.md](../system/testing.md) · [resources.md](../system/resources.md)
