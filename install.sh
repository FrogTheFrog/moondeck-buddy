#!/usr/bin/env bash
# Installs MoonDeckBuddy for the current user and keeps it updated via a systemd timer.
# Usage: curl -fsSL https://raw.githubusercontent.com/MoonDeckTeam/moondeck-buddy/main/install.sh | bash
#        ... | bash -s -- --uninstall
set -euo pipefail

REPO="MoonDeckTeam/moondeck-buddy"
SCRIPT_URL="https://raw.githubusercontent.com/$REPO/main/install.sh"
APP_DIR="$HOME/.local/share/moondeckbuddy"
BIN_DIR="$HOME/.local/bin"
DESKTOP_FILE="$HOME/.local/share/applications/MoonDeckBuddy.desktop"
UNIT_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/systemd/user"
APPIMAGE="$APP_DIR/MoonDeckBuddy.AppImage"
UPDATER="$APP_DIR/install.sh"

install_fuse() {
    # The AppImage runtime is static and only needs fusermount
    if command -v fusermount3 >/dev/null || command -v fusermount >/dev/null; then
        return
    fi

    # shellcheck source=/dev/null
    . /etc/os-release
    case " $ID ${ID_LIKE:-} " in
        *" arch "*) sudo pacman -S --needed --noconfirm fuse3 ;;
        *" fedora "* | *" rhel "*) sudo dnf install -y fuse3 ;;
        *" debian "* | *" ubuntu "*) sudo apt-get update && sudo apt-get install -y fuse3 ;;
        *) echo "Unsupported distro '$ID', please install fuse3 manually." >&2 && exit 1 ;;
    esac
}

update() {
    local release url version
    release="$(curl -fsSL "https://api.github.com/repos/$REPO/releases/latest")"
    # `|| true`, otherwise pipefail exits silently before the error below can be printed
    url="$(grep -o '"browser_download_url": *"[^"]*x86_64\.AppImage"' <<<"$release" | cut -d'"' -f4 || true)"
    version="$(grep -o '"tag_name": *"[^"]*"' <<<"$release" | cut -d'"' -f4 || true)"
    [ -n "$url" ] || { echo "No AppImage found in the latest release." >&2 && exit 1; }

    if [ -x "$APPIMAGE" ] && [ "$(cat "$APP_DIR/version" 2>/dev/null)" = "$version" ]; then
        echo "MoonDeckBuddy $version is up to date."
        return
    fi

    # Do not restart Buddy in the middle of a stream, the next run will try again
    if [ -x "$APPIMAGE" ] && pgrep -x MoonDeckStream >/dev/null; then
        echo "A stream is running, postponing the update to $version."
        return
    fi

    echo "Installing MoonDeckBuddy $version..."
    curl -fsSL "$url" -o "$APPIMAGE.tmp"
    chmod +x "$APPIMAGE.tmp"
    mv -f "$APPIMAGE.tmp" "$APPIMAGE"
    echo "$version" >"$APP_DIR/version"

    # The timer runs the local copy of this script, so only refresh it from a released tag (older ones lack it)
    if curl -fsSL "https://raw.githubusercontent.com/$REPO/$version/install.sh" -o "$UPDATER.tmp" 2>/dev/null; then
        mv -f "$UPDATER.tmp" "$UPDATER"
    else
        rm -f "$UPDATER.tmp"
    fi

    # Pick up the new version right away
    systemctl --user try-restart moondeckbuddy.service 2>/dev/null || true
}

install_buddy() {
    [ "$(id -u)" -ne 0 ] || { echo "Please run as your normal user, not as root." >&2 && exit 1; }
    install_fuse
    mkdir -p "$APP_DIR" "$BIN_DIR" "$UNIT_DIR" "$(dirname "$DESKTOP_FILE")"
    update

    # Keep a local copy for the update timer, so it never runs unreleased code from main
    if [ -f "${BASH_SOURCE[0]:-}" ]; then
        cp -f "${BASH_SOURCE[0]}" "$UPDATER.tmp"
    else
        curl -fsSL "$SCRIPT_URL" -o "$UPDATER.tmp"
    fi
    mv -f "$UPDATER.tmp" "$UPDATER"

    # The binary to run is picked by the symlink name
    ln -sf "$APPIMAGE" "$BIN_DIR/MoonDeckBuddy"
    ln -sf "$APPIMAGE" "$BIN_DIR/MoonDeckStream"

    curl -fsSL "https://raw.githubusercontent.com/$REPO/main/resources/icons/moondeckbuddy-256.png" -o "$APP_DIR/moondeckbuddy.png"
    cat >"$DESKTOP_FILE" <<EOF
[Desktop Entry]
Name=MoonDeckBuddy
Type=Application
Exec=$BIN_DIR/MoonDeckBuddy
Icon=$APP_DIR/moondeckbuddy.png
Categories=Utility
EOF

    cat >"$UNIT_DIR/moondeckbuddy-update.service" <<EOF
[Unit]
Description=Update MoonDeckBuddy

[Service]
Type=oneshot
ExecStart=/bin/bash "$UPDATER" --update
EOF

    cat >"$UNIT_DIR/moondeckbuddy-update.timer" <<EOF
[Unit]
Description=Check for MoonDeckBuddy updates daily

[Timer]
OnCalendar=daily
RandomizedDelaySec=1h
Persistent=true

[Install]
WantedBy=timers.target
EOF

    systemctl --user daemon-reload
    systemctl --user enable --now moondeckbuddy-update.timer

    # Safe to repeat, an existing entry is updated instead of added again
    "$APPIMAGE" --register-stream-app \
        || echo "Could not add MoonDeckStream to Sunshine, Apollo or Vibepollo, please add it manually." >&2

    echo "Done! Start it with: $BIN_DIR/MoonDeckBuddy"
}

uninstall() {
    # Also stops instances that were not started by systemd
    "$APPIMAGE" --close-all >/dev/null 2>&1 || true

    # One by one, as systemctl skips stopping all of them if any unit does not exist
    for unit in moondeckbuddy-update.timer moondeckbuddy.service moondeckbuddy-gui-session.service; do
        systemctl --user disable --now "$unit" 2>/dev/null || true
    done
    rm -rf "$APP_DIR" "$BIN_DIR/MoonDeckBuddy" "$BIN_DIR/MoonDeckStream" "$DESKTOP_FILE" \
        "$UNIT_DIR"/moondeckbuddy{,-gui-session,-update}.service "$UNIT_DIR/moondeckbuddy-update.timer"
    systemctl --user daemon-reload
    echo "MoonDeckBuddy has been removed."
}

case "${1:-}" in
    --update) update ;;
    --uninstall) uninstall ;;
    *) install_buddy ;;
esac
