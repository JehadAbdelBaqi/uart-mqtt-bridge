# Finds the PC's LAN address and puts it in LAN_ADDRESS.
# On Windows it is read from the named adapter; on Linux the adapter name is not used.
# Arguments: <is windows: true|false> <adapter name>
find_lan_address() {
    local is_windows="$1"
    local adapter="$2"

    echo "Looking up the PC's LAN address..."

    if [ "$is_windows" = true ]; then
        LAN_ADDRESS=$(netsh interface ipv4 show addresses "$adapter" | tr -d '\r' | awk '/IP Address/ { print $NF }')
    else
        LAN_ADDRESS=$(ip -4 route get 1.1.1.1 | awk '{ for (i = 1; i < NF; i++) if ($i == "src") print $(i + 1) }')
    fi

    if [ -z "$LAN_ADDRESS" ]; then
        echo "No LAN address found." >&2
        echo "Check that the PC is connected to a network, and on Windows that the adapter name '$adapter' is right." >&2
        exit 1
    fi

    echo "The PC's LAN address is $LAN_ADDRESS"
}

# Passes connections that arrive at the PC's LAN address on a port to the same port on the VM.
# Replaces any earlier rule for that port. Needs administrator rights (Windows) or sudo (Linux).
# Arguments: <is windows: true|false> <port> <lan address> <vm address>
set_port_rule() {
    local is_windows="$1"
    local port="$2"
    local lan_address="$3"
    local vm_address="$4"
    local tag="broker-port-$port"

    echo "Pointing port $port on $lan_address at $vm_address..."

    if [ "$is_windows" = true ]; then
        for old_address in $(netsh interface portproxy show v4tov4 | tr -d '\r' | awk -v port="$port" '$2 == port { print $1 }'); do
            netsh interface portproxy delete v4tov4 listenport="$port" listenaddress="$old_address" > /dev/null
        done
        netsh interface portproxy add v4tov4 listenport="$port" listenaddress="$lan_address" connectport="$port" connectaddress="$vm_address"
    else
        sudo sysctl -q -w net.ipv4.ip_forward=1
        sudo iptables -t nat -S PREROUTING | grep -- "--comment $tag" | sed 's/^-A/-D/' | while read -r rule; do sudo iptables -t nat $rule; done || true
        sudo iptables -S FORWARD | grep -- "--comment $tag" | sed 's/^-A/-D/' | while read -r rule; do sudo iptables $rule; done || true
        sudo iptables -t nat -A PREROUTING -d "$lan_address" -p tcp --dport "$port" -m comment --comment "$tag" -j DNAT --to-destination "$vm_address:$port"
        sudo iptables -I FORWARD -d "$vm_address" -p tcp --dport "$port" -m comment --comment "$tag" -j ACCEPT
    fi

    echo "Port rule set."
}

# Lets devices on the local network connect to the PC on a port, if no rule for it exists yet.
# Needs administrator rights on Windows. On Linux the port rule already lets the traffic through.
# Arguments: <is windows: true|false> <rule name> <port>
ensure_firewall_rule() {
    local is_windows="$1"
    local rule_name="$2"
    local port="$3"

    if [ "$is_windows" != true ]; then
        echo "No separate firewall rule needed on Linux."
        return
    fi

    echo "Checking for firewall rule '$rule_name'..."
    if netsh advfirewall firewall show rule name="$rule_name" > /dev/null 2>&1; then
        echo "Firewall rule found."
    else
        echo "No firewall rule found. Creating one..."
        netsh advfirewall firewall add rule name="$rule_name" dir=in action=allow protocol=TCP localport="$port" remoteip=localsubnet
    fi
}

# Removes the rule that passes a port on the PC to the VM, if there is one.
# Needs administrator rights (Windows) or sudo (Linux).
# Arguments: <is windows: true|false> <port>
remove_port_rule() {
    local is_windows="$1"
    local port="$2"
    local tag="broker-port-$port"

    echo "Removing the port rule for port $port..."

    if [ "$is_windows" = true ]; then
        for old_address in $(netsh interface portproxy show v4tov4 | tr -d '\r' | awk -v port="$port" '$2 == port { print $1 }'); do
            netsh interface portproxy delete v4tov4 listenport="$port" listenaddress="$old_address" > /dev/null
        done
    else
        sudo iptables -t nat -S PREROUTING | grep -- "--comment $tag" | sed 's/^-A/-D/' | while read -r rule; do sudo iptables -t nat $rule; done || true
        sudo iptables -S FORWARD | grep -- "--comment $tag" | sed 's/^-A/-D/' | while read -r rule; do sudo iptables $rule; done || true
    fi

    echo "Port rule removed."
}

# Removes the firewall rule that lets the local network connect to the PC on a port, if there is one.
# Needs administrator rights on Windows. On Linux there is no separate firewall rule.
# Arguments: <is windows: true|false> <rule name>
remove_firewall_rule() {
    local is_windows="$1"
    local rule_name="$2"

    if [ "$is_windows" != true ]; then
        echo "No separate firewall rule to remove on Linux."
        return
    fi

    echo "Removing firewall rule '$rule_name'..."
    if netsh advfirewall firewall show rule name="$rule_name" > /dev/null 2>&1; then
        netsh advfirewall firewall delete rule name="$rule_name" > /dev/null
        echo "Firewall rule removed."
    else
        echo "No firewall rule found."
    fi
}
