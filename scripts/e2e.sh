#!/usr/bin/env bash
set -euo pipefail

# Runs the steps switched on in build-config.sh, in order, and ends with a short report of them.
# How it works: README.md in this folder.

source helpers/settings.sh
source helpers/report.sh
# shellcheck source=helpers/network-helpers.sh
source "$NETWORK_HELPERS"
# The bridge's own build config, unless a project names its own
# shellcheck source=build-config.sh
source "${BUILD_CONFIG:-build-config.sh}"

if [ "$#" -gt 0 ]; then
    echo "e2e.sh takes no options: set the steps to run in build-config.sh." >&2
    exit 1
fi

# Run by a project's own build, these steps go into its report, marked as the bridge's
report_start
NAME_PREFIX=""
if [ "$REPORT_OWNER" = 0 ]; then
    NAME_PREFIX="bridge: "
fi

# Runs when this script ends, whether it finished or failed: the port rule stays while the steps
# run and is removed here, and the report is the last thing printed.
finish() {
    if [ "$KEEP_PORT_RULE" = 0 ]; then
        remove_port_rule "$BROKER_PORT"
    fi
    report_print
}
trap finish EXIT

# Each entry is a switch, then the step it runs. After a step fails, the rest are not run.
FAILED=0
for step in "${STEPS[@]}"; do
    run="${step%% *}"
    script="${step#* }"
    name="$NAME_PREFIX$(report_step_name "$script")"

    if [ "$run" != 1 ]; then
        continue
    fi
    if [ "$FAILED" = 1 ]; then
        report_add "$name" "not run"
        continue
    fi

    # shellcheck disable=SC2086  # a step's options are separate words
    report_run_step "$name" bash $script || FAILED=1
done

if [ "$FAILED" = 1 ]; then
    exit 1
fi

echo "Done."
