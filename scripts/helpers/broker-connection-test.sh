# Stops the script if a TLS connection to the broker can't be made with the client certificate,
# or if the broker's certificate isn't signed by the CA and named for the address.
# Arguments: <wsl prefix> <project name> <address> <port>
check_broker_tls() {
    local wsl_prefix="$1"
    local project_name="$2"
    local address="$3"
    local port="$4"
    local cert_dir="~/certs/$project_name"
    local output

    echo "Opening a TLS connection to the broker at $address:$port..."
    if ! output=$($wsl_prefix bash -c "timeout 10 openssl s_client -connect $address:$port -CAfile $cert_dir/ca.crt -cert $cert_dir/client.crt -key $cert_dir/client.key -verify_ip $address -verify_return_error < /dev/null 2>&1"); then
        echo "TLS connection failed:" >&2
        echo "$output" >&2
        exit 1
    fi

    echo "$output" | grep "Verify return code"
    echo "TLS connection works."
}
