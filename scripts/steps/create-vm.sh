#!/usr/bin/env bash
set -euo pipefail

# Creates the Multipass VM named in config.sh, with a cloud-init file that creates the deploy user
# and lets the project's SSH key log in as it. The key must exist (create-ssh-key.sh).
# Nothing is installed in the VM here: the broker is set up by setup-broker.sh.

source helpers/settings.sh
source helpers/vm-helpers.sh

echo "Checking whether VM '$VM_NAME' exists..."
if multipass info "$VM_NAME" > /dev/null 2>&1; then
    echo "VM '$VM_NAME' already exists: nothing to create."
    exit 0
fi

echo "Creating VM '$VM_NAME'..."
print_cloud_init "$PROJECT_NAME" "vm/cloud-init.yaml" | multipass launch --name "$VM_NAME" --cloud-init -

echo "VM '$VM_NAME' is created."
multipass list
