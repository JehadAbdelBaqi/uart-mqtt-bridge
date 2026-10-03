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
| 2 | **Lines read while the broker is unreachable are held, then sent late** — the MQTT client keeps a QoS 1 message and delivers it after connecting, late and not always in order (seen on the bench: the second dummy line arrived before the first) | A project puts a sequence number or timestamp in its messages, so late and repeated lines can be told apart | Open |
| 3 | **Behaviour not yet seen on the board** — a line ending in `\r\n`, a line over 127 characters, a downlink message over 127 characters or containing `\n`, and subscribing again after a real broker drop are written but untested | To be checked on the board; the checks then go into [testing.md](../system/testing.md) | Open |
| 4 | **The dummy data source is on in the repository's config** — a build for a real MCU that keeps `include/config.h` as it is would have the bridge writing test lines to that MCU | The dummy source is off when `DUMMY_LINE_INTERVAL_MS` is `0` or absent, and logs a warning at startup when it is on | Accepted |
| 5 | **The certificate files can only be embedded from `src/CMakeLists.txt`** — listed in the modules component, PlatformIO builds the embedded object twice | The embedding stays in `src/CMakeLists.txt`, with a comment in the modules' build file pointing to it | Handled |
| 6 | **The MQTT client is an outside package** — ESP-IDF 6 no longer includes it | Pinned to an exact version, with `dependencies.lock` committed | Handled |
| 7 | **ESP-IDF under PlatformIO can lag behind Espressif's own release** | The full `sdkconfig` is committed, so a build uses the settings it was tested with whichever version is installed | Accepted |

## The test setup

| # | Risk / unknown | Mitigation | Status |
|---|----------------|------------|--------|
| 8 | **The PC's address and the VM's address change** — each change breaks the server certificate, the port rule, or the address built into the firmware | `setup-certs.sh` finds both addresses on every run and remakes all three; the firmware is then rebuilt | Handled |
| 9 | **Antivirus can stall the Multipass service** — seen with Avast: `multipass launch` and `multipass list` stop answering | Shields off while the VM is created; restarting the service is in [commands.md](../system/commands.md) | Accepted |
| 10 | **The broker's port is open on the PC while a test runs** | The firewall rule allows the local network only, the broker refuses any client without a certificate, and both rules are removed at the end unless `--keep-alive` is given | Handled |
| 11 | **The end-to-end test needs the board** — it can't run on a hosted CI machine | It is run on the bench with `e2e.sh` | Accepted |
| 12 | **The scripts have only been run on Windows** — the Linux branches are written but not run, and the adapter checks use a Windows-only tool | To be run and fixed on Linux | Open |

## See also

[decisions.md](decisions.md) · [testing.md](../system/testing.md) · [resources.md](../system/resources.md)
