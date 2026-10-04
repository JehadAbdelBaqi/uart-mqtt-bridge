# Loads the settings every script needs. How it works: README.md in the folder above.
# shellcheck disable=SC2034  # the values are used by the scripts that load this file

# The config files: the bridge's own, unless a project names its own (full paths)
SCRIPT_CONFIG="${SCRIPT_CONFIG:-config.sh}"
FIRMWARE_CONFIG="${FIRMWARE_CONFIG:-$(cd ../include && pwd)/config.h}"

# shellcheck source=config.example.sh
source "$SCRIPT_CONFIG"

# The same for every setup
SSH_OPTIONS="-i $HOME/.ssh/$PROJECT_NAME/id_ed25519 -o IdentitiesOnly=yes -o BatchMode=yes -o ConnectTimeout=5 -o StrictHostKeyChecking=accept-new"
MULTIPASS="multipass"
PLATFORMIO="$HOME/.platformio/penv/bin/platformio"
NETWORK_HELPERS="helpers/network-helpers.sh"
LAN_ADAPTER=""  # not needed: the address is found without it

# Another system's start script names its own settings file here
if [ -n "${SYSTEM_SETTINGS:-}" ]; then
    # shellcheck source=/dev/null
    source "$SYSTEM_SETTINGS"
fi
