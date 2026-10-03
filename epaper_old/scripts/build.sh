#!/usr/bin/env bash
# Cross-compile epaper for RM2 via the amd64 SDK Docker image.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT/build/bin/epaper"
DOCKER_DIR="$ROOT/docker"
IMAGE_SERVICE="rm-sdk"
ENV_SETUP="/opt/remarkable-sdk/environment-setup-cortexa7hf-neon-remarkable-linux-gnueabi"

usage() {
  cat <<'EOF'
Usage:
  ./scripts/build.sh [--no-cache] [--shell]

Builds ARM epaper binary at epaper/build/epaper using Docker (linux/amd64)
and the reMarkable SDK.

Options:
  --no-cache   Rebuild the Docker image without cache
  --shell      Enter the SDK container instead of building
  -h, --help   Show this help

Prerequisites:
  1. Docker Desktop (or compatible) with amd64 emulation
  2. SDK installer .sh in epaper/docker/sdk-installer/
     See epaper/TOOLCHAIN.md
EOF
}

NO_CACHE=0
SHELL_ONLY=0
while [[ $# -gt 0 ]]; do
  case "$1" in
    --no-cache) NO_CACHE=1; shift ;;
    --shell) SHELL_ONLY=1; shift ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown arg: $1" >&2; usage >&2; exit 1 ;;
  esac
done

if ! command -v docker >/dev/null 2>&1; then
  echo "docker not found" >&2
  exit 1
fi

INSTALLER=$(find "$DOCKER_DIR/sdk-installer" -maxdepth 1 \( \
  -name 'remarkable-*-x86_64-toolchain.sh' -o \
  -name 'meta-toolchain-remarkable-*-x86_64-toolchain.sh' \) 2>/dev/null | head -1 || true)
if [[ -z "$INSTALLER" ]]; then
  echo "No SDK installer in $DOCKER_DIR/sdk-installer/" >&2
  echo "See $ROOT/TOOLCHAIN.md" >&2
  exit 1
fi

cd "$DOCKER_DIR"
echo "Building SDK container image..."
if [[ "$NO_CACHE" -eq 1 ]]; then
  docker compose build --no-cache
else
  docker compose build
fi

if [[ "$SHELL_ONLY" -eq 1 ]]; then
  exec docker compose run --rm "$IMAGE_SERVICE" bash -lc \
    "if [[ ! -f $ENV_SETUP ]]; then install-sdk.sh; fi; source $ENV_SETUP; exec bash"
fi

echo "Installing SDK (if needed) and compiling epaper..."
docker compose run --rm "$IMAGE_SERVICE" bash -lc "
set -euo pipefail
if [[ ! -f $ENV_SETUP ]]; then
  install-sdk.sh
fi
source $ENV_SETUP
cd /workspace/epaper
cmake -B build -G Ninja
cmake --build build
file build/bin/epaper
"

if [[ ! -f "$ROOT/build/bin/epaper" ]]; then
  echo "Build finished but $ROOT/build/bin/epaper is missing" >&2
  exit 1
fi

echo "OK: $ROOT/build/bin/epaper"
file "$ROOT/build/bin/epaper" || true
ls -lh "$ROOT/build/bin/epaper"
