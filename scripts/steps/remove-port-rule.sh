#!/usr/bin/env bash
set -euo pipefail

# Removes the PC's port rule for the broker's port, if there is one.
# Asks for the sudo password.

source helpers/settings.sh
source helpers/network-helpers.sh

remove_port_rule "$BROKER_PORT"
