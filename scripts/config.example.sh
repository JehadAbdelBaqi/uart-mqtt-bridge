# The values the setup scripts need that someone has to choose, including the ones they write
# into the firmware's files. Copy this file to config.sh and fill in your own values.
# On Windows, also fill in windows/config.sh (see windows/config.example.sh).
# shellcheck disable=SC2034  # the values are used by the scripts that load this file

# Name of the project these scripts are set up for; its SSH key is kept in ~/.ssh/<PROJECT_NAME>/
# and its certificates in ~/certs/<PROJECT_NAME>/
PROJECT_NAME="<your_project_name>"

# Name of the Multipass VM that runs the broker
VM_NAME="<your_vm_name>"

# Port the broker listens on; the PC passes connections on this port to the VM
BROKER_PORT=8883

# Wi-Fi network the bridge joins; written into include/secrets/wifi.h for the firmware
WIFI_SSID="<your_wifi_network_name>"
WIFI_PASSWORD="<your_wifi_password>"
