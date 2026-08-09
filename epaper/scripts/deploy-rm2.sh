#!/usr/bin/env bash
# Deploy and launch epaper on connected RM2 (USB 10.11.99.1).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT/build/bin/epaper"
KEY="${RM_SSH_KEY:-$HOME/Library/Application Support/Reawa/keys/2ff02795-fae7-4587-be4b-e40c94b9a636/id_rsa}"
HOST="${RM_HOST:-root@10.11.99.1}"
REMOTE="${RM_REMOTE_PATH:-/home/root/epaper}"

usage() {
  cat <<'EOF'
Usage:
  ./scripts/deploy-rm2.sh [--build] [--restore]

Deploy epaper to RM2 over USB Ethernet, stop xochitl, and launch.

Options:
  --build     Run ./scripts/build.sh first
  --restore   Kill epaper and start xochitl (no deploy)
  -h, --help  Show this help

Env:
  RM_HOST          default root@10.11.99.1
  RM_SSH_KEY       path to device SSH private key
  RM_REMOTE_PATH   default /home/root/epaper
  RM_SYNC_HOST     optional macOS IP for stroke sync (passed to app)
EOF
}

DO_BUILD=0
DO_RESTORE=0
while [[ $# -gt 0 ]]; do
  case "$1" in
    --build) DO_BUILD=1; shift ;;
    --restore) DO_RESTORE=1; shift ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown arg: $1" >&2; usage >&2; exit 1 ;;
  esac
done

ssh_rm() {
  ssh -i "$KEY" -o StrictHostKeyChecking=no -o ConnectTimeout=8 "$HOST" "$@"
}

if [[ ! -f "$KEY" ]]; then
  echo "SSH key not found: $KEY" >&2
  echo "Set RM_SSH_KEY to your Reawa/device key." >&2
  exit 1
fi

if [[ "$DO_RESTORE" -eq 1 ]]; then
  echo "Restoring xochitl on $HOST ..."
  ssh_rm 'killall epaper rm-canvas-spike 2>/dev/null || true; systemctl start xochitl; pgrep -a xochitl || true'
  exit 0
fi

if [[ "$DO_BUILD" -eq 1 ]]; then
  "$ROOT/scripts/build.sh"
fi

if [[ ! -f "$BIN" ]]; then
  echo "Binary missing: $BIN" >&2
  echo "Build first: $ROOT/scripts/build.sh" >&2
  exit 1
fi

if ! ping -c 1 -W 2 10.11.99.1 >/dev/null 2>&1; then
  # Host may be IP in RM_HOST
  TARGET_IP="${HOST##*@}"
  if ! ping -c 1 -W 2 "$TARGET_IP" >/dev/null 2>&1; then
    echo "RM2 unreachable at $TARGET_IP" >&2
    echo "Plug USB (expect route via en* to 10.11.99.1, not Wi‑Fi gateway)." >&2
    exit 1
  fi
fi

echo "Deploying $(basename "$BIN") ($(ls -lh "$BIN" | awk '{print $5}')) to $HOST:$REMOTE ..."
scp -i "$KEY" -o StrictHostKeyChecking=no "$BIN" "$HOST:$REMOTE"

SYNC_EXPORT=""
if [[ -n "${RM_SYNC_HOST:-}" ]]; then
  SYNC_EXPORT="export RM_SYNC_HOST=$(printf '%q' "$RM_SYNC_HOST")"
fi

echo "Launching (stops xochitl) ..."
ssh_rm bash -s <<EOF
set -e
chmod +x $REMOTE
killall epaper rm-canvas-spike 2>/dev/null || true
systemctl stop xochitl || true
export QT_QPA_PLATFORM="epaper:enable_fonts"
export QT_QPA_EVDEV_TOUCHSCREEN_PARAMETERS="rotate=180:invertx"
export QT_QPA_GENERIC_PLUGINS=evdevtablet
export QT_QUICK_BACKEND=epaper
$SYNC_EXPORT
cd /home/root
nohup ./epaper -platform epaper > /tmp/epaper.log 2>&1 &
sleep 1
pgrep -a epaper || (echo "FAILED:"; cat /tmp/epaper.log; exit 1)
echo "---- /tmp/epaper.log (tail) ----"
tail -n 20 /tmp/epaper.log || true
EOF

echo "OK: epaper running on RM2. Draw to verify ink. Restore with:"
echo "  $0 --restore"
