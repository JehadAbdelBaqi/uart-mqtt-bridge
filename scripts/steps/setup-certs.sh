#!/usr/bin/env bash
set -euo pipefail

# Sets up the connection to the broker in the VM named in config.sh: certificates, the firmware's
# files in include/secrets/, the broker's certificates, and the PC's port rule.
# Ends with a TLS connection test. The VM must exist (create-vm.sh).
# Asks for the sudo password when it sets the port rule.

source config.sh
source helpers/vm-helpers.sh
source helpers/ssl-helpers.sh
# shellcheck source=helpers/network-helpers.sh
source "$NETWORK_HELPERS"
source helpers/broker-connection-test.sh
source helpers/firmware-helpers.sh

# Options:
#   --keep-alive   leave the port rule in place at the end
KEEP_ALIVE=false
for option in "$@"; do
    if [ "$option" = "--keep-alive" ]; then
        KEEP_ALIVE=true
    else
        echo "Unknown option '$option'. Options: --keep-alive" >&2
        exit 1
    fi
done

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

# 5. PC
# Without --keep-alive, the trap removes the port rule when the script ends, whether it finishes or fails.
if [ "$KEEP_ALIVE" = false ]; then
    trap 'remove_port_rule "$BROKER_PORT"' EXIT
fi
set_port_rule "$BROKER_PORT" "$LAN_ADDRESS" "$VM_ADDRESS"

# 6. Test
check_broker_tls "$PROJECT_NAME" "$LAN_ADDRESS" "$BROKER_PORT"
