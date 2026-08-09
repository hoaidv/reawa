#!/usr/bin/env bash
# Deploy and launch Epaper on connected RM2 (USB 10.11.99.1).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT/build/epaper"
KEY="${RM_SSH_KEY:-$HOME/Library/Application Support/Reawa/keys/2ff02795-fae7-4587-be4b-e40c94b9a636/id_rsa}"
HOST="${RM_HOST:-root@10.11.99.1}"
REMOTE="${RM_REMOTE_PATH:-/home/root/epaper}"

if [[ ! -f "$BIN" ]]; then
  echo "Binary missing. Build first (inside x86_64 SDK container):"
  echo "  source /opt/remarkable-sdk/environment-setup-cortexa7hf-neon-remarkable-linux-gnueabi"
  echo "  cd /workspace/Epaper && cmake -B build -G Ninja && cmake --build build"
  exit 1
fi

echo "Deploying to $HOST ..."
scp -i "$KEY" -o StrictHostKeyChecking=no "$BIN" "$HOST:$REMOTE"

echo "Launching (stops xochitl) ..."
ssh -i "$KEY" -o StrictHostKeyChecking=no "$HOST" bash -s <<'EOF'
set -e
chmod +x /home/root/epaper
killall epaper rm-canvas-spike 2>/dev/null || true
systemctl stop xochitl || true
export QT_QPA_PLATFORM="epaper:enable_fonts"
export QT_QPA_EVDEV_TOUCHSCREEN_PARAMETERS="rotate=180:invertx"
export QT_QPA_GENERIC_PLUGINS=evdevtablet
export QT_QUICK_BACKEND=epaper
cd /home/root
nohup ./epaper -platform epaper > /tmp/epaper.log 2>&1 &
sleep 1
pgrep -a epaper || (echo "FAILED:"; cat /tmp/epaper.log; exit 1)
EOF
