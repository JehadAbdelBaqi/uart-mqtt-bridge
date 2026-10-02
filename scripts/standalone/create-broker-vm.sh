#!/usr/bin/env bash
set -euo pipefail

# Creates the Multipass VM named in config.sh, running Mosquitto set up for TLS with client certificates.

source config.sh

echo "Checking that VM '$VM_NAME' doesn't exist yet..."
if "$MULTIPASS" info "$VM_NAME" > /dev/null 2>&1; then
    echo "VM '$VM_NAME' already exists." >&2
    echo "To start again, remove it first: $MULTIPASS delete $VM_NAME && $MULTIPASS purge" >&2
    exit 1
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
