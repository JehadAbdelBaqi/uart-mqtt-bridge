#!/usr/bin/env bash
set -euo pipefail

# Deletes everything the setup scripts created for the project named in config.sh:
# the VM, the certificates (including the CA), the SSH key, the port rule and the firewall rule.

source config.sh
source helpers/network-helpers.sh

# Removing the port rule and the firewall rule on Windows needs administrator rights
if [ "$IS_WINDOWS" = true ] && ! net session > /dev/null 2>&1; then
    echo "Run this from a Git Bash opened as administrator." >&2
    exit 1
fi

# The folders below are built from PROJECT_NAME, so anything but a plain name is refused
if [[ ! "$PROJECT_NAME" =~ ^[A-Za-z0-9_-]+$ ]]; then
    echo "PROJECT_NAME in config.sh must be a plain name (letters, numbers, - and _)." >&2
    exit 1
fi

CERT_DIR="~/certs/$PROJECT_NAME"
SSH_KEY_DIR="~/.ssh/$PROJECT_NAME"

echo "This will permanently delete, if they exist:"
echo "  - the VM '$VM_NAME' and everything on it"
echo "  - $CERT_DIR (the CA, server and client certificates and their keys)"
echo "  - $SSH_KEY_DIR (the project's SSH key)"
echo "  - the port rule for port $BROKER_PORT"
echo "  - the firewall rule '$FIREWALL_RULE_NAME'"
echo

read -r -p "Are you sure you want to delete all of this? (y/n) " answer
if [ "$answer" != "y" ]; then
    echo "Nothing deleted."
    exit 1
fi

read -r -p "Type the project name to confirm: " typed_name
if [ "$typed_name" != "$PROJECT_NAME" ]; then
    echo "That doesn't match the project name. Nothing deleted."
    exit 1
fi

echo

if "$MULTIPASS" info "$VM_NAME" > /dev/null 2>&1; then
    VM_ADDRESS=$("$MULTIPASS" list --format csv | tr -d '\r' | awk -F, -v name="$VM_NAME" '$1 == name { print $3 }')

    if [[ "$VM_ADDRESS" =~ ^[0-9.]+$ ]]; then
        echo "Forgetting the VM's SSH identity for $VM_ADDRESS..."
        $WSL_PREFIX bash -c "ssh-keygen -R $VM_ADDRESS" > /dev/null 2>&1 || true
    fi

    echo "Deleting VM '$VM_NAME'..."
    "$MULTIPASS" delete --purge "$VM_NAME"
else
    echo "VM '$VM_NAME' not found, skipping."
fi

for folder in "$CERT_DIR" "$SSH_KEY_DIR"; do
    if $WSL_PREFIX bash -c "test -d $folder"; then
        echo "Deleting $folder..."
        $WSL_PREFIX bash -c "rm -rf -- $folder"
    else
        echo "$folder not found, skipping."
    fi
done

remove_port_rule "$IS_WINDOWS" "$BROKER_PORT"
remove_firewall_rule "$IS_WINDOWS" "$FIREWALL_RULE_NAME"

echo "Done."
