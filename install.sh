#!/usr/bin/env bash
# Install (or remove) a systemd service that runs run.sh at boot.

# Usage:
#   ./install.sh              install and enable the service
#   ./install.sh --uninstall  disable and remove it

set -euo pipefail

SERVICE="doomhat"
UNIT="/etc/systemd/system/$SERVICE.service"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RUN_USER="${SUDO_USER:-$USER}"

if [ "${1:-}" = "--uninstall" ]; then
    sudo systemctl disable --now "$SERVICE" 2>/dev/null || true
    sudo rm -f "$UNIT"
    sudo systemctl daemon-reload
    echo "Removed $SERVICE service."
    exit 0
fi

if [ ! -f "$ROOT/run.sh" ]; then
    echo "run.sh not found in $ROOT" >&2
    exit 1
fi
chmod +x "$ROOT/run.sh"

echo "Building"
"$ROOT/run.sh" -b

# The user needs access to GPIO and SPI without sudo
for grp in gpio spi; do
    if getent group "$grp" >/dev/null && ! id -nG "$RUN_USER" | grep -qw "$grp"; then
        echo "Adding $RUN_USER to group $grp (log out/in or reboot to apply)"
        sudo usermod -aG "$grp" "$RUN_USER"
    fi
done

echo "== Writing $UNIT =="
sudo tee "$UNIT" >/dev/null <<EOF
[Unit]
Description=Doom on Waveshare 1.44" LCD HAT
After=multi-user.target

[Service]
Type=simple
User=$RUN_USER
WorkingDirectory=$ROOT
ExecStart=$ROOT/run.sh
Restart=on-failure
RestartSec=2
StandardOutput=null
StandardError=journal

[Install]
WantedBy=multi-user.target
EOF

sudo systemctl daemon-reload
sudo systemctl enable "$SERVICE"

echo
echo "Done. Doom will start at next boot."
echo "Start it now with:  sudo systemctl start $SERVICE"
echo "Log (errors only): journalctl -u $SERVICE -f"
echo "Remove with: ./install.sh --uninstall"