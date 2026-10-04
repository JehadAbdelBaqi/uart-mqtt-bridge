#!/usr/bin/env bash
set -euo pipefail

# Builds the firmware with the files in include/secrets/ and uploads it to the board.
# The board must be plugged in, with no serial monitor holding its port.

source helpers/settings.sh
source helpers/firmware-helpers.sh

build_and_upload_firmware "$PLATFORMIO" ".."
