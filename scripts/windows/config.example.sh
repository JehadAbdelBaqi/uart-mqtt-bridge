# The values only a Windows PC needs. Copy this file to windows/config.sh and fill in your own
# values, as well as config.sh in the folder above.
# shellcheck disable=SC2034  # the values are used by the scripts that load this file

# The PlatformIO command, from PlatformIO's own folder in the Windows user's home directory.
# The scripts run in WSL, so the path starts at /mnt/c
PLATFORMIO="/mnt/c/Users/<your_windows_user_name>/.platformio/penv/Scripts/platformio.exe"

# The PC's network adapters for WSL's network and the VM's network.
# 'netsh interface ipv4 show interfaces' lists the adapter names on this PC
WSL_ADAPTER="<your_wsl_adapter_name>"
VM_ADAPTER="<your_vm_adapter_name>"

# The PC's adapter on the network the device connects through
LAN_ADAPTER="<your_lan_adapter_name>"
