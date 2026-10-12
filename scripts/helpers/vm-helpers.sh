# Finds the address of a Multipass VM and puts it in VM_ADDRESS.
# Arguments: <vm name>
find_vm_address() {
    local vm_name="$1"

    echo "Looking up the address of VM '$vm_name'..."

    VM_ADDRESS=$(multipass list --format csv | awk -F, -v name="$vm_name" '$1 == name { print $3 }')

    if [ -z "$VM_ADDRESS" ]; then
        echo "No address found for VM '$vm_name'." >&2
        echo "Run 'multipass list' to check that the VM exists and is running." >&2
        exit 1
    fi

    echo "$vm_name is at $VM_ADDRESS"
}

# Creates the project's SSH key in ~/.ssh/<project name>/ if it doesn't exist yet.
# Arguments: <project name>
ensure_ssh_key() {
    local project_name="$1"
    local key_dir="$HOME/.ssh/$project_name"

    echo "Checking for an SSH key in $key_dir..."
    if [ -f "$key_dir/id_ed25519" ]; then
        echo "SSH key found."
        return
    fi

    echo "No SSH key found. Creating one..."
    mkdir -p "$key_dir"
    chmod 700 "$HOME/.ssh" "$key_dir"
    ssh-keygen -t ed25519 -N '' -C "$project_name" -f "$key_dir/id_ed25519"
}

# Prints the cloud-init file for a new VM: the template, with the project's public SSH key put in.
# It is printed and not written to a file, so that it can be handed straight to Multipass, which
# cannot read a file in the PC's temporary folder.
# Arguments: <project name> <template file>
print_cloud_init() {
    local project_name="$1"
    local template="$2"
    local public_key

    public_key=$(cat "$HOME/.ssh/$project_name/id_ed25519.pub")

    # | as the separator: a public key has / in it
    sed "s|PUBLIC_SSH_KEY|$public_key|" "$template"
}
