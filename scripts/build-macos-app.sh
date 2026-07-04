#!/bin/sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
CONFIGURATION="debug"
OUTPUT_APP="$ROOT_DIR/dist/Reawa.app"
SIGN_IDENTITY=""
PROVISIONING_PROFILE=""
ENTITLEMENTS_FILE="$ROOT_DIR/Config/Reawa.entitlements"
OPEN_AFTER_BUILD=0
SHOW_ENTITLEMENTS=0

usage() {
    cat <<'EOF'
Usage:
  sh scripts/build-macos-app.sh [options]

Options:
  --configuration <debug|release>     Swift build configuration (default: debug)
  --output <path>                     Output app bundle path (default: dist/Reawa.app)
  --sign "<codesign identity>"        Sign the app bundle and executable
  --provisioning-profile <path>       Copy a provisioning profile into the app bundle
  --entitlements <path>               Entitlements plist for signing the executable
  --open                              Open the built app after packaging
  --show-entitlements                 Print executable entitlements after signing
  --help                              Show this help

Notes:
  - Native Stylus will not work from `swift run reawa`.
  - To test Native Stylus, you need:
      1. a signed `.app` bundle
      2. Apple approval for `com.apple.developer.hid.virtual.device`
      3. a provisioning profile that includes that entitlement
      4. local approval for the macOS Accessibility permission prompt
EOF
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --configuration)
            CONFIGURATION="$2"
            shift 2
            ;;
        --output)
            OUTPUT_APP="$2"
            shift 2
            ;;
        --sign)
            SIGN_IDENTITY="$2"
            shift 2
            ;;
        --provisioning-profile)
            PROVISIONING_PROFILE="$2"
            shift 2
            ;;
        --entitlements)
            ENTITLEMENTS_FILE="$2"
            shift 2
            ;;
        --open)
            OPEN_AFTER_BUILD=1
            shift
            ;;
        --show-entitlements)
            SHOW_ENTITLEMENTS=1
            shift
            ;;
        --help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown argument: $1" >&2
            usage >&2
            exit 1
            ;;
    esac
done

case "$CONFIGURATION" in
    debug|release) ;;
    *)
        echo "Unsupported configuration: $CONFIGURATION" >&2
        exit 1
        ;;
esac

cd "$ROOT_DIR"
swift build -c "$CONFIGURATION" --product reawa

BUILD_DIR=""
for candidate in "$ROOT_DIR"/.build/*-apple-macosx/"$CONFIGURATION"; do
    if [ -d "$candidate" ]; then
        BUILD_DIR="$candidate"
        break
    fi
done

if [ -z "$BUILD_DIR" ]; then
    echo "Could not locate the SwiftPM build output directory." >&2
    exit 1
fi

EXECUTABLE_PATH="$BUILD_DIR/reawa"
RESOURCE_BUNDLE_PATH="$BUILD_DIR/Reawa_ReawaApp.bundle"

if [ ! -f "$EXECUTABLE_PATH" ]; then
    echo "Built executable not found at: $EXECUTABLE_PATH" >&2
    exit 1
fi

APP_DIR="$OUTPUT_APP"
CONTENTS_DIR="$APP_DIR/Contents"
MACOS_DIR="$CONTENTS_DIR/MacOS"
RESOURCES_DIR="$CONTENTS_DIR/Resources"

rm -rf "$APP_DIR"
mkdir -p "$MACOS_DIR" "$RESOURCES_DIR"

cp "$ROOT_DIR/Config/Info.plist" "$CONTENTS_DIR/Info.plist"
cp "$EXECUTABLE_PATH" "$MACOS_DIR/reawa"

if [ -d "$RESOURCE_BUNDLE_PATH" ]; then
    cp -R "$RESOURCE_BUNDLE_PATH" "$RESOURCES_DIR/"
fi

if [ -n "$PROVISIONING_PROFILE" ]; then
    if [ ! -f "$PROVISIONING_PROFILE" ]; then
        echo "Provisioning profile not found: $PROVISIONING_PROFILE" >&2
        exit 1
    fi
    cp "$PROVISIONING_PROFILE" "$CONTENTS_DIR/embedded.provisionprofile"
fi

if [ -n "$SIGN_IDENTITY" ]; then
    if [ ! -f "$ENTITLEMENTS_FILE" ]; then
        echo "Entitlements file not found: $ENTITLEMENTS_FILE" >&2
        exit 1
    fi

    if [ -z "$PROVISIONING_PROFILE" ]; then
        echo "Warning: signing without a provisioning profile. Restricted Virtual HID entitlements normally require one." >&2
    fi

    codesign --force --sign "$SIGN_IDENTITY" --timestamp=none --entitlements "$ENTITLEMENTS_FILE" "$MACOS_DIR/reawa"

    if [ -d "$RESOURCES_DIR/Reawa_ReawaApp.bundle" ]; then
        codesign --force --sign "$SIGN_IDENTITY" --timestamp=none "$RESOURCES_DIR/Reawa_ReawaApp.bundle"
    fi

    codesign --force --sign "$SIGN_IDENTITY" --timestamp=none "$APP_DIR"
fi

echo "Built app bundle: $APP_DIR"
echo "Executable path: $MACOS_DIR/reawa"

if [ "$SHOW_ENTITLEMENTS" -eq 1 ]; then
    codesign -d --entitlements :- "$MACOS_DIR/reawa"
fi

if [ "$OPEN_AFTER_BUILD" -eq 1 ]; then
    open "$APP_DIR"
fi
