# Stops the script if a TLS connection to the broker can't be made with the client certificate,
# or if the broker's certificate isn't signed by the CA and named for the address.
# Arguments: <project name> <address> <port>
check_broker_tls() {
    local project_name="$1"
    local address="$2"
    local port="$3"
    local cert_dir="$HOME/certs/$project_name"
    local output

    echo "Opening a TLS connection to the broker at $address:$port..."
    if ! output=$(timeout 10 openssl s_client -connect "$address:$port" -CAfile "$cert_dir/ca.crt" -cert "$cert_dir/client.crt" -key "$cert_dir/client.key" -verify_ip "$address" -verify_return_error < /dev/null 2>&1); then
        echo "TLS connection failed:" >&2
        echo "$output" >&2
        exit 1
    fi

    echo "$output" | grep "Verify return code"
    echo "TLS connection works."
}
