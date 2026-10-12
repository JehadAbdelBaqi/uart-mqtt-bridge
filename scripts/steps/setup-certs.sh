#!/usr/bin/env bash
set -euo pipefail

# Makes the certificates for the connection to the broker, on this PC, and writes the firmware's
# files in include/secrets/. The broker's own are put into the VM by setup-broker.sh.

source helpers/settings.sh
source helpers/ssl-helpers.sh
source helpers/network-helpers.sh
source helpers/firmware-helpers.sh

if [ "$#" -gt 0 ]; then
    echo "setup-certs.sh takes no options. The port rule is set by set-port-rule.sh." >&2
    exit 1
fi

# 1. Certificates
ensure_ca "$PROJECT_NAME"
ensure_client_cert "$PROJECT_NAME"
find_lan_address
create_server_cert "$PROJECT_NAME" "$LAN_ADDRESS"

# 2. Firmware files
copy_firmware_certs "$PROJECT_NAME" "../include/secrets"
write_broker_header "$LAN_ADDRESS" "$BROKER_PORT" "../include/secrets"
write_wifi_header "$WIFI_SSID" "$WIFI_PASSWORD" "../include/secrets"
