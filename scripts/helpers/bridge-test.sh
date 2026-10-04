# Stops the script unless a line from the bridge arrives on the uplink topic in time.
# The lines come from the bridge's dummy data source, so this proves the bridge started,
# joined Wi-Fi, connected to the broker and published a line read from its UART.
# Arguments: <project name> <address> <port> <uplink topic> <seconds to wait>
check_bridge_uplink() {
    local project_name="$1"
    local address="$2"
    local port="$3"
    local up_topic="$4"
    local timeout="$5"
    local cert_dir="$HOME/certs/$project_name"
    local connection=(-h "$address" -p "$port" --cafile "$cert_dir/ca.crt" --cert "$cert_dir/client.crt" --key "$cert_dir/client.key")
    local line

    echo "Waiting up to $timeout s for a line from the bridge on $up_topic..."
    if ! line=$(mosquitto_sub "${connection[@]}" -t "$up_topic" -C 1 -W "$timeout"); then
        echo "No line arrived from the bridge." >&2
        echo "Check the jumper from TX to RX, and that DUMMY_LINE_INTERVAL_MS in include/config.h isn't 0." >&2
        exit 1
    fi

    echo "Received: $line"
    echo "Uplink works."
}

# Stops the script unless a message published to the downlink topic comes back on the uplink topic.
# The message goes down to the bridge's UART, across the jumper, and back up by its first letter.
# Arguments: <project name> <address> <port> <uplink topic> <downlink topic> <seconds to listen>
check_bridge_loopback() {
    local project_name="$1"
    local address="$2"
    local port="$3"
    local up_topic="$4"
    local down_topic="$5"
    local window="$6"
    local cert_dir="$HOME/certs/$project_name"
    local connection=(-h "$address" -p "$port" --cafile "$cert_dir/ca.crt" --cert "$cert_dir/client.crt" --key "$cert_dir/client.key")
    local message="T,e2e-$RANDOM"
    local received
    received=$(mktemp)

    echo "Publishing '$message' to $down_topic and listening on $up_topic for $window s..."

    # Listen first, in the background, so the message can't come back before anything is listening
    # mosquitto_sub's own "Timed out" at the end of its listening time is not shown
    mosquitto_sub "${connection[@]}" -t "$up_topic" -W "$window" > "$received" 2> /dev/null &
    sleep 2
    mosquitto_pub "${connection[@]}" -t "$down_topic" -m "$message" -q 1
    wait || true  # mosquitto_sub ends with an error code when its time is up

    if ! grep -Fxq "$message" "$received"; then
        echo "'$message' did not come back on $up_topic." >&2
        echo "Check the jumper from TX to RX." >&2
        rm -f "$received"
        exit 1
    fi

    rm -f "$received"
    echo "Came back: $message"
    echo "Downlink and loopback work."
}
