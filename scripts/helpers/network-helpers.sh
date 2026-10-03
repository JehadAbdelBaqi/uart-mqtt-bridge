# The Linux versions of these functions; helpers/windows-network-helpers.sh holds the Windows ones.
# NETWORK_HELPERS in config.sh chooses which file the scripts use.

# Finds the PC's LAN address - the one it uses to reach the internet - and puts it in LAN_ADDRESS.
# The adapter name is not used on Linux; it is accepted so both versions are called the same way.
# Arguments: <adapter name>
find_lan_address() {
    echo "Looking up the PC's LAN address..."

    LAN_ADDRESS=$(ip -4 route get 1.1.1.1 | awk '{ for (i = 1; i < NF; i++) if ($i == "src") print $(i + 1) }')

    if [ -z "$LAN_ADDRESS" ]; then
        echo "No LAN address found." >&2
        echo "Check that the PC is connected to a network." >&2
        exit 1
    fi

    echo "The PC's LAN address is $LAN_ADDRESS"
}

# Deletes every rule carrying a tag from one iptables chain. Needs sudo.
# Arguments: <table> <chain> <tag>
delete_tagged_rules() {
    local table="$1"
    local chain="$2"
    local tag="$3"

    # Each rule is listed as the command that added it (-A ...); the same command with -D deletes it.
    # $rule is left unquoted on purpose: each part of it has to reach iptables as a separate word
    # shellcheck disable=SC2086
    sudo iptables -t "$table" -S "$chain" | grep -- "--comment $tag" | sed 's/^-A/-D/' | while read -r rule; do sudo iptables -t "$table" $rule; done || true
}

# Passes connections that arrive at the PC's LAN address on a port to the same port on the VM.
# Replaces any earlier rule for that port. Needs sudo.
# Arguments: <port> <lan address> <vm address>
set_port_rule() {
    local port="$1"
    local lan_address="$2"
    local vm_address="$3"
    local tag="broker-port-$port"

    echo "Pointing port $port on $lan_address at $vm_address..."

    remove_port_rule "$port"

    sudo sysctl -q -w net.ipv4.ip_forward=1
    sudo iptables -t nat -A PREROUTING -d "$lan_address" -p tcp --dport "$port" -m comment --comment "$tag" -j DNAT --to-destination "$vm_address:$port"
    # PREROUTING only sees connections arriving from other devices; OUTPUT covers ones made from this PC
    sudo iptables -t nat -A OUTPUT -d "$lan_address" -p tcp --dport "$port" -m comment --comment "$tag" -j DNAT --to-destination "$vm_address:$port"
    sudo iptables -I FORWARD -d "$vm_address" -p tcp --dport "$port" -m comment --comment "$tag" -j ACCEPT

    echo "Port rule set."
}

# Removes the rule that passes a port on the PC to the VM, if there is one. Needs sudo.
# Arguments: <port>
remove_port_rule() {
    local port="$1"
    local tag="broker-port-$port"

    echo "Removing the port rule for port $port..."

    delete_tagged_rules nat PREROUTING "$tag"
    delete_tagged_rules nat OUTPUT "$tag"
    delete_tagged_rules filter FORWARD "$tag"

    echo "Port rule removed."
}
