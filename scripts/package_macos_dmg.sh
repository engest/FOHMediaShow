#!/usr/bin/env bash
set -euo pipefail

APP_BUNDLE="${1:-}"
OUTPUT_DMG="${2:-}"
ICON_PATH="${3:-}"
VOL_NAME="${4:-FOHMedia}"

if [ -z "$APP_BUNDLE" ] || [ -z "$OUTPUT_DMG" ]; then
    echo "Usage: $0 <AppBundlePath> <OutputDmgPath> [IconPath] [VolumeName]"
    exit 1
fi

STAGING_DIR="$(mktemp -d /tmp/fohmedia_dmg_staging.XXXXXX)"
trap 'rm -rf "$STAGING_DIR"' EXIT

STAGING_APP="$STAGING_DIR/$(basename "$APP_BUNDLE")"

echo "Staging DMG contents at $STAGING_DIR..."
cp -R "$APP_BUNDLE" "$STAGING_APP"
ln -s /Applications "$STAGING_DIR/Applications"

# Ensure PkgInfo and Bundle attribute are set on the application
echo -n "APPL????" > "$STAGING_APP/Contents/PkgInfo"
SetFile -a B "$STAGING_APP" 2>/dev/null || true

if [ -n "$ICON_PATH" ] && [ -f "$ICON_PATH" ]; then
    echo "Setting DMG volume icon and application icon from $ICON_PATH..."
    cp "$ICON_PATH" "$STAGING_DIR/.VolumeIcon.icns"
    SetFile -c icnC "$STAGING_DIR/.VolumeIcon.icns" 2>/dev/null || true
    SetFile -a C "$STAGING_DIR" 2>/dev/null || true

    # Explicitly set icon on staged .app bundle so Finder recognizes it immediately
    swift -e 'import Cocoa; NSWorkspace.shared.setIcon(NSImage(contentsOfFile: CommandLine.arguments[1]), forFile: CommandLine.arguments[2], options: [])' "$ICON_PATH" "$STAGING_APP" 2>/dev/null || true
    SetFile -a C "$STAGING_APP" 2>/dev/null || true
fi

# Clean any non-executable resource files accidentally placed in Contents/MacOS
find "$STAGING_DIR" -path "*/Contents/MacOS/*" ! -name "FOHMedia" -type f -name "*.ttf" -delete 2>/dev/null || true
find "$STAGING_DIR" -path "*/Contents/MacOS/*" ! -name "FOHMedia" -type f -name "*.png" -delete 2>/dev/null || true
find "$STAGING_DIR" -path "*/Contents/MacOS/*" ! -name "FOHMedia" -type f -name "*.json" -delete 2>/dev/null || true
find "$STAGING_DIR" -path "*/Contents/MacOS/*" ! -name "FOHMedia" -type f -name "*.yaml" -delete 2>/dev/null || true

# Ensure all files and directories in the DMG are world-readable (mktemp creates 0700 which prevents iconservicesagent from reading icons)
chmod -R ugo+rX "$STAGING_DIR"
if [ -f "$STAGING_DIR/.VolumeIcon.icns" ]; then
    chmod 644 "$STAGING_DIR/.VolumeIcon.icns"
fi

rm -f "$OUTPUT_DMG"
echo "Creating compressed UDZO DMG at $OUTPUT_DMG..."
hdiutil create -volname "$VOL_NAME" -srcfolder "$STAGING_DIR" -ov -format UDZO "$OUTPUT_DMG"

if [ -n "$ICON_PATH" ] && [ -f "$ICON_PATH" ]; then
    echo "Setting icon on DMG file..."
    swift -e 'import Cocoa; NSWorkspace.shared.setIcon(NSImage(contentsOfFile: CommandLine.arguments[1]), forFile: CommandLine.arguments[2], options: [])' "$ICON_PATH" "$OUTPUT_DMG" 2>/dev/null || true
fi

echo "Successfully created DMG at $OUTPUT_DMG"
