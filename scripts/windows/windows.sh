#!/usr/bin/env bash
set -euo pipefail

# Runs one of the scripts on Windows: does the Windows-only parts, then runs the script in WSL.
# Run from scripts/, in a Git Bash opened as administrator, with the script and its options as the arguments:
#   bash windows/windows.sh e2e.sh
#   bash windows/windows.sh steps/setup-certs.sh --keep-alive

# shellcheck source=windows/config.example.sh
source windows/config.sh
source windows/helpers.sh

if [ "$#" -eq 0 ]; then
    echo "Give the script to run, e.g.: bash windows/windows.sh e2e.sh" >&2
    exit 1
fi

# 1. Checks, before anything is changed
check_administrator

# WSL's network adapter only exists while WSL is running
wsl -e true

check_adapter_exists "$WSL_ADAPTER"
check_adapter_exists "$VM_ADAPTER"
check_adapter_exists "$LAN_ADAPTER"

# 2. Forwarding between WSL's network and the VM's, which SSH from WSL to the VM needs.
# The trap switches it off when this script ends, whether the run finishes or fails.
trap 'set_forwarding disabled "$WSL_ADAPTER" "$VM_ADAPTER"' EXIT
set_forwarding enabled "$WSL_ADAPTER" "$VM_ADAPTER"

# 3. The script itself, in WSL, told to use the Windows settings
wsl env SYSTEM_SETTINGS=windows/settings.sh bash "$@"
