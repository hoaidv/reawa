#!/usr/bin/env bash
# Open an interactive SSH session on the tablet.
set -euo pipefail

HOST_INPUT="${1:-${RM_HOST:-10.11.99.1}}"
HOST_INPUT="${HOST_INPUT#ssh://}"
if [[ "$HOST_INPUT" == *@* ]]; then
  HOST="$HOST_INPUT"
else
  HOST="root@${HOST_INPUT}"
fi

KNOWN_HOSTS="${RM_SSH_KNOWN_HOSTS:-$HOME/.ssh/reawa_rm_known_hosts}"
mkdir -p "$(dirname "$KNOWN_HOSTS")"
: >>"$KNOWN_HOSTS"

SSH_OPTS=(
  -o StrictHostKeyChecking=accept-new
  -o UserKnownHostsFile="$KNOWN_HOSTS"
  -o GlobalKnownHostsFile=/dev/null
  -o LogLevel=ERROR
  -o ConnectTimeout=8
)

if [[ -n "${RM_SSH_KEY:-}" && -f "$RM_SSH_KEY" ]]; then
  exec ssh -i "$RM_SSH_KEY" "${SSH_OPTS[@]}" "$HOST"
fi

if [[ -n "${RM_SSH_PASSWORD:-}" ]]; then
  if ! command -v sshpass >/dev/null 2>&1; then
    echo "sshpass is required for password login. Install it, or set RM_SSH_KEY." >&2
    exit 1
  fi
  exec sshpass -p "$RM_SSH_PASSWORD" ssh \
    -o PreferredAuthentications=password \
    -o PubkeyAuthentication=no \
    "${SSH_OPTS[@]}" "$HOST"
fi

echo "Set RM_SSH_KEY to a private key file, or RM_SSH_PASSWORD." >&2
exit 1