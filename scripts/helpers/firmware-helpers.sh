# Copies the files the firmware is built with - the CA certificate, the client certificate
# and the client key - into a folder, unchanged. Replaces any copies already there.
# Arguments: <wsl prefix> <project name> <destination folder>
copy_firmware_certs() {
    local wsl_prefix="$1"
    local project_name="$2"
    local destination="$3"
    local cert_dir="~/certs/$project_name"

    echo "Copying the firmware's certificates to $destination..."
    for file in ca.crt client.crt client.key; do
        $wsl_prefix bash -c "cat $cert_dir/$file" > "$destination/$file"
    done
    echo "Firmware certificates copied."
}

# Writes broker.h, the header that tells the firmware where the broker is.
# Replaces any copy already there.
# Arguments: <broker address> <broker port> <destination folder>
write_broker_header() {
    local address="$1"
    local port="$2"
    local destination="$3"

    echo "Writing $destination/broker.h for $address:$port..."
    cat > "$destination/broker.h" <<EOF
// Written by scripts/setup-certs.sh on every run - changes made here are overwritten.
#ifndef BROKER_H
#define BROKER_H

#define BROKER_ADDRESS "$address"
#define BROKER_PORT    $port

#endif
EOF
    echo "broker.h written."
}

# Writes wifi.h, the header with the network the bridge joins.
# Replaces any copy already there.
# Arguments: <network name> <password> <destination folder>
write_wifi_header() {
    local ssid="$1"
    local password="$2"
    local destination="$3"

    # A \ or " inside a C string needs a \ in front of it
    ssid="${ssid//\\/\\\\}"
    ssid="${ssid//\"/\\\"}"
    password="${password//\\/\\\\}"
    password="${password//\"/\\\"}"

    echo "Writing $destination/wifi.h for network '$1'..."
    cat > "$destination/wifi.h" <<EOF
// Written by scripts/setup-certs.sh on every run - changes made here are overwritten.
#ifndef WIFI_H
#define WIFI_H

#define WIFI_SSID     "$ssid"
#define WIFI_PASSWORD "$password"

#endif
EOF
    echo "wifi.h written."
}
