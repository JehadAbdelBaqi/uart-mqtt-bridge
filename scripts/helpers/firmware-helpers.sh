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

# Writes config_in_use.h, the header that points the firmware at the config file it is built with
# and tells it whether it is built for a project.
# Arguments: <path of the config header to use> <built for a project: 1 or 0> <destination folder>
write_config_in_use_header() {
    local config_header="$1"
    local built_for_project="$2"
    local destination="$3"

    if [ ! -f "$config_header" ]; then
        echo "Config header not found: $config_header" >&2
        exit 1
    fi

    echo "Writing $destination/config_in_use.h for $config_header..."
    mkdir -p "$destination"
    cat > "$destination/config_in_use.h" <<EOF
// Written by scripts/steps/build-and-upload.sh on every run - changes made here are overwritten.
#define BUILT_FOR_PROJECT $built_for_project
#include "$config_header"
EOF
    echo "config_in_use.h written."
}

# Deletes the firmware's build output. Only generated files: the next build recreates them.
# Arguments: <project folder>
delete_build_output() {
    local project_dir="$1"

    echo "Deleting the build output in $project_dir/.pio/build..."
    rm -rf -- "$project_dir/.pio/build"
    echo "Build output deleted."
}

# Runs the firmware's unit tests on the PC. The script stops if any of them fails.
# Arguments: <platformio command> <project folder>
run_unit_tests() {
    local platformio="$1"
    local project_dir="$2"

    echo "Running the unit tests in $project_dir on the PC..."
    "$platformio" test --project-dir "$project_dir" --environment native
    echo "Unit tests passed."
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
