#!/usr/bin/env bash
set -euo pipefail

# Deletes everything the scripts created for the project named in config.sh:
# the VM, the certificates (including the CA), the SSH key, the firmware's files in include/secrets/,
# the port rule and the firewall rule. Asks twice before deleting anything.
# Run from a Git Bash opened as administrator.

source config.sh
source helpers/network-helpers.sh
source helpers/nuke-helpers.sh

# 1. Checks, before anything is deleted
check_administrator "$IS_WINDOWS"
check_project_name "$PROJECT_NAME"
confirm_nuke "$PROJECT_NAME" "$VM_NAME" "$BROKER_PORT" "$FIREWALL_RULE_NAME"

# 2. VM
delete_vm "$WSL_PREFIX" "$MULTIPASS" "$VM_NAME"

# 3. Certificates and SSH key
delete_folder "$WSL_PREFIX" "~/certs/$PROJECT_NAME"
delete_folder "$WSL_PREFIX" "~/.ssh/$PROJECT_NAME"

# 4. Firmware files
delete_firmware_files "../include/secrets"

# 5. PC
remove_port_rule "$IS_WINDOWS" "$BROKER_PORT"
remove_firewall_rule "$IS_WINDOWS" "$FIREWALL_RULE_NAME"

echo "Done."
