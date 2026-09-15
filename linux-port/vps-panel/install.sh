#!/bin/sh
# Installs or updates the M2 VPS Panel as a systemd service.
#
# Idempotent: re-run it after copying a new app.py to pick up code changes -
# it reuses the existing password and secret rather than generating fresh
# ones, so a re-run never logs the operator out or changes the URL.
#
# Usage: sudo sh install.sh [/path/to/docker-compose-project]
set -eu

SRC_DIR="$(cd "$(dirname "$0")" && pwd)"
INSTALL_DIR="/opt/m2-vps-panel"
ENV_FILE="/etc/m2-vps-panel.env"
SERVICE_FILE="/etc/systemd/system/m2-vps-panel.service"
STATE_DIR="/var/lib/m2-vps-panel"
PANEL_PORT="${M2_VPS_PANEL_PORT:-9797}"

if [ "$(id -u)" != "0" ]; then
    echo "Uruchom przez sudo: sudo sh install.sh" >&2
    exit 1
fi

# --- find the docker-compose project this panel will control ---------------
COMPOSE_DIR="${1:-}"
if [ -z "$COMPOSE_DIR" ]; then
    for candidate in \
        /home/*/metin2-playerbots/linux-port/docker \
        /opt/metin2-playerbots/linux-port/docker \
        /opt/metin2/stack/linux-port/docker
    do
        if [ -f "$candidate/docker-compose.yml" ]; then
            COMPOSE_DIR="$candidate"
            break
        fi
    done
fi
if [ -z "$COMPOSE_DIR" ] || [ ! -f "$COMPOSE_DIR/docker-compose.yml" ]; then
    echo "Nie znaleziono docker-compose.yml. Podaj ścieżkę: sudo sh install.sh /sciezka/do/docker" >&2
    exit 1
fi
echo "Stos: $COMPOSE_DIR"

# --- app files ---------------------------------------------------------
mkdir -p "$INSTALL_DIR" "$STATE_DIR"
cp "$SRC_DIR/app.py" "$INSTALL_DIR/app.py"
cp "$SRC_DIR/requirements.txt" "$INSTALL_DIR/requirements.txt"

if [ ! -x "$INSTALL_DIR/venv/bin/python3" ]; then
    python3 -m venv "$INSTALL_DIR/venv"
fi
"$INSTALL_DIR/venv/bin/pip" install --quiet --upgrade pip
"$INSTALL_DIR/venv/bin/pip" install --quiet -r "$INSTALL_DIR/requirements.txt"

# --- password + secret: generated once, kept on every re-run ---------------
NEW_PASSWORD=""
if [ -f "$ENV_FILE" ] && grep -q '^M2_VPS_PANEL_PASSWORD_HASH=' "$ENV_FILE"; then
    PASSWORD_HASH="$(grep '^M2_VPS_PANEL_PASSWORD_HASH=' "$ENV_FILE" | cut -d= -f2-)"
    SECRET="$(grep '^M2_VPS_PANEL_SECRET=' "$ENV_FILE" | cut -d= -f2-)"
else
    NEW_PASSWORD="$("$INSTALL_DIR/venv/bin/python3" -c 'import secrets; print(secrets.token_urlsafe(12))')"
    PASSWORD_HASH="$("$INSTALL_DIR/venv/bin/python3" -c "
import hashlib, secrets, sys
password = sys.argv[1]
salt = secrets.token_hex(16)
digest = hashlib.sha256((salt + password).encode()).hexdigest()
print(f'{salt}:{digest}')
" "$NEW_PASSWORD")"
    SECRET="$("$INSTALL_DIR/venv/bin/python3" -c 'import secrets; print(secrets.token_hex(32))')"
fi

# git-setup.sh dopisuje tu M2_GIT_*, operator może dopisać M2_VPS_PANEL_BRAND.
# Ten plik jest przepisywany od zera przy każdym uruchomieniu instalatora, więc
# bez przeniesienia tych linii aktualizacja panelu wyłączałaby jego własny
# aktualizator -- po cichu, bo panel po prostu wróciłby do wartości domyślnych.
KEPT_LINES=""
if [ -f "$ENV_FILE" ]; then
    KEPT_LINES="$(grep -E '^(M2_GIT_|M2_VPS_PANEL_BRAND=)' "$ENV_FILE" || true)"
fi

cat > "$ENV_FILE" <<EOF
M2_COMPOSE_DIR=$COMPOSE_DIR
M2_VPS_PANEL_PORT=$PANEL_PORT
M2_VPS_PANEL_PASSWORD_HASH=$PASSWORD_HASH
M2_VPS_PANEL_SECRET=$SECRET
M2_VPS_PANEL_STATE_DIR=$STATE_DIR
EOF
[ -n "$KEPT_LINES" ] && printf '%s\n' "$KEPT_LINES" >> "$ENV_FILE"
chmod 600 "$ENV_FILE"

cp "$SRC_DIR/m2-vps-panel.service" "$SERVICE_FILE"
systemctl daemon-reload
systemctl enable --now m2-vps-panel.service
systemctl restart m2-vps-panel.service

echo ""
echo "=== M2 VPS Panel zainstalowany ==="
echo "URL: http://<adres-vps>:$PANEL_PORT"
if [ -n "$NEW_PASSWORD" ]; then
    echo "Hasło (zanotuj, nie pokaże się drugi raz): $NEW_PASSWORD"
else
    echo "Hasło bez zmian (odczytano istniejącą konfigurację)."
fi
echo "Status: systemctl status m2-vps-panel"
echo "Autostart po restarcie VPS: systemctl is-enabled m2-vps-panel"
if grep -q '^M2_GIT_DIR=' "$ENV_FILE" 2>/dev/null; then
    echo "Aktualizacje z GitHuba: włączone ($(grep '^M2_GIT_DIR=' "$ENV_FILE" | cut -d= -f2-))"
else
    echo "Aktualizacje z GitHuba: nieskonfigurowane — sudo sh $SRC_DIR/git-setup.sh <url-repo>"
fi
