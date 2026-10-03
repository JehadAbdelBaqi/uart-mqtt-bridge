# Testing

Manual checks of the bridge on the bench, against the local test broker.

## Before any test

- Test broker set up and reachable: from `scripts/`, in a Git Bash opened as
  administrator, `bash setup-certs.sh --keep-alive` (see [scripts/README.md](../../scripts/README.md)).
  It ends with `TLS connection works.`
- Firmware built and flashed, serial monitor open at 115200 baud.

## Wi-Fi connects

| | |
|---|---|
| **Steps** | Reset the board. |
| **Expect** | After the startup LED sequence, the LED flashes amber, then blinks green once a second. The log shows `wifi: connecting to '<network>'...` then `wifi: connected, address <address>`. |

## Wi-Fi recovers after a drop

| | |
|---|---|
| **Steps** | With the bridge connected, take the network away (switch off the router's Wi-Fi, or move the board out of range). Bring it back after a minute. |
| **Expect** | LED red, and `wifi: not connected (reason <code>), trying again in 5 s`. Every 5 s the LED flashes amber while it tries again. Once the network is back: `wifi: connected, address <address>` with the LED blinking green, then solid green once the broker is connected again. |

The reason codes are ESP-IDF's `wifi_err_reason_t` values.

## Connects to the broker

| | |
|---|---|
| **Steps** | Reset the board. |
| **Expect** | Shortly after Wi-Fi connects: `mqtt: connecting to broker <address>:<port>...` then `mqtt: connected to broker <address>:<port>`, and the LED goes from blinking to solid green. |

If it doesn't connect, the `mqtt: error` lines give the TLS error code or the broker's reason for refusing.

## Recovers after a broker restart

| | |
|---|---|
| **Steps** | With the bridge connected to the broker, open a shell in the VM: `multipass shell <vm name>`. Run `sudo systemctl stop mosquitto`, wait, then `sudo systemctl start mosquitto`. |
| **Expect** | On stop: `mqtt: disconnected from broker, the client will try again` (the error lines above it are the closed connection being reported), and the LED blinks green. After start, within about 20 s: `mqtt: connected to broker <address>:<port>` and solid green, with nothing done on the bridge. |

## Secrets can't be committed

| | |
|---|---|
| **Steps** | With the hook switched on (`git config core.hooksPath .githooks`), stage a file in `include/secrets/` (`git add -f`), or any file containing a `BEGIN ... PRIVATE KEY` line, and try to commit. |
| **Expect** | `Commit refused: it would add secrets to the repo.`, naming the file. Unstage it with `git restore --staged <file>`. |

---

**See also:** [scripts/README.md](../../scripts/README.md) · [decisions.md](../project-design/decisions.md)
