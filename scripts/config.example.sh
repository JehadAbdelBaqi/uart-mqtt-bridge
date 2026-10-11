# The values someone has to choose. Copy this file to config.sh and fill in your own.
# shellcheck disable=SC2034  # the values are used by the scripts that load this file

# Names the folders the SSH key and certificates are kept in
PROJECT_NAME="<your_project_name>"

# Name of the Multipass VM that runs the broker
VM_NAME="<your_vm_name>"

# Port the broker listens on
BROKER_PORT=8883

# Wi-Fi network the bridge joins
WIFI_SSID="<your_wifi_network_name>"
WIFI_PASSWORD="<your_wifi_password>"
