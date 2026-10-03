# Stops the script on Windows unless it was started from a terminal with administrator rights,
# which removing the port rule and the firewall rule needs. On Linux nothing is checked.
# Arguments: <is windows: true|false>
check_administrator() {
    local is_windows="$1"

    if [ "$is_windows" != true ]; then
        return
    fi

    if ! net session > /dev/null 2>&1; then
        echo "Run this from a Git Bash opened as administrator." >&2
        exit 1
    fi
}

# Stops the script unless the project name is a plain name (letters, numbers, - and _).
# The folders that get deleted are built from it, so anything else is refused.
# Arguments: <project name>
check_project_name() {
    local project_name="$1"

    if [[ ! "$project_name" =~ ^[A-Za-z0-9_-]+$ ]]; then
        echo "PROJECT_NAME in config.sh must be a plain name (letters, numbers, - and _)." >&2
        exit 1
    fi
}

# Lists what is about to be deleted and asks twice: y/n, then the project name typed in.
# Stops the script, with nothing deleted, unless both are answered.
# Arguments: <project name> <vm name> <broker port> <firewall rule name>
confirm_nuke() {
    local project_name="$1"
    local vm_name="$2"
    local port="$3"
    local rule_name="$4"
    local answer
    local typed_name

    echo "This will permanently delete, if they exist:"
    echo "  - the VM '$vm_name' and everything on it"
    echo "  - ~/certs/$project_name (the CA, server and client certificates and their keys)"
    echo "  - ~/.ssh/$project_name (the project's SSH key)"
    echo "  - the firmware's files in include/secrets/ (wifi.h broker.h ca.crt client.crt client.key)"
    echo "  - the port rule for port $port"
    echo "  - the firewall rule '$rule_name'"
    echo

    read -r -p "Are you sure you want to delete all of this? (y/n) " answer
    if [ "$answer" != "y" ]; then
        echo "Nothing deleted."
        exit 1
    fi

    read -r -p "Type the project name to confirm: " typed_name
    if [ "$typed_name" != "$project_name" ]; then
        echo "That doesn't match the project name. Nothing deleted."
        exit 1
    fi

    echo
}

# Removes the VM's SSH identity from known_hosts, so a new VM given the same address is accepted.
# Does nothing if the VM has no address.
# Arguments: <wsl prefix> <multipass command> <vm name>
forget_vm_identity() {
    local wsl_prefix="$1"
    local multipass="$2"
    local vm_name="$3"
    local vm_address

    vm_address=$("$multipass" list --format csv | tr -d '\r' | awk -F, -v name="$vm_name" '$1 == name { print $3 }')
    if [[ ! "$vm_address" =~ ^[0-9.]+$ ]]; then
        return
    fi

    echo "Forgetting the VM's SSH identity for $vm_address..."
    $wsl_prefix bash -c "ssh-keygen -R $vm_address" > /dev/null 2>&1 || true
}

# Deletes the VM and everything on it, if it exists.
# Arguments: <wsl prefix> <multipass command> <vm name>
delete_vm() {
    local wsl_prefix="$1"
    local multipass="$2"
    local vm_name="$3"

    if ! "$multipass" info "$vm_name" > /dev/null 2>&1; then
        echo "VM '$vm_name' not found, skipping."
        return
    fi

    forget_vm_identity "$wsl_prefix" "$multipass" "$vm_name"

    echo "Deleting VM '$vm_name'..."
    "$multipass" delete --purge "$vm_name"
}

# Deletes a folder in the home directory where the Linux tools run (WSL on Windows), if it exists.
# Arguments: <wsl prefix> <folder>
delete_folder() {
    local wsl_prefix="$1"
    local folder="$2"

    if ! $wsl_prefix bash -c "test -d $folder"; then
        echo "$folder not found, skipping."
        return
    fi

    echo "Deleting $folder..."
    $wsl_prefix bash -c "rm -rf -- $folder"
}

# Deletes the firmware's generated files from a folder: wifi.h, broker.h and the three certificate files.
# Arguments: <folder>
delete_firmware_files() {
    local folder="$1"

    for file in wifi.h broker.h ca.crt client.crt client.key; do
        if [ ! -f "$folder/$file" ]; then
            echo "$folder/$file not found, skipping."
            continue
        fi

        echo "Deleting $folder/$file..."
        rm -f -- "$folder/$file"
    done
}
