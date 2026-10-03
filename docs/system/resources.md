# Resources

Everything needed to build, flash and test the bridge: hardware, software and
reference documentation.

[← README](../../README.md)

"Tested with" is the version the bridge and its scripts were last run with. A
dash means the version wasn't recorded.

## Hardware

| Item | Used for | Where to find out more |
|------|----------|------------------------|
| **Board:** Axiometa Genesis Mini | Runs the bridge firmware | [axiometa.io](https://axiometa.io) — the pinout and the board's other documents are on Axiometa's website |
| **Microcontroller:** Espressif ESP32-S3, as the ESP32-S3-MINI-1 module on the board (N4R2: 4 MB flash, 2 MB PSRAM) | The chip the firmware runs on: Wi-Fi, UART, the RMT peripheral that drives the LED | Espressif's ESP32-S3-MINI-1 datasheet: pins, memory, radio and electrical limits |
| USB cable to the PC | Power, uploading the firmware, the serial monitor | — |
| One jumper wire, GPIO7 (TX) to GPIO6 (RX) | Loopback for testing without an MCU | — |
| A 2.4 GHz Wi-Fi network, with the PC on the same network | The bridge's route to the broker | — |

The bridge uses three of the board's pins, all set in `include/board.h`: GPIO7
(UART TX), GPIO6 (UART RX) and GPIO21 (the on-board RGB LED).

## Software on the PC (Windows)

| Software | Used for | Tested with |
|----------|----------|-------------|
| Windows | Runs everything below | Windows 11 Pro |
| Visual Studio Code with the PlatformIO IDE extension | Editing, building, uploading, the serial monitor | — |
| PlatformIO `espressif32` platform | The ESP32 toolchain and upload tools; installed by PlatformIO | 7.1.3 |
| ESP-IDF, as PlatformIO's `framework-espidf` package | The firmware's framework; downloaded on the first build | 6.1.0 |
| Git for Windows, including Git Bash | Version control; the shell the scripts are started in | — |
| Multipass | Creates and runs the test broker's VM | 1.16.4 |
| Hyper-V | The virtual machine layer Multipass and WSL run on | — |
| WSL 2 with Ubuntu | The Linux tools the scripts use | Ubuntu 24.04 |

## Software inside WSL

| Software | Used for | Tested with |
|----------|----------|-------------|
| `openssl` | Creating the CA and the certificates; the TLS connection test | — |
| `ssh`, `scp`, `ssh-keygen` | Reaching the VM and copying the certificates to it | — |
| `mosquitto-clients` (`mosquitto_pub`, `mosquitto_sub`) | Sending and watching test messages; the end-to-end test. Install: `sudo apt install mosquitto-clients` | 2.0.18 |

## Software inside the VM

Installed by `scripts/steps/create-vm.sh`; nothing to install by hand.

| Software | Used for | Tested with |
|----------|----------|-------------|
| Ubuntu (Multipass's default image) | The VM's operating system | 26.04 LTS |
| Mosquitto | The MQTT broker, with TLS and client certificates required | — |

## Firmware dependencies

| Dependency | Used for | Version |
|------------|----------|---------|
| `espressif/mqtt` (ESP-IDF component registry) | The MQTT client | 1.1.0, pinned in `components/modules/idf_component.yml` |
| ESP-IDF's own components (`esp_wifi`, `esp_netif`, `esp_event`, `esp-tls`, `mbedtls`, UART and RMT drivers) | Wi-Fi, networking, TLS, the UART link, the status LED | With ESP-IDF |

## Reference documentation

| Document | For |
|----------|-----|
| ESP-IDF Programming Guide — https://docs.espressif.com/projects/esp-idf/ | UART driver, Wi-Fi, event loop, ESP-TLS, the RMT driver |
| ESP-MQTT documentation (part of Espressif's docs) | The MQTT client's configuration and events |
| PlatformIO documentation — https://docs.platformio.org/ | `platformio.ini`, the command line |
| Mosquitto documentation — https://mosquitto.org/documentation/ | `mosquitto.conf`, `mosquitto_pub`, `mosquitto_sub` |
| Genesis Mini pinout and documents — https://axiometa.io | Which pins the UART and the LED are on |
| ESP32-S3-MINI-1 datasheet (Espressif) | The microcontroller module: pins, memory, radio, electrical limits |

## See also

[set-up-and-test.md](../how-to/set-up-and-test.md) · [commands.md](commands.md) · [risks.md](../project-design/risks.md)
