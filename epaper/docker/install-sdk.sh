#!/usr/bin/env bash
# Install reMarkable SDK from mounted installer (x86_64 host only).
set -euo pipefail

INSTALLER_DIR="${SDK_INSTALLER_DIR:-/sdk-installer}"
INSTALLER=$(find "$INSTALLER_DIR" -maxdepth 1 \( -name 'remarkable-*-x86_64-toolchain.sh' -o -name 'meta-toolchain-remarkable-*-x86_64-toolchain.sh' \) | head -1)

if [[ -z "$INSTALLER" ]]; then
  echo "No SDK installer found in $INSTALLER_DIR"
  echo "Download from https://developer.remarkable.com/documentation/links (rm2 matching device OS)"
  exit 1
fi

if [[ "$(uname -m)" != "x86_64" ]]; then
  echo "ERROR: SDK host must be x86_64 (got $(uname -m)). Use --platform linux/amd64."
  exit 1
fi

if [[ ! -w "$INSTALLER" ]]; then
  WRITABLE_INSTALLER="/tmp/$(basename "$INSTALLER")"
  cp "$INSTALLER" "$WRITABLE_INSTALLER"
  chmod +x "$WRITABLE_INSTALLER"
  INSTALLER="$WRITABLE_INSTALLER"
else
  chmod +x "$INSTALLER"
fi
"$INSTALLER" -d "${RM_SDK_ROOT:-/opt/remarkable-sdk}" -y

echo "SDK installed. Source environment:"
ls "${RM_SDK_ROOT:-/opt/remarkable-sdk}"/environment-setup-* 2>/dev/null || true
