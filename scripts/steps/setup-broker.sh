#!/usr/bin/env bash
set -euo pipefail

# Sets the broker up in the VM named in config.sh, with Ansible: Mosquitto installed, set to
# require TLS and a client certificate, with its certificates in place.
# The VM and the certificates must exist (create-vm.sh, setup-certs.sh). Safe to run again: only
# what differs is changed.

source helpers/settings.sh
source helpers/vm-helpers.sh
source helpers/ansible-helpers.sh

find_vm_address "$VM_NAME"
run_playbook "$ANSIBLE_PLAYBOOK" "ansible/broker.yml" "$PROJECT_NAME" "$VM_ADDRESS" "broker_port=$BROKER_PORT" "cert_dir=$HOME/certs/$PROJECT_NAME"
