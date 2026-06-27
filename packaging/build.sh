#!/usr/bin/env bash
#
# Build Reawa.app with py2app.
#
# Produces packaging/dist/Reawa.app from a sanitized staging copy of the
# sources. No SSH keys, secrets, or the dev .venv are ever bundled.
#
# Usage:
#   ./build.sh
#
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PKG_DIR="$(dirname "$HERE")"            # remarkable/
VENV_PY="$PKG_DIR/.venv/bin/python"
ICON_SRC="$PKG_DIR/assets/app_icon.png"

cd "$HERE"

echo "==> Cleaning previous build artifacts"
rm -rf build dist src

echo "==> Staging sanitized sources (.py only)"
rsync -a --prune-empty-dirs \
  --exclude='packaging' --exclude='.venv' --exclude='.ssh' \
  --exclude='__pycache__' --exclude='.docs' --exclude='.git' \
  --include='*/' --include='*.py' --exclude='*' \
  "$PKG_DIR/" src/remarkable/

echo "==> Generating app_icon.icns"
rm -rf app_icon.iconset app_icon.icns
mkdir app_icon.iconset
for spec in \
  "16 icon_16x16" "32 icon_16x16@2x" "32 icon_32x32" "64 icon_32x32@2x" \
  "128 icon_128x128" "256 icon_128x128@2x" "256 icon_256x256" \
  "512 icon_256x256@2x" "512 icon_512x512" "1024 icon_512x512@2x"; do
  set -- $spec
  sips -z "$1" "$1" "$ICON_SRC" --out "app_icon.iconset/$2.png" >/dev/null
done
iconutil -c icns app_icon.iconset
rm -rf app_icon.iconset

echo "==> Building app bundle"
"$VENV_PY" setup.py py2app

echo "==> Done: $HERE/dist/Reawa.app"
