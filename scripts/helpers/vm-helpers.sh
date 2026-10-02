# Finds the address of a Multipass VM and puts it in VM_ADDRESS.
# Arguments: <vm name> <multipass command>
find_vm_address() {
    local vm_name="$1"
    local multipass="$2"

    echo "Looking up the address of VM '$vm_name'..."

    # tr removes the Windows line endings in multipass.exe's output
    VM_ADDRESS=$("$multipass" list --format csv | tr -d '\r' | awk -F, -v name="$vm_name" '$1 == name { print $3 }')

    if [ -z "$VM_ADDRESS" ]; then
        echo "No address found for VM '$vm_name'." >&2
        echo "Run '$multipass list' to check that the VM exists and is running." >&2
        exit 1
    fi

    echo "$vm_name is at $VM_ADDRESS"
}

# Creates the project's SSH key in ~/.ssh/<project name>/ if it doesn't exist yet.
# Arguments: <wsl prefix> <project name>
ensure_ssh_key() {
    local wsl_prefix="$1"
    local project_name="$2"
    local key_dir="~/.ssh/$project_name"

    echo "Checking for an SSH key in $key_dir..."
    if $wsl_prefix bash -c "test -f $key_dir/id_ed25519"; then
        echo "SSH key found."
    else
        echo "No SSH key found. Creating one..."
        $wsl_prefix bash -c "mkdir -p $key_dir && chmod 700 ~/.ssh $key_dir"
        $wsl_prefix bash -c "ssh-keygen -t ed25519 -N '' -C '$project_name' -f $key_dir/id_ed25519"
    fi
}

# Adds the project's public SSH key to the VM's authorized_keys if it isn't there yet.
# Arguments: <wsl prefix> <project name> <multipass command> <vm name>
authorize_ssh_key() {
    local wsl_prefix="$1"
    local project_name="$2"
    local multipass="$3"
    local vm_name="$4"
    local public_key

    echo "Reading the public key..."
    public_key=$($wsl_prefix bash -c "cat ~/.ssh/$project_name/id_ed25519.pub" | tr -d '\r')

    echo "Checking whether VM '$vm_name' already has the key..."
    if "$multipass" exec "$vm_name" -- bash -c "grep -qF '$public_key' ~/.ssh/authorized_keys"; then
        echo "Key already authorised."
    else
        echo "Adding the key to VM '$vm_name'..."
        "$multipass" exec "$vm_name" -- bash -c "echo '$public_key' >> ~/.ssh/authorized_keys"
        echo "Key added."
    fi
}

# Stops the script if the VM can't be logged into over SSH with the project's key.
# Arguments: <wsl prefix> <ssh options> <vm address>
check_ssh() {
    local wsl_prefix="$1"
    local ssh_options="$2"
    local vm_address="$3"

    echo "Connecting to $vm_address over SSH..."
    $wsl_prefix bash -c "ssh $ssh_options ubuntu@$vm_address hostname"
    echo "SSH connection works."
}

# Copies the CA certificate, server certificate and server key to the VM,
# puts them where Mosquitto reads them, and restarts Mosquitto.
# Arguments: <wsl prefix> <project name> <ssh options> <vm address>
install_broker_certs() {
    local wsl_prefix="$1"
    local project_name="$2"
    local ssh_options="$3"
    local vm_address="$4"
    local cert_dir="~/certs/$project_name"

    echo "Copying the certificates to the VM..."
    $wsl_prefix bash -c "scp $ssh_options $cert_dir/ca.crt $cert_dir/server.crt $cert_dir/server.key ubuntu@$vm_address:"

    echo "Putting the certificates in Mosquitto's folders..."
    $wsl_prefix bash -c "ssh $ssh_options ubuntu@$vm_address 'sudo install -o root -g root -m 644 ~/ca.crt /etc/mosquitto/ca_certificates/ca.crt && sudo install -o root -g root -m 644 ~/server.crt /etc/mosquitto/certs/server.crt && sudo install -o mosquitto -g mosquitto -m 600 ~/server.key /etc/mosquitto/certs/server.key && rm ~/ca.crt ~/server.crt ~/server.key'"

    echo "Restarting Mosquitto..."
    $wsl_prefix bash -c "ssh $ssh_options ubuntu@$vm_address 'sudo systemctl restart mosquitto && systemctl is-active mosquitto'"
}
