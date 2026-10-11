#!/usr/bin/env bash
set -euo pipefail

# Deletes the firmware's build output, so the next build sets itself up from scratch.

source helpers/settings.sh
source helpers/firmware-helpers.sh

delete_build_output ".."
