#!/usr/bin/env bash
#
# Build Reawa.app with py2app.
#
# Produces packaging/dist/Reawa.app from a sanitized staging copy of the
# sources. No SSH keys, secrets, or the dev .venv are ever bundled.
#
# After building, the bundle is code-signed. py2app's default ad-hoc signature
# uses an unstable cdhash that changes every build, so macOS keeps forgetting
# granted permissions (Accessibility, Keychain). Signing with a real identity
# gives a stable code identity that TCC/Keychain grants stick to.
#
# Usage:
#   ./build.sh                                  # prompts you to pick an identity
#   CODESIGN_IDENTITY="Developer ID Application: Name (TEAMID)" ./build.sh
#   CODESIGN_IDENTITY="-" ./build.sh            # force ad-hoc (no identity)
#   CODESIGN_HARDENED=1 ./build.sh              # add hardened runtime (auto for Developer ID)
#
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PKG_DIR="$(dirname "$HERE")"            # reawa/
VENV_PY="$PKG_DIR/.venv/bin/python"
ICON_SRC="$PKG_DIR/assets/app_icon.png"
APP="$HERE/dist/Reawa.app"
ENTITLEMENTS="$HERE/entitlements.plist"

cd "$HERE"

# ---------------------------------------------------------------------------
# Pick the signing identity up front (fail fast before a long build).
#
# Honors CODESIGN_IDENTITY when set; otherwise lists the available identities
# and prompts. "-" means ad-hoc.
# ---------------------------------------------------------------------------
choose_identity() {
  if [ -n "${CODESIGN_IDENTITY:-}" ]; then
    SIGN_IDENTITY="$CODESIGN_IDENTITY"
    echo "==> Using signing identity from CODESIGN_IDENTITY: $SIGN_IDENTITY"
    return
  fi

  echo "==> Available code-signing identities"
  local names=() line name
  while IFS= read -r line; do
    name=$(printf '%s\n' "$line" | sed -E 's/^[^"]*"(.*)"$/\1/')
    names+=("$name")
  done < <(security find-identity -v -p codesigning | grep -E '^[[:space:]]*[0-9]+\)' || true)

  local count=${#names[@]} i=1
  if [ "$count" -eq 0 ]; then
    echo "    (none found — install an Apple Development or Developer ID cert)"
  else
    for name in "${names[@]}"; do
      printf "  %d) %s\n" "$i" "$name"
      i=$((i + 1))
    done
  fi
  printf "  a) ad-hoc (no identity — granted permissions will NOT persist)\n"

  if [ ! -t 0 ]; then
    echo "    Non-interactive shell; set CODESIGN_IDENTITY to choose. Defaulting to ad-hoc." >&2
    SIGN_IDENTITY="-"
    return
  fi

  local choice default
  if [ "$count" -ge 1 ]; then default="1"; else default="a"; fi
  read -rp "Select identity to sign with [$default]: " choice
  choice=${choice:-$default}

  case "$choice" in
    a | A)
      SIGN_IDENTITY="-"
      ;;
    *)
      if printf '%s' "$choice" | grep -qE '^[0-9]+$' \
        && [ "$choice" -ge 1 ] && [ "$choice" -le "$count" ]; then
        SIGN_IDENTITY="${names[$((choice - 1))]}"
      else
        echo "Invalid choice: $choice" >&2
        exit 1
      fi
      ;;
  esac
}

sign_app() {
  local app="$1"
  local args=(--force --deep --sign "$SIGN_IDENTITY")

  if [ "$SIGN_IDENTITY" = "-" ]; then
    echo "==> Ad-hoc signing $app"
  else
    echo "==> Signing $app with: $SIGN_IDENTITY"
    local hardened="${CODESIGN_HARDENED:-0}"
    case "$SIGN_IDENTITY" in
      "Developer ID Application:"*)
        # Developer ID builds are almost always notarized, which requires a
        # hardened runtime and a secure timestamp.
        hardened=1
        args+=(--timestamp)
        ;;
    esac
    [ "$hardened" = "1" ] && args+=(--options runtime)
    if [ -f "$ENTITLEMENTS" ]; then
      echo "    using entitlements: $ENTITLEMENTS"
      args+=(--entitlements "$ENTITLEMENTS")
    fi
  fi

  codesign "${args[@]}" "$app"

  echo "==> Verifying signature"
  codesign --verify --strict --verbose=2 "$app" || true
  codesign -dvv "$app" 2>&1 | grep -E "Authority|TeamIdentifier|Signature=" || true
}

choose_identity

echo "==> Cleaning previous build artifacts"
rm -rf build dist src

echo "==> Staging sanitized sources (.py only)"
rsync -a --prune-empty-dirs \
  --exclude='packaging' --exclude='.venv' --exclude='.ssh' \
  --exclude='__pycache__' --exclude='.docs' --exclude='.git' \
  --include='*/' --include='*.py' --exclude='*' \
  "$PKG_DIR/" src/reawa/

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

sign_app "$APP"

echo "==> Done: $APP"
