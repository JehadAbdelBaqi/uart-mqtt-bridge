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
# Arguments: <project name> <vm name> <broker port>
confirm_nuke() {
    local project_name="$1"
    local vm_name="$2"
    local port="$3"
    local answer
    local typed_name

    echo "This will permanently delete, if they exist:"
    echo "  - the VM '$vm_name' and everything on it"
    echo "  - ~/certs/$project_name (the CA, server and client certificates and their keys)"
    echo "  - ~/.ssh/$project_name (the project's SSH key)"
    echo "  - the firmware's files in include/secrets/ (wifi.h broker.h ca.crt client.crt client.key)"
    echo "  - the port rule for port $port"
    echo

    # The questions are printed with printf: read's own prompt is only shown when the input is a
    # terminal, which it isn't when the script is run through windows.sh
    printf "Are you sure you want to delete all of this? (y/n) "
    read -r answer
    if [ "$answer" != "y" ]; then
        echo "Nothing deleted."
        exit 1
    fi

    printf "Type the project name to confirm: "
    read -r typed_name
    if [ "$typed_name" != "$project_name" ]; then
        echo "That doesn't match the project name. Nothing deleted."
        exit 1
    fi

    echo
}

# Removes the VM's SSH identity from known_hosts, so a new VM given the same address is accepted.
# Does nothing if the VM has no address.
# Arguments: <multipass command> <vm name>
forget_vm_identity() {
    local multipass="$1"
    local vm_name="$2"
    local vm_address

    vm_address=$("$multipass" list --format csv | tr -d '\r' | awk -F, -v name="$vm_name" '$1 == name { print $3 }')
    if [[ ! "$vm_address" =~ ^[0-9.]+$ ]]; then
        return
    fi

    echo "Forgetting the VM's SSH identity for $vm_address..."
    ssh-keygen -R "$vm_address" > /dev/null 2>&1 || true
}

# Deletes the VM and everything on it, if it exists.
# Arguments: <multipass command> <vm name>
delete_vm() {
    local multipass="$1"
    local vm_name="$2"

    if ! "$multipass" info "$vm_name" > /dev/null 2>&1; then
        echo "VM '$vm_name' not found, skipping."
        return
    fi

    forget_vm_identity "$multipass" "$vm_name"

    echo "Deleting VM '$vm_name'..."
    "$multipass" delete --purge "$vm_name"
}

# Deletes a folder, if it exists.
# Arguments: <folder>
delete_folder() {
    local folder="$1"

    if [ ! -d "$folder" ]; then
        echo "$folder not found, skipping."
        return
    fi

    echo "Deleting $folder..."
    rm -rf -- "$folder"
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
