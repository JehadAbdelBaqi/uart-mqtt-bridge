# Copies the files the firmware is built with - the CA certificate, the client certificate
# and the client key - into a folder, unchanged. Replaces any copies already there.
# Arguments: <project name> <destination folder>
copy_firmware_certs() {
    local project_name="$1"
    local destination="$2"
    local cert_dir="$HOME/certs/$project_name"

    echo "Copying the firmware's certificates to $destination..."
    for file in ca.crt client.crt client.key; do
        cp "$cert_dir/$file" "$destination/$file"
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
// Written by scripts/steps/setup-certs.sh on every run - changes made here are overwritten.
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
// Written by scripts/steps/setup-certs.sh on every run - changes made here are overwritten.
#ifndef WIFI_H
#define WIFI_H

#define WIFI_SSID     "$ssid"
#define WIFI_PASSWORD "$password"

#endif
EOF
    echo "wifi.h written."
}

# Writes config_in_use.h, the header that points the firmware at the config file it is built with.
# Arguments: <path of the config header to use> <destination folder>
write_config_in_use_header() {
    local config_header="$1"
    local destination="$2"

    if [ ! -f "$config_header" ]; then
        echo "Config header not found: $config_header" >&2
        exit 1
    fi

    echo "Writing $destination/config_in_use.h for $config_header..."
    mkdir -p "$destination"
    cat > "$destination/config_in_use.h" <<EOF
// Written by scripts/steps/build-and-upload.sh on every run - changes made here are overwritten.
#include "$config_header"
EOF
    echo "config_in_use.h written."
}

# Builds the firmware and uploads it to the board. The script stops if either step fails.
# The board must be plugged in, with no serial monitor holding its port.
# Arguments: <platformio command> <project folder>
build_and_upload_firmware() {
    local platformio="$1"
    local project_dir="$2"

    echo "Building the firmware and uploading it to the board..."
    "$platformio" run --project-dir "$project_dir" --target upload
    echo "Firmware uploaded."
}
