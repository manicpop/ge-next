#!/bin/sh

# RLTELNET.SH connects to MajorBBS or MBBSEmu with rlwrap command-line
# editing and history. Usage: RLTELNET.SH <host> <port>
#
# Requires telnet, rlwrap, and expect. The expect bridge answers the ANSI
# cursor-position query used by MajorBBS terminal detection before rlwrap can
# mistake the terminal's reply for user input. It also moves MBBSEmu's trailing
# ANSI reset so both original password prompts share a suffix rlwrap can match.

set -eu

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <host> <port>" >&2
    echo "Example: $0 localhost 1234" >&2
    exit 1
fi

TELNET_HOST=$1
TELNET_PORT=$2
PASSWORD_PROMPT='assword:'

export TELNET_HOST TELNET_PORT

if ! command -v rlwrap >/dev/null 2>&1; then
    echo "RLTELNET.SH requires rlwrap. Install rlwrap or run telnet manually:" >&2
    echo "telnet $TELNET_HOST $TELNET_PORT" >&2
    exit 1
fi

if ! command -v expect >/dev/null 2>&1; then
    echo "RLTELNET.SH requires expect." >&2
    exit 1
fi

exec rlwrap -a"$PASSWORD_PROMPT" -A --no-warnings \
    expect -c '
        set timeout -1
        log_user 0
        spawn telnet $env(TELNET_HOST) $env(TELNET_PORT)
        log_user 1
        interact {
            -o -exact "\033\[6n" { send -- "\033\[1;1R" }
            -exact "\033\[1m\033\[37mPassword:\033\[0m" {
                send_user -- "\033\[0m\033\[1m\033\[37mPassword:"
            }
            eof { exit }
        }
    '
