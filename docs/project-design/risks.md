# Risks & Unknowns

What could go wrong with the bridge or its test setup, and what is done about
each.

[← README](../../README.md)

**Status:** **Handled** — dealt with, nothing left to do · **Accepted** — known
and lived with · **Open** — still to be settled.

## The bridge

| # | Risk / unknown | Mitigation | Status |
|---|----------------|------------|--------|
| 1 | **Flash encryption and secure boot can't be undone** — they are set by eFuses, which can't be reset. In release mode, plain reflashing over USB is disabled, while every settings change is a reflash because settings are compiled in | Read up and choose the mode (development or release) before any eFuse is written; try it on a spare board first | Open |
| 2 | **Secrets are readable on the flash chip until flash encryption is on** — the Wi-Fi password and the client's private key are compiled into the firmware | Use only the test broker's certificates and a test network until then; `include/secrets/` is never committed | Open |
| 3 | **Lines sent while the broker is unreachable are held, not dropped** — the MQTT client keeps a QoS 1 message and delivers it after reconnecting, late and not always in order (seen on the bench: the second dummy line arrived before the first). The design says the bridge keeps no backlog | A project puts a sequence number or timestamp in its messages, so late and repeated lines can be told apart. Dropping lines while the link is down is settled with the first project that uses the bridge | Open |
| 4 | **Behaviour not yet seen on the board** — a line ending in `\r\n`, a line over 127 characters, a downlink message over 127 characters or containing `\n`, and resubscribing after a real broker drop are written but untested | Checked with the first project that uses the bridge; the checks go into [testing.md](../system/testing.md) | Open |
| 5 | **How a project's own config reaches the build is not worked out** — the build has to be pointed at a config header and secrets kept outside the bridge's repository | Solved with the first project that uses the bridge; until then the repository's test config is the only one | Open |
| 6 | **The dummy data source is on in the repository's config** — a project that copies `include/config.h` as its starting point would have the bridge writing test lines to its MCU | The dummy source is off when `DUMMY_LINE_INTERVAL_MS` is `0` or absent, and logs a warning at startup when it is on | Accepted |
| 7 | **The certificate files can only be embedded from `src/CMakeLists.txt`** — listed in the modules component, PlatformIO builds the embedded object twice | The embedding stays in `src/CMakeLists.txt`, with a comment in the modules' build file pointing to it | Handled |
| 8 | **The MQTT client is an outside package** — ESP-IDF 6 no longer includes it | Pinned to an exact version, with `dependencies.lock` committed | Handled |
| 9 | **ESP-IDF under PlatformIO can lag behind Espressif's own release** | The full `sdkconfig` is committed, so a build uses the settings it was tested with whichever version is installed | Accepted |

## The test setup

| # | Risk / unknown | Mitigation | Status |
|---|----------------|------------|--------|
| 10 | **The PC's address and the VM's address change** — each change breaks the server certificate, the port rule, or the address built into the firmware | `setup-certs.sh` finds both addresses on every run and remakes all three; the firmware is then rebuilt | Handled |
| 11 | **Antivirus can stall the Multipass service** — seen with Avast: `multipass launch` and `multipass list` stop answering | Shields off while the VM is created; restarting the service is in [commands.md](../system/commands.md) | Accepted |
| 12 | **The broker's port is open on the PC while a test runs** | The firewall rule allows the local network only, the broker refuses any client without a certificate, and both rules are removed at the end unless `--keep-alive` is given | Handled |
| 13 | **The end-to-end test needs the board** — it can't run on a hosted CI machine | CI builds the firmware; the end-to-end test is run on the bench with `e2e.sh` | Accepted |
| 14 | **The scripts have only been run on Windows** — the Linux branches are written but not run, and the adapter checks use a Windows-only tool | To be run and fixed on Linux | Open |

## See also

[decisions.md](decisions.md) · [testing.md](../system/testing.md) · [resources.md](../system/resources.md)
