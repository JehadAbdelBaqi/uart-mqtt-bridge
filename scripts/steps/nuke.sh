#!/usr/bin/env bash
set -euo pipefail

# Deletes everything the scripts created for the project named in config.sh:
# the VM, the certificates (including the CA), the SSH key, the firmware's files in include/secrets/
# and the port rule. Asks twice before deleting anything.
# Asks for the sudo password when it removes the port rule.

source config.sh
# shellcheck source=helpers/network-helpers.sh
source "$NETWORK_HELPERS"
source helpers/nuke-helpers.sh

# 1. Checks, before anything is deleted
check_project_name "$PROJECT_NAME"
confirm_nuke "$PROJECT_NAME" "$VM_NAME" "$BROKER_PORT"

# 2. VM
delete_vm "$MULTIPASS" "$VM_NAME"

# 3. Certificates and SSH key
delete_folder "$HOME/certs/$PROJECT_NAME"
delete_folder "$HOME/.ssh/$PROJECT_NAME"

# 4. Firmware files
delete_firmware_files "../include/secrets"

# 5. PC
remove_port_rule "$BROKER_PORT"

echo "Done."
