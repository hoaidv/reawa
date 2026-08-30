#!/usr/bin/env bash
# Deploy and launch epaper on connected RM2 (USB 10.11.99.1).
#
# [STORY-EP-034] Diagnose BEFORE unplugging USB:
#   ping-dead / en* 169.254  → USB gadget or Mac USB power. SSH keepalives will not revive it.
#   ping-alive, SSH hang     → dropbear / leftover ssh children.
#   ping-alive, Infini "RM disconnected" → StrokeSync TCP only (observed 2026-08-16).
# Do not unplug for the ping-alive Infini-down class.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT/build/bin/epaper"
KEY="$RM_SSH_KEY"
HOST="${RM_HOST:-root@10.11.99.1}"
REMOTE="${RM_REMOTE_PATH:-/home/root/epaper}"

usage() {
  cat <<'EOF'
Usage:
  ./scripts/deploy-rm2.sh [--build] [--restore] [--usb-stay]

Deploy epaper to RM2 over USB Ethernet, stop xochitl, and launch.

Options:
  --build     Run ./scripts/build.sh first
  --restore   Kill epaper and start xochitl (no deploy)
  --usb-stay  Ping-dead class only: one-shot disable usb0 autosuspend.
              SSH keepalives are NOT a gadget-death fix. Safe to skip when ping works.
  -h, --help  Show this help

Env:
  RM_HOST              default root@10.11.99.1
  RM_SSH_KEY           path to device SSH private key
  RM_SSH_KNOWN_HOSTS   per-machine host-key file (default ~/.ssh/reawa_rm_known_hosts)
  RM_REMOTE_PATH       default /home/root/epaper

App env forwarded to the device when set locally:
  RM_SYNC_HOST     macOS USB IP for stroke sync + debug (e.g. 10.11.99.12).
                   NEVER the tablet address 10.11.99.1 — epaper must dial the Mac.
  EPAPER_DEBUG_LOG 1|true|on|yes — ship Qt/stdio logs to Infini :9878 (STORY-EP-021)
  EPAPER_DEBUG_PORT override debug sidecar port (default 9878)
  INFINI_DEBUG_PORT fallback debug port if EPAPER_DEBUG_PORT unset
  RM_INK_MODE      painted (default) | pool
  RM_INK_TRACE     1 — latency instrumentation
  RM_DOC_PROBE     1 — 500-node / 50k-sample stub + hit-test on ingest (STORY-EP-013)
  RM_DOC_PROBE_SYNTH 1 — synthetic strokes then exit (device harness)
  RM_DOC_PROBE_EVERY_SAMPLE 1 — hit-test every sample (stress)
  RM_EP_SWAP       1 — direct EPFramebuffer::swapBuffers(Pen) after each flush
  RM_EP_SCREEN_MODE / RM_EP_CONTENT_TYPE   override resolved enum values
  EPAPER_TOUCH_TRACE 1 — log every touch point the gesture filter sees
  EPAPER_UI_STALL_MS UI-thread stall threshold for the watchdog log
  QT_LOGGING_RULES Qt categories, e.g. qt.pointer.grab=true;qt.quick.handler.dispatch=true
                   Verbose rules cost live ink — pen strokes only land on pen-up.
EOF
}

DO_BUILD=0
DO_RESTORE=0
DO_USB_STAY=0
while [[ $# -gt 0 ]]; do
  case "$1" in
    --build) DO_BUILD=1; shift ;;
    --restore) DO_RESTORE=1; shift ;;
    --usb-stay) DO_USB_STAY=1; shift ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown arg: $1" >&2; usage >&2; exit 1 ;;
  esac
done

# @implements [SRS-RW-10] Per-machine known_hosts — learn 10.11.99.1 once; no /dev/null spam
# @implements [STORY-EP-034] SSH client keepalives — detect dead path without unplug
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
  -o ServerAliveInterval=15
  -o ServerAliveCountMax=4
  -o TCPKeepAlive=yes
)

ssh_rm() {
  ssh "${SSH_OPTS[@]}" "$HOST" "$@"
}

# Ping-dead gadget class only. Keepalives will not revive L3 death.
# @fix [STORY-EP-034] usb0 autosuspend off while tethered
usb_stay_remote() {
  ssh_rm bash -s <<'REMOTE'
set +e
# Best-effort; kernel paths differ. Do not treat failure as deploy failure.
for f in \
  /sys/class/net/usb0/device/power/control \
  /sys/devices/platform/soc/*/udc/*/power/control
do
  for p in $f; do
    [ -w "$p" ] && echo on > "$p" && echo "usb-stay: $p -> on"
  done
done
for f in /sys/class/net/usb0/device/power/autosuspend_delay_ms; do
  [ -w "$f" ] && echo -1 > "$f" && echo "usb-stay: $f -> -1"
done
REMOTE
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

if [[ "$DO_USB_STAY" -eq 1 ]]; then
  echo "usb-stay (ping-dead class only; skip if ping to 10.11.99.1 already works) ..."
  usb_stay_remote || true
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

echo "Stopping remote epaper (if any) ..."
ssh_rm 'killall epaper rm-canvas-spike 2>/dev/null || true; sleep 0.3' || true

echo "Deploying $(basename "$BIN") ($(ls -lh "$BIN" | awk '{print $5}')) to $HOST:$REMOTE ..."
scp "${SSH_OPTS[@]}" "$BIN" "$HOST:$REMOTE"

FORWARDED=(RM_SYNC_HOST RM_INK_MODE RM_INK_TRACE RM_EP_SWAP
           RM_EP_SCREEN_MODE RM_EP_CONTENT_TYPE
           RM_DOC_PROBE RM_DOC_PROBE_SYNTH RM_DOC_PROBE_EVERY_SAMPLE
           EPAPER_DEBUG_LOG EPAPER_DEBUG_PORT INFINI_DEBUG_PORT
           EPAPER_TOUCH_TRACE EPAPER_UI_STALL_MS QT_LOGGING_RULES)
APP_ENV=""
for name in "${FORWARDED[@]}"; do
  if [[ -n "${!name:-}" ]]; then
    APP_ENV+="export $name=$(printf '%q' "${!name}")"$'\n'
  fi
done

# Guard: tablet must dial the Mac, not itself.
if [[ "${RM_SYNC_HOST:-}" == "10.11.99.1" ]]; then
  echo "ERROR: RM_SYNC_HOST=10.11.99.1 is the tablet. Use the Mac USB IP (usually 10.11.99.12)." >&2
  exit 1
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
$APP_ENV
cd /home/root
nohup ./epaper -platform epaper > /tmp/epaper.log 2>&1 &
sleep 1
pgrep -a epaper || (echo "FAILED:"; cat /tmp/epaper.log; exit 1)
echo "---- /tmp/epaper.log (tail) ----"
tail -n 20 /tmp/epaper.log || true
EOF

echo "OK: epaper running on RM2. Draw to verify ink. Restore with:"
echo "  $0 --restore"
echo "If ping to the tablet is dead while USB is plugged (gadget class), not Infini-down:"
echo "  $0 --usb-stay"
