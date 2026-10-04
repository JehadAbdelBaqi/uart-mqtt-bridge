# The Windows values that take the place of the Linux ones in helpers/settings.sh.
# windows.sh names this file when it runs a script in WSL; it is loaded from scripts/.
# shellcheck disable=SC2034  # the values are used by the scripts that load this file

# The values this PC's developer chose: the PlatformIO command and the adapter names
# shellcheck source=windows/config.example.sh
source windows/config.sh

# The Multipass command: Windows' own, called from WSL
MULTIPASS="multipass.exe"

# The file holding the Windows functions for the PC's LAN address and the port rule
NETWORK_HELPERS="windows/network-helpers.sh"
