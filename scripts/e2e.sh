#!/usr/bin/env bash
set -euo pipefail

# Runs the steps switched on in build-config.sh, in order.
# How it works: README.md in this folder.

source helpers/settings.sh
# shellcheck source=helpers/network-helpers.sh
source "$NETWORK_HELPERS"
# The bridge's own build config, unless a project names its own
# shellcheck source=build-config.sh
source "${BUILD_CONFIG:-build-config.sh}"

if [ "$#" -gt 0 ]; then
    echo "e2e.sh takes no options: set the steps to run in build-config.sh." >&2
    exit 1
fi

# The port rule stays while the steps run; the trap removes it when this script ends
if [ "$KEEP_PORT_RULE" = 0 ]; then
    trap 'remove_port_rule "$BROKER_PORT"' EXIT
fi

# Each entry is a switch, then the step it runs
for step in "${STEPS[@]}"; do
    run="${step%% *}"
    script="${step#* }"

    if [ "$run" = 1 ]; then
        # shellcheck disable=SC2086  # a step's options are separate words
        bash $script
    fi
done

echo "Done."
