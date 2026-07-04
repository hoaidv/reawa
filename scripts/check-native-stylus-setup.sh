#!/bin/sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
ENTITLEMENTS_FILE="$ROOT_DIR/Config/Reawa.entitlements"

echo "== Code signing identities =="
security find-identity -v -p codesigning || true
echo

echo "== Entitlements file =="
if [ -f "$ENTITLEMENTS_FILE" ]; then
    plutil -p "$ENTITLEMENTS_FILE"
else
    echo "Missing: $ENTITLEMENTS_FILE"
fi
echo

echo "== What you still need =="
echo "1. Apple Developer Program membership."
echo "2. An App ID for io.github.hoaidv.reawa."
echo "3. Apple approval for com.apple.developer.hid.virtual.device."
echo "4. A provisioning profile for that App ID after approval."
echo "5. A local Apple Development signing identity installed in Keychain."
echo

echo "== Local build reminder =="
echo "Native Stylus cannot work from \`swift run reawa\`."
echo "Use: sh scripts/build-macos-app.sh --configuration debug"
