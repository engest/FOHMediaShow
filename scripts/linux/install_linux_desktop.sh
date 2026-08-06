#!/bin/bash

# Define paths
APP_NAME="FOHMediaShow"
EXE_NAME="FOHMedia"
PROJECT_DIR=$(pwd)
BUILD_DIR="${PROJECT_DIR}/cmake-build-debug"
ICON_SOURCE="${PROJECT_DIR}/Media/logo0_192.png"

DESKTOP_FILE_DIR="${HOME}/.local/share/applications"
ICON_DIR="${HOME}/.local/share/icons/hicolor/192x192/apps"

# Ensure directories exist
mkdir -p "${DESKTOP_FILE_DIR}"
mkdir -p "${ICON_DIR}"

# Copy the icon
cp "${ICON_SOURCE}" "${ICON_DIR}/${APP_NAME}.png"
echo "Copied icon to ${ICON_DIR}/${APP_NAME}.png"

# Create the .desktop file
cat <<EOF > "${DESKTOP_FILE_DIR}/${APP_NAME}.desktop"
[Desktop Entry]
Version=1.0
Type=Application
Name=FOH Media
Comment=Front of House Media Presentation Software
Exec=${BUILD_DIR}/${EXE_NAME}
Icon=${APP_NAME}
Terminal=false
StartupWMClass=FOHMedia
Categories=AudioVideo;Presentation;
EOF

echo "Created desktop entry at ${DESKTOP_FILE_DIR}/${APP_NAME}.desktop"

# Update desktop database
if command -v update-desktop-database &> /dev/null; then
    update-desktop-database "${DESKTOP_FILE_DIR}"
fi

# Update icon cache
if command -v gtk-update-icon-cache &> /dev/null; then
    gtk-update-icon-cache -f -t "${HOME}/.local/share/icons/hicolor"
fi

echo "Installation complete! You can now search for 'FOH Media' in your application launcher."
