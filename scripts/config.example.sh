# Settings for the test broker setup scripts.
# Copy this file to config.sh and fill in your own values; the scripts read config.sh.

# Name of the project these scripts are set up for; its SSH key is kept in ~/.ssh/<PROJECT_NAME>/
PROJECT_NAME="your_project_name"

# Name of the Multipass VM that runs the broker
VM_NAME="your_vm_name"

# Port the broker listens on; the PC passes connections on this port to the VM
BROKER_PORT=8883

# Windows only: name of the firewall rule that lets the local network reach BROKER_PORT
FIREWALL_RULE_NAME="$PROJECT_NAME broker port $BROKER_PORT"

# Options given to every ssh and scp command: use the project's key, never stop to ask a question
SSH_OPTIONS="-i ~/.ssh/$PROJECT_NAME/id_ed25519 -o IdentitiesOnly=yes -o BatchMode=yes -o ConnectTimeout=5 -o StrictHostKeyChecking=accept-new"

# true when the scripts are started from Windows (Git Bash) and the Linux part runs in WSL
IS_WINDOWS=true_or_false

# The Multipass command. On Windows the program is called by its .exe name,
# which works from both Git Bash and WSL.
if [ "$IS_WINDOWS" = true ]; then
    MULTIPASS="multipass.exe"
    WSL_PREFIX="wsl"
else
    MULTIPASS="multipass"
    WSL_PREFIX=""
fi

# Windows only: the PC's network adapters for WSL's network and the VM's network
WSL_ADAPTER="your_wsl_adapter_name"
VM_ADAPTER="your_vm_adapter_name"

# Windows only: the PC's adapter on the network the device connects through
LAN_ADAPTER="your_lan_adapter_name"
