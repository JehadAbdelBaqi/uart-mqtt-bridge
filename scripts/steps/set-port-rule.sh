#!/usr/bin/env bash
set -euo pipefail

# Sets the PC's port rule, which passes the broker's port on the PC's LAN address to the VM named
# in config.sh, then tests the TLS connection to the broker through it.
# The VM must exist (create-vm.sh) and the certificates must be made (setup-certs.sh).
# Asks for the sudo password. The rule is left in place: remove-port-rule.sh takes it away.

source helpers/settings.sh
source helpers/vm-helpers.sh
# shellcheck source=helpers/network-helpers.sh
source "$NETWORK_HELPERS"
source helpers/broker-connection-test.sh

# 1. Addresses
find_vm_address "$VM_NAME" "$MULTIPASS"
find_lan_address "$LAN_ADAPTER"

# 2. PC
set_port_rule "$BROKER_PORT" "$LAN_ADDRESS" "$VM_ADDRESS"

# 3. Test
check_broker_tls "$PROJECT_NAME" "$LAN_ADDRESS" "$BROKER_PORT"
