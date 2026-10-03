#!/usr/bin/env bash
set -euo pipefail

source config.sh
source helpers/vm-helpers.sh
source helpers/ssl-helpers.sh
source helpers/network-helpers.sh
source helpers/windows-forwarding-helpers.sh
source helpers/broker-connection-test.sh
source helpers/firmware-helpers.sh

# Options:
#   --nuke         delete everything first, then create the VM and carry on
#   --create-vm    create the VM first, then carry on
#   --keep-alive   leave the port rule and the firewall rule in place at the end
NUKE=false
CREATE_VM=false
KEEP_ALIVE=false
for option in "$@"; do
    if [ "$option" = "--nuke" ]; then
        NUKE=true
    elif [ "$option" = "--create-vm" ]; then
        CREATE_VM=true
    elif [ "$option" = "--keep-alive" ]; then
        KEEP_ALIVE=true
    else
        echo "Unknown option '$option'. Options: --nuke, --create-vm, --keep-alive" >&2
        exit 1
    fi
done

# WSL's network adapter only exists while WSL is running
if [ "$IS_WINDOWS" = true ]; then
    wsl -e true
fi

# Adapter names from config.sh, checked before anything is changed
check_adapter_exists "$WSL_ADAPTER"
check_adapter_exists "$VM_ADAPTER"
check_adapter_exists "$LAN_ADAPTER"

# 0. VM (only with --nuke or --create-vm)
if [ "$NUKE" = true ]; then
    bash standalone/self-destruct.sh
fi
if [ "$NUKE" = true ] || [ "$CREATE_VM" = true ]; then
    bash standalone/create-broker-vm.sh
fi

# 1. Access to the VM
find_vm_address "$VM_NAME" "$MULTIPASS"
ensure_ssh_key "$WSL_PREFIX" "$PROJECT_NAME"
authorize_ssh_key "$WSL_PREFIX" "$PROJECT_NAME" "$MULTIPASS" "$VM_NAME"

# 2. Certificates
ensure_ca "$WSL_PREFIX" "$PROJECT_NAME"
ensure_client_cert "$WSL_PREFIX" "$PROJECT_NAME"
find_lan_address "$IS_WINDOWS" "$LAN_ADAPTER"
create_server_cert "$WSL_PREFIX" "$PROJECT_NAME" "$LAN_ADDRESS"

# 3. Firmware files
copy_firmware_certs "$WSL_PREFIX" "$PROJECT_NAME" "../include/secrets"
write_broker_header "$LAN_ADDRESS" "$BROKER_PORT" "../include/secrets"
write_wifi_header "$WIFI_SSID" "$WIFI_PASSWORD" "../include/secrets"

# 4. Broker
# Forwarding is only needed for SSH. The trap switches it off if anything in this section fails.
trap 'set_forwarding disabled "$WSL_ADAPTER" "$VM_ADAPTER"' EXIT
set_forwarding enabled "$WSL_ADAPTER" "$VM_ADAPTER"
check_ssh "$WSL_PREFIX" "$SSH_OPTIONS" "$VM_ADDRESS"
install_broker_certs "$WSL_PREFIX" "$PROJECT_NAME" "$SSH_OPTIONS" "$VM_ADDRESS"
set_forwarding disabled "$WSL_ADAPTER" "$VM_ADAPTER"
trap - EXIT

# 5. PC
# Without --keep-alive, the trap removes both rules when the script ends, whether it finishes or fails.
if [ "$KEEP_ALIVE" = false ]; then
    trap 'remove_port_rule "$IS_WINDOWS" "$BROKER_PORT"; remove_firewall_rule "$IS_WINDOWS" "$FIREWALL_RULE_NAME"' EXIT
fi
set_port_rule "$IS_WINDOWS" "$BROKER_PORT" "$LAN_ADDRESS" "$VM_ADDRESS"
ensure_firewall_rule "$IS_WINDOWS" "$FIREWALL_RULE_NAME" "$BROKER_PORT"

# 6. Test
check_broker_tls "$WSL_PREFIX" "$PROJECT_NAME" "$LAN_ADDRESS" "$BROKER_PORT"
