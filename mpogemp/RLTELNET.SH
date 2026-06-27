#!/bin/sh

set -eu

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <host> <port>" >&2
    echo "Example: $0 localhost 1234" >&2
    exit 1
fi

TELNET_HOST=$1
TELNET_PORT=$2
PASSWORD_PROMPT=$(printf '\033[1m\033[37mPassword:\033[0m')

if ! command -v rlwrap >/dev/null 2>&1; then
    echo "RLTELNET.SH requires rlwrap. Install rlwrap or run telnet manually:" >&2
    echo "telnet" >&2
    echo "open $TELNET_HOST -$TELNET_PORT" >&2
    exit 1
fi

echo "Confirm this is correct and press ENTER:"
echo "open $TELNET_HOST -$TELNET_PORT"

exec rlwrap -a"$PASSWORD_PROMPT" -A --no-warnings \
    --pre-given "open $TELNET_HOST -$TELNET_PORT" telnet
