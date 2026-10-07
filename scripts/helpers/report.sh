# A short report of what a run did: one line for each step or check, printed when the run ends.
# The script that starts a run calls report_start, and everything it runs adds to the same report,
# through the REPORT_FILE it hands on. A script run by itself, with no report started, adds nothing.

# Starts a report, unless the script that ran this one already started it.
# Sets REPORT_OWNER to 1 in the script that has to print the report, 0 in every other.
report_start() {
    REPORT_OWNER=0
    if [ -n "${REPORT_FILE:-}" ]; then
        return
    fi

    REPORT_FILE=$(mktemp)
    export REPORT_FILE
    REPORT_OWNER=1
}

# Adds a line to the report. Does nothing when no report was started.
# Arguments: <name> <result>
report_add() {
    if [ -z "${REPORT_FILE:-}" ]; then
        return
    fi

    printf '%s\t%s\n' "$1" "$2" >> "$REPORT_FILE"
}

# Prints how many of the report's lines match a pattern; 0 when no report was started.
# Arguments: <pattern>
report_count() {
    if [ -z "${REPORT_FILE:-}" ]; then
        echo 0
        return
    fi

    grep -c -- "$1" "$REPORT_FILE" || true
}

# Prints the name a step goes by in the report: its script's file name, without the folder,
# the ending and any options.
# Arguments: <step, e.g. "steps/setup-certs.sh">
report_step_name() {
    basename "${1%% *}" .sh
}

# Runs a command and adds a line for it: success or FAILED.
# The command runs in a shell of its own, so one that stops the script only stops itself.
# Arguments: <name> <command> [the command's arguments]
# Returns: 1 if the command failed
report_run() {
    local name="$1"
    shift

    if ("$@"); then
        report_add "$name" "success"
        return 0
    fi

    report_add "$name" "FAILED"
    return 1
}

# Runs one step of a list and adds a line for it, unless the step added lines of its own:
# a step that reports each of its checks, or runs a list of steps itself, is not counted twice.
# Arguments: <name> <command> [the command's arguments]
# Returns: what the command returned
report_run_step() {
    local name="$1"
    shift
    local lines_before
    local failures_before
    local status=0

    lines_before=$(report_count ".")
    failures_before=$(report_count "FAILED$")

    "$@" || status=$?

    if [ "$status" -eq 0 ] && [ "$(report_count ".")" -eq "$lines_before" ]; then
        report_add "$name" "success"
    fi
    if [ "$status" -ne 0 ] && [ "$(report_count "FAILED$")" -eq "$failures_before" ]; then
        report_add "$name" "FAILED"
    fi

    return "$status"
}

# Prints the report and deletes its file, in the script that started it; does nothing in any other.
report_print() {
    if [ "${REPORT_OWNER:-0}" != 1 ]; then
        return
    fi

    echo
    echo "---------------- Report ----------------"
    awk -F '\t' '{ printf "%2d. %-34s %s\n", NR, $1, $2 }' "$REPORT_FILE"
    echo "----------------------------------------"
    rm -f "$REPORT_FILE"
}
