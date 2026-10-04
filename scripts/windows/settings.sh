# The Windows values that take the place of the Linux ones in helpers/settings.sh.
# shellcheck disable=SC2034  # the values are used by the scripts that load this file

# shellcheck source=windows/config.example.sh
source windows/config.sh

MULTIPASS="multipass.exe"
NETWORK_HELPERS="windows/network-helpers.sh"
