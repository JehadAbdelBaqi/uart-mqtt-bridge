# The Windows versions of the functions in network-helpers.sh, chosen by NETWORK_HELPERS in config.sh.
# They run inside WSL and call Windows' own netsh.exe, so WSL has to have been started from a
# terminal with administrator rights (windows.sh does that).

# Finds the PC's LAN address from the named adapter and puts it in LAN_ADDRESS.
# Arguments: <adapter name>
find_lan_address() {
    local adapter="$1"

    echo "Looking up the PC's LAN address..."

    LAN_ADDRESS=$(netsh.exe interface ipv4 show addresses "$adapter" | tr -d '\r' | awk '/IP Address/ { print $NF }')

    if [ -z "$LAN_ADDRESS" ]; then
        echo "No LAN address found." >&2
        echo "Check that the PC is connected to a network, and that the adapter name '$adapter' is right." >&2
        exit 1
    fi

    echo "The PC's LAN address is $LAN_ADDRESS"
}

# Passes connections that arrive at the PC's LAN address on a port to the same port on the VM,
# and lets devices on the local network through the firewall on that port.
# Replaces any earlier rule for that port.
# Arguments: <port> <lan address> <vm address>
set_port_rule() {
    local port="$1"
    local lan_address="$2"
    local vm_address="$3"
    local rule_name="broker-port-$port"

    echo "Pointing port $port on $lan_address at $vm_address..."

    remove_port_rule "$port"

    netsh.exe interface portproxy add v4tov4 listenport="$port" listenaddress="$lan_address" connectport="$port" connectaddress="$vm_address"
    netsh.exe advfirewall firewall add rule name="$rule_name" dir=in action=allow protocol=TCP localport="$port" remoteip=localsubnet

    echo "Port rule set."
}

# Removes the rule that passes a port on the PC to the VM, and its firewall rule, if they exist.
# Arguments: <port>
remove_port_rule() {
    local port="$1"
    local rule_name="broker-port-$port"

    echo "Removing the port rule for port $port..."

    for old_address in $(netsh.exe interface portproxy show v4tov4 | tr -d '\r' | awk -v port="$port" '$2 == port { print $1 }'); do
        netsh.exe interface portproxy delete v4tov4 listenport="$port" listenaddress="$old_address" > /dev/null
    done

    if netsh.exe advfirewall firewall show rule name="$rule_name" > /dev/null 2>&1; then
        netsh.exe advfirewall firewall delete rule name="$rule_name" > /dev/null
    fi

    echo "Port rule removed."
}
