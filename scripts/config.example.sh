# Everything the setup scripts need, including the values they write into the firmware's files.
# Copy this file to config.sh and fill in your own values; the scripts read config.sh.

# Name of the project these scripts are set up for; its SSH key is kept in ~/.ssh/<PROJECT_NAME>/
PROJECT_NAME="your_project_name"

# Name of the Multipass VM that runs the broker
VM_NAME="your_vm_name"

# Port the broker listens on; the PC passes connections on this port to the VM
BROKER_PORT=8883

# Wi-Fi network the bridge joins; written into include/secrets/wifi.h for the firmware
WIFI_SSID="your_wifi_network_name"
WIFI_PASSWORD="your_wifi_password"

# Options given to every ssh and scp command: use the project's key, never stop to ask a question
SSH_OPTIONS="-i $HOME/.ssh/$PROJECT_NAME/id_ed25519 -o IdentitiesOnly=yes -o BatchMode=yes -o ConnectTimeout=5 -o StrictHostKeyChecking=accept-new"

# The Multipass command. Comment out the one not being used.
# Linux
MULTIPASS="multipass"
# Windows
# MULTIPASS="multipass.exe"

# The PlatformIO command (used by build-and-upload.sh), from PlatformIO's own folder in the home directory.
# Comment out the one not being used.
# Linux
PLATFORMIO="$HOME/.platformio/penv/bin/platformio"
# Windows (the scripts run in WSL, so the path starts at /mnt/c)
# PLATFORMIO="/mnt/c/Users/your_windows_user_name/.platformio/penv/Scripts/platformio.exe"

# The file holding the functions for the PC's LAN address and the port rule.
# Comment out the one not being used.
# Linux
NETWORK_HELPERS="helpers/network-helpers.sh"
# Windows
# NETWORK_HELPERS="helpers/windows-network-helpers.sh"

# Windows only (windows.sh): the PC's network adapters for WSL's network and the VM's network
WSL_ADAPTER="your_wsl_adapter_name"
VM_ADAPTER="your_vm_adapter_name"

# Windows only: the PC's adapter on the network the device connects through
LAN_ADAPTER="your_lan_adapter_name"
