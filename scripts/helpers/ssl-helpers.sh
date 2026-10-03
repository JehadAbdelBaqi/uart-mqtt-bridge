# Creates the CA's key and certificate in ~/certs/<project name>/ if they don't exist yet.
# Arguments: <project name>
ensure_ca() {
    local project_name="$1"
    local cert_dir="$HOME/certs/$project_name"

    echo "Checking for a CA in $cert_dir..."
    if [ -f "$cert_dir/ca.key" ]; then
        echo "CA found."
        return
    fi

    echo "No CA found. Creating one..."
    mkdir -p "$cert_dir"
    chmod 700 "$cert_dir"
    openssl genrsa -out "$cert_dir/ca.key" 2048
    openssl req -x509 -new -key "$cert_dir/ca.key" -sha256 -days 3650 -subj "/CN=$project_name test CA" -out "$cert_dir/ca.crt"
    echo "CA created."
}

# Creates the client's key and certificate, signed by the CA, if they don't exist yet.
# Arguments: <project name>
ensure_client_cert() {
    local project_name="$1"
    local cert_dir="$HOME/certs/$project_name"

    echo "Checking for a client certificate in $cert_dir..."
    if [ -f "$cert_dir/client.crt" ]; then
        echo "Client certificate found."
        return
    fi

    echo "No client certificate found. Creating one..."
    openssl genrsa -out "$cert_dir/client.key" 2048
    openssl req -new -key "$cert_dir/client.key" -subj "/CN=$project_name client" -out "$cert_dir/client.csr"
    openssl x509 -req -in "$cert_dir/client.csr" -CA "$cert_dir/ca.crt" -CAkey "$cert_dir/ca.key" -CAcreateserial -sha256 -days 3650 -out "$cert_dir/client.crt"
    echo "Client certificate created."
}

# Creates the server's key and certificate, signed by the CA and named for the address.
# Replaces any existing ones.
# Arguments: <project name> <address>
create_server_cert() {
    local project_name="$1"
    local address="$2"
    local cert_dir="$HOME/certs/$project_name"

    echo "Creating the server certificate for $address..."
    openssl genrsa -out "$cert_dir/server.key" 2048
    openssl req -new -key "$cert_dir/server.key" -subj "/CN=$address" -out "$cert_dir/server.csr"
    openssl x509 -req -in "$cert_dir/server.csr" -CA "$cert_dir/ca.crt" -CAkey "$cert_dir/ca.key" -CAcreateserial -sha256 -days 3650 -extfile <(printf 'subjectAltName=IP:%s' "$address") -out "$cert_dir/server.crt"
    echo "Server certificate created."
}
