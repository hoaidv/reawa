#!/usr/bin/env bash
# Copy the ARM binary to a USB-connected RM2 and launch it.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT/build/bin/epaper"
KEY="${RM_SSH_KEY:-}"
HOST="${RM_HOST:-root@10.11.99.1}"
REMOTE="${RM_REMOTE_PATH:-/home/root/epaper}"

if [[ -z "$KEY" || ! -f "$KEY" ]]; then
  echo "SSH key not found: ${RM_SSH_KEY:-unset}" >&2
  echo "Set RM_SSH_KEY to your device private key." >&2
  exit 1
fi

if [[ ! -f "$BIN" ]]; then
  echo "Binary missing: $BIN" >&2
  echo "Build first: $ROOT/scripts/build.sh" >&2
  exit 1
fi

KNOWN_HOSTS="${RM_SSH_KNOWN_HOSTS:-$HOME/.ssh/reawa_rm_known_hosts}"
mkdir -p "$(dirname "$KNOWN_HOSTS")"
: >>"$KNOWN_HOSTS"

SSH_OPTS=(
  -i "$KEY"
  -o StrictHostKeyChecking=accept-new
  -o UserKnownHostsFile="$KNOWN_HOSTS"
  -o GlobalKnownHostsFile=/dev/null
  -o LogLevel=ERROR
  -o ConnectTimeout=8
)

echo "Deploying $(basename "$BIN") to $HOST:$REMOTE ..."
scp "${SSH_OPTS[@]}" "$BIN" "$HOST:$REMOTE"

ssh "${SSH_OPTS[@]}" "$HOST" bash -s <<EOF
set -e
chmod +x $REMOTE
killall epaper 2>/dev/null || true
systemctl stop xochitl || true
cd /home/root
nohup ./epaper > /tmp/epaper.log 2>&1 &
sleep 1
pgrep -a epaper || { echo "FAILED:"; cat /tmp/epaper.log; exit 1; }
EOF

echo "OK: launched $REMOTE on $HOST"
