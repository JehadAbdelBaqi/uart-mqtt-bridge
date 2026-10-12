# Running Ansible against the VM.

# Runs a playbook on the VM, logging in as the deploy user with the project's SSH key.
# The VM is named by its address on the command line, so no inventory file is kept.
# Arguments: <ansible-playbook command> <playbook> <project name> <vm address>
#            [values for the playbook, each as name=value]
run_playbook() {
    local ansible_playbook="$1"
    local playbook="$2"
    local project_name="$3"
    local vm_address="$4"
    shift 4
    local values=()
    local value

    for value in "$@"; do
        values+=(--extra-vars "$value")
    done

    echo "Running $playbook on $vm_address..."
    # The comma after the address is what tells Ansible it is a list of machines and not a file
    "$ansible_playbook" "$playbook" \
        --inventory "$vm_address," \
        --user deploy \
        --private-key "$HOME/.ssh/$project_name/id_ed25519" \
        --ssh-common-args "-o IdentitiesOnly=yes -o StrictHostKeyChecking=accept-new" \
        "${values[@]}"
}
