#!/usr/bin/env bash
set -euo pipefail

# Tests the running bridge through the broker: a line comes up from the bridge, and a message
# sent down comes back up.
# Needs the firmware running on the board with the test config (include/config.h), the UART's TX
# jumpered to RX, and the broker reachable (setup-certs.sh --keep-alive).

source config.sh
# shellcheck source=helpers/network-helpers.sh
source "$NETWORK_HELPERS"
source helpers/bridge-test.sh

# The topics in the bridge's test config (include/config.h)
TEST_UP_TOPIC="bridge/test/up"
TEST_DOWN_TOPIC="bridge/test/down"

find_lan_address "$LAN_ADAPTER"
check_bridge_uplink "$PROJECT_NAME" "$LAN_ADDRESS" "$BROKER_PORT" "$TEST_UP_TOPIC" 60
check_bridge_loopback "$PROJECT_NAME" "$LAN_ADDRESS" "$BROKER_PORT" "$TEST_UP_TOPIC" "$TEST_DOWN_TOPIC" 10

echo "Bridge test passed."
