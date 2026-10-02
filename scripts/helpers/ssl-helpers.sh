# Creates the CA's key and certificate in ~/certs/<project name>/ if they don't exist yet.
# Arguments: <wsl prefix> <project name>
ensure_ca() {
    local wsl_prefix="$1"
    local project_name="$2"
    local cert_dir="~/certs/$project_name"

    echo "Checking for a CA in $cert_dir..."
    if $wsl_prefix bash -c "test -f $cert_dir/ca.key"; then
        echo "CA found."
    else
        echo "No CA found. Creating one..."
        $wsl_prefix bash -c "mkdir -p $cert_dir && chmod 700 $cert_dir"
        $wsl_prefix bash -c "openssl genrsa -out $cert_dir/ca.key 2048"
        $wsl_prefix bash -c "openssl req -x509 -new -key $cert_dir/ca.key -sha256 -days 3650 -subj '/CN=$project_name test CA' -out $cert_dir/ca.crt"
        echo "CA created."
    fi
}

# Creates the client's key and certificate, signed by the CA, if they don't exist yet.
# Arguments: <wsl prefix> <project name>
ensure_client_cert() {
    local wsl_prefix="$1"
    local project_name="$2"
    local cert_dir="~/certs/$project_name"

    echo "Checking for a client certificate in $cert_dir..."
    if $wsl_prefix bash -c "test -f $cert_dir/client.crt"; then
        echo "Client certificate found."
    else
        echo "No client certificate found. Creating one..."
        $wsl_prefix bash -c "openssl genrsa -out $cert_dir/client.key 2048"
        $wsl_prefix bash -c "openssl req -new -key $cert_dir/client.key -subj '/CN=$project_name client' -out $cert_dir/client.csr"
        $wsl_prefix bash -c "openssl x509 -req -in $cert_dir/client.csr -CA $cert_dir/ca.crt -CAkey $cert_dir/ca.key -CAcreateserial -sha256 -days 3650 -out $cert_dir/client.crt"
        echo "Client certificate created."
    fi
}

# Creates the server's key and certificate, signed by the CA and named for the address.
# Replaces any existing ones.
# Arguments: <wsl prefix> <project name> <address>
create_server_cert() {
    local wsl_prefix="$1"
    local project_name="$2"
    local address="$3"
    local cert_dir="~/certs/$project_name"

    echo "Creating the server certificate for $address..."
    $wsl_prefix bash -c "openssl genrsa -out $cert_dir/server.key 2048"
    $wsl_prefix bash -c "openssl req -new -key $cert_dir/server.key -subj '/CN=$address' -out $cert_dir/server.csr"
    $wsl_prefix bash -c "openssl x509 -req -in $cert_dir/server.csr -CA $cert_dir/ca.crt -CAkey $cert_dir/ca.key -CAcreateserial -sha256 -days 3650 -extfile <(printf 'subjectAltName=IP:$address') -out $cert_dir/server.crt"
    echo "Server certificate created."
}
