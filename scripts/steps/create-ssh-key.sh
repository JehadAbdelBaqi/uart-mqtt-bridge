#!/usr/bin/env bash
set -euo pipefail

# Creates the project's SSH key on this PC, if it doesn't exist yet. It is the key the VM is
# logged into with, so it has to exist before the VM is created (create-vm.sh).

source helpers/settings.sh
source helpers/vm-helpers.sh

ensure_ssh_key "$PROJECT_NAME"
