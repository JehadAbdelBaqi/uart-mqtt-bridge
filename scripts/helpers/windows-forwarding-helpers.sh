# Stops the script if the network adapter doesn't exist.
# Arguments: <adapter name>
check_adapter_exists() {
    echo "Checking network adapter '$1' exists..."
    if ! netsh interface ipv4 show interface "$1" > /dev/null 2>&1; then
        echo "Network adapter '$1' not found." >&2
        echo "Run 'netsh interface ipv4 show interfaces' to see the adapter names on this PC." >&2
        exit 1
    fi
}

# Switches forwarding on or off on each adapter. Needs administrator rights.
# Arguments: <enabled|disabled> <adapter name>...
set_forwarding() {
    local state="$1"
    shift

    for adapter in "$@"; do
        echo "Setting forwarding to $state for adapter '$adapter'..."
        netsh interface ipv4 set interface "$adapter" forwarding="$state"
    done
}
