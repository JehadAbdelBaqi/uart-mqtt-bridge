#!/usr/bin/env bash
set -euo pipefail

# Runs the firmware's unit tests on the PC: the application code, with stand-ins for the board.
# No board is needed.

source helpers/settings.sh
source helpers/firmware-helpers.sh

run_unit_tests "$PLATFORMIO" ".."
