#!/usr/bin/env bash
set -euo pipefail

# Removes the PC's port rule for the broker's port, if there is one.
# Asks for the sudo password.

source helpers/settings.sh
# shellcheck source=helpers/network-helpers.sh
source "$NETWORK_HELPERS"

remove_port_rule "$BROKER_PORT"
