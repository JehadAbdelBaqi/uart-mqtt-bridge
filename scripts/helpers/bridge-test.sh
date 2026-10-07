# Tests made through the broker. The first two work for any topics and any lines, so a project
# that uses the bridge can test its own device with them; the last two are the bridge's own test.

# Waits for one line to arrive on a topic, and shows it.
# Arguments: <project name> <address> <port> <topic> <seconds to wait>
# Returns: 1 if no line arrived in time
check_line_arrives() {
    local project_name="$1"
    local address="$2"
    local port="$3"
    local topic="$4"
    local timeout="$5"
    local cert_dir="$HOME/certs/$project_name"
    local connection=(-h "$address" -p "$port" --cafile "$cert_dir/ca.crt" --cert "$cert_dir/client.crt" --key "$cert_dir/client.key")
    local line

    echo "Waiting up to $timeout s for a line on $topic..."
    if ! line=$(mosquitto_sub "${connection[@]}" -t "$topic" -C 1 -W "$timeout"); then
        echo "No line arrived on $topic." >&2
        return 1
    fi

    echo "Received: $line"
}

# Publishes a message to one topic and checks that an expected line arrives on another.
# Arguments: <project name> <address> <port> <topic to publish to> <message> <topic to listen on>
#            <expected line> <seconds to listen>
# Returns: 1 if the expected line did not arrive in time
check_exchange() {
    local project_name="$1"
    local address="$2"
    local port="$3"
    local publish_topic="$4"
    local message="$5"
    local listen_topic="$6"
    local expected="$7"
    local window="$8"
    local cert_dir="$HOME/certs/$project_name"
    local connection=(-h "$address" -p "$port" --cafile "$cert_dir/ca.crt" --cert "$cert_dir/client.crt" --key "$cert_dir/client.key")
    local received
    received=$(mktemp)

    echo "Publishing '$message' to $publish_topic and listening on $listen_topic for $window s..."

    # Listen first, in the background, so the answer can't arrive before anything is listening
    # mosquitto_sub's own "Timed out" at the end of its listening time is not shown
    mosquitto_sub "${connection[@]}" -t "$listen_topic" -W "$window" > "$received" 2> /dev/null &
    sleep 2
    mosquitto_pub "${connection[@]}" -t "$publish_topic" -m "$message" -q 1
    wait || true  # mosquitto_sub ends with an error code when its time is up

    if ! grep -Fxq -- "$expected" "$received"; then
        echo "'$expected' did not arrive on $listen_topic. What did arrive:" >&2
        cat "$received" >&2
        rm -f "$received"
        return 1
    fi

    rm -f "$received"
    echo "Arrived: $expected"
}

# Stops the script unless a line from the bridge arrives on the uplink topic in time.
# The lines come from the bridge's dummy data source, so this proves the bridge started,
# joined Wi-Fi, connected to the broker and published a line read from its UART.
# Arguments: <project name> <address> <port> <uplink topic> <seconds to wait>
check_bridge_uplink() {
    if ! check_line_arrives "$1" "$2" "$3" "$4" "$5"; then
        echo "Check the jumper from TX to RX, and that DUMMY_LINE_INTERVAL_MS in include/config.h isn't 0." >&2
        exit 1
    fi

    echo "Uplink works."
}

# Stops the script unless a message published to the downlink topic comes back on the uplink topic.
# The message goes down to the bridge's UART, across the jumper, and back up by its first letter.
# Arguments: <project name> <address> <port> <uplink topic> <downlink topic> <seconds to listen>
check_bridge_loopback() {
    local up_topic="$4"
    local down_topic="$5"
    local message="T,e2e-$RANDOM"

    if ! check_exchange "$1" "$2" "$3" "$down_topic" "$message" "$up_topic" "$message" "$6"; then
        echo "Check the jumper from TX to RX." >&2
        exit 1
    fi

    echo "Downlink and loopback work."
}
