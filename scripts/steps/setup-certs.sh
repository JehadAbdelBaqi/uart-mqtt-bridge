#!/usr/bin/env bash
set -euo pipefail

# Sets up the certificates for the connection to the broker in the VM named in config.sh:
# makes them, writes the firmware's files in include/secrets/ and installs the broker's own.
# The VM must exist (create-vm.sh). The broker is reached once the port rule is set
# (set-port-rule.sh).

source helpers/settings.sh
source helpers/vm-helpers.sh
source helpers/ssl-helpers.sh
# shellcheck source=helpers/network-helpers.sh
source "$NETWORK_HELPERS"
source helpers/firmware-helpers.sh

if [ "$#" -gt 0 ]; then
    echo "setup-certs.sh takes no options. The port rule is set by set-port-rule.sh." >&2
    exit 1
fi

# 1. Access to the VM
find_vm_address "$VM_NAME" "$MULTIPASS"
ensure_ssh_key "$PROJECT_NAME"
authorize_ssh_key "$PROJECT_NAME" "$MULTIPASS" "$VM_NAME"

# 2. Certificates
ensure_ca "$PROJECT_NAME"
ensure_client_cert "$PROJECT_NAME"
find_lan_address "$LAN_ADAPTER"
create_server_cert "$PROJECT_NAME" "$LAN_ADDRESS"

# 3. Firmware files
copy_firmware_certs "$PROJECT_NAME" "../include/secrets"
write_broker_header "$LAN_ADDRESS" "$BROKER_PORT" "../include/secrets"
write_wifi_header "$WIFI_SSID" "$WIFI_PASSWORD" "../include/secrets"

# 4. Broker
check_ssh "$SSH_OPTIONS" "$VM_ADDRESS"
install_broker_certs "$PROJECT_NAME" "$SSH_OPTIONS" "$VM_ADDRESS"
