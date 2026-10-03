#!/usr/bin/env bash
set -euo pipefail

# Proves the whole project end to end: deletes the test setup, creates it again from nothing,
# builds and uploads the firmware, and tests the bridge through the broker.
# Needs the board plugged in with no serial monitor on its port, and the UART's TX jumpered to RX.
# Run from a Git Bash opened as administrator.

source config.sh
source helpers/network-helpers.sh

# Options:
#   --skip-nuke    keep the VM and certificates that exist: nothing is deleted, no VM is created
#   --keep-alive   leave the port rule and the firewall rule in place at the end
SKIP_NUKE=false
KEEP_ALIVE=false
for option in "$@"; do
    if [ "$option" = "--skip-nuke" ]; then
        SKIP_NUKE=true
    elif [ "$option" = "--keep-alive" ]; then
        KEEP_ALIVE=true
    else
        echo "Unknown option '$option'. Options: --skip-nuke, --keep-alive" >&2
        exit 1
    fi
done

# 1. Start from nothing
if [ "$SKIP_NUKE" = false ]; then
    bash steps/nuke.sh
    bash steps/create-vm.sh
fi

# 2. Certificates and connection
# The two rules have to stay in place for the bridge test, so the setup is told to keep them.
# Without --keep-alive, the trap removes them when this script ends, whether it finishes or fails.
if [ "$KEEP_ALIVE" = false ]; then
    trap 'remove_port_rule "$IS_WINDOWS" "$BROKER_PORT"; remove_firewall_rule "$IS_WINDOWS" "$FIREWALL_RULE_NAME"' EXIT
fi
bash steps/setup-certs.sh --keep-alive

# 3. Firmware
bash steps/build-and-upload.sh

# 4. Test
bash steps/test-bridge.sh

echo "End-to-end test passed."
