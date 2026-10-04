# The values only a Windows PC needs. Copy this file to windows/config.sh and fill in your own.
# shellcheck disable=SC2034  # the values are used by the scripts that load this file

# The PlatformIO command, as WSL sees it
PLATFORMIO="/mnt/c/Users/<your_windows_user_name>/.platformio/penv/Scripts/platformio.exe"

# The PC's adapters for WSL's network and the VM's network
WSL_ADAPTER="<your_wsl_adapter_name>"
VM_ADAPTER="<your_vm_adapter_name>"

# The PC's adapter on the network the device connects through
LAN_ADAPTER="<your_lan_adapter_name>"
