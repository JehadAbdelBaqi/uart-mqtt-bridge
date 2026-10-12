# Loads the settings every script needs. How it works: README.md in the folder above.
# shellcheck disable=SC2034  # the values are used by the scripts that load this file

# 1 when a project named its own firmware config, 0 for the bridge standing alone
BUILT_FOR_PROJECT="${FIRMWARE_CONFIG:+1}"
BUILT_FOR_PROJECT="${BUILT_FOR_PROJECT:-0}"

# The config files: the bridge's own, unless a project names its own (full paths)
SCRIPT_CONFIG="${SCRIPT_CONFIG:-config.sh}"
FIRMWARE_CONFIG="${FIRMWARE_CONFIG:-$(cd ../include && pwd)/config.h}"

# shellcheck source=config.example.sh
source "$SCRIPT_CONFIG"

# The same for every setup
PLATFORMIO="$HOME/.platformio/penv/bin/platformio"
ANSIBLE_PLAYBOOK="ansible-playbook"
