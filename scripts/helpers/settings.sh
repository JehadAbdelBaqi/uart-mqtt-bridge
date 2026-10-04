# Loads everything the scripts need: the values a developer chose (config.sh), then the values
# that are the same for every setup. Every script loads this file, not config.sh itself.
# shellcheck disable=SC2034  # the values are used by the scripts that load this file

source config.sh

# Options given to every ssh and scp command: use the project's key, never stop to ask a question
SSH_OPTIONS="-i $HOME/.ssh/$PROJECT_NAME/id_ed25519 -o IdentitiesOnly=yes -o BatchMode=yes -o ConnectTimeout=5 -o StrictHostKeyChecking=accept-new"

# The Multipass command
MULTIPASS="multipass"

# The PlatformIO command (used by build-and-upload.sh), from PlatformIO's own folder in the home directory
PLATFORMIO="$HOME/.platformio/penv/bin/platformio"

# The file holding the functions for the PC's LAN address and the port rule
NETWORK_HELPERS="helpers/network-helpers.sh"

# Name of the PC's network adapter on the LAN. Not needed here: the address is found without it
LAN_ADAPTER=""

# A start script for another system names its own settings file in SYSTEM_SETTINGS;
# the values in that file take the place of the ones above.
if [ -n "${SYSTEM_SETTINGS:-}" ]; then
    # shellcheck source=/dev/null
    source "$SYSTEM_SETTINGS"
fi
