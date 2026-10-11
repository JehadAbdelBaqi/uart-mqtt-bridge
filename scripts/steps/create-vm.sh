#!/usr/bin/env bash
set -euo pipefail

# Creates the Multipass VM named in config.sh, running Mosquitto set up for TLS with client certificates.

source helpers/settings.sh

echo "Checking whether VM '$VM_NAME' exists..."
if "$MULTIPASS" info "$VM_NAME" > /dev/null 2>&1; then
    echo "VM '$VM_NAME' already exists: nothing to create."
    exit 0
fi

echo "Creating VM '$VM_NAME'..."
"$MULTIPASS" launch --name "$VM_NAME"

echo "Installing Mosquitto..."
"$MULTIPASS" exec "$VM_NAME" -- bash -c "sudo apt-get update && sudo apt-get install -y mosquitto mosquitto-clients"

echo "Writing Mosquitto's TLS settings..."
"$MULTIPASS" exec "$VM_NAME" -- bash -c "printf '%s\n' 'listener $BROKER_PORT' 'cafile /etc/mosquitto/ca_certificates/ca.crt' 'certfile /etc/mosquitto/certs/server.crt' 'keyfile /etc/mosquitto/certs/server.key' 'require_certificate true' 'use_identity_as_username true' | sudo tee /etc/mosquitto/conf.d/tls.conf > /dev/null"

echo "VM '$VM_NAME' is ready."
echo "Mosquitto keeps running without TLS until the certificates are installed and it is restarted."
"$MULTIPASS" list
