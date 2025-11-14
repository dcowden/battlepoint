#!/bin/bash
set -euo pipefail

### CONFIGURABLE BITS ###
BATTLE_USER="orangepi"
BATTLE_HOME="/home/${BATTLE_USER}"
APP_DIR="${BATTLE_HOME}/battlepoint"
APP_VENV="${APP_DIR}/venv"
BARE_REPO_DIR="${APP_DIR}/repos/battlepoint.git"
SSID="battlepoint"
WIFI_PASS="BattlePoint123"
AP_NET="10.10.0.1"
AP_CIDR="10.10.0.0/24"
HOSTNAME="game"

echo "[*] BattlePoint setup starting..."

if [[ "$(id -u)" -ne 0 ]]; then
  echo "Run as root."
  exit 1
fi

### Detect Wi-Fi interface ###
WLAN_IF=$(iw dev 2>/dev/null | awk '$1=="Interface"{print $2}' | head -n1 || true)
if [[ -z "${WLAN_IF}" ]]; then
  echo "No Wi-Fi interface detected via 'iw dev'. Fix this first."
  exit 1
fi
echo "[*] Using Wi-Fi interface: ${WLAN_IF}"

### Basic packages ###
echo "[*] Installing packages..."
apt-get update
apt-get install -y \
  sudo git python3 python3-venv python3-pip \
  nginx hostapd dnsmasq avahi-daemon openssh-server \
  xserver-xorg x11-xserver-utils xinit openbox \
  pulseaudio pavucontrol || true

# Chromium package name differences
#apt-get install -y chromium || apt-get install -y chromium-browser || true

### Create user ###
#if ! id "${BATTLE_USER}" &>/dev/null; then
#  echo "[*] Creating user ${BATTLE_USER}"
#  useradd -m -s /bin/bash "${BATTLE_USER}"
#fi

# Ensure ownership
#chown -R "${BATTLE_USER}:${BATTLE_USER}" "${BATTLE_HOME}"

### Hostname ###
echo "[*] Setting hostname to ${HOSTNAME}"
hostnamectl set-hostname "${HOSTNAME}"

# Ensure /etc/hosts sane
if ! grep -q "${HOSTNAME}" /etc/hosts; then
  cat <<EOF >> /etc/hosts
127.0.1.1   ${HOSTNAME}
EOF
fi

### Network: static IP for AP interface ###
echo "[*] Configuring static IP for ${WLAN_IF}"

mkdir -p /etc/network/interfaces.d

cat > /etc/network/interfaces.d/battlepoint-ap <<EOF
auto ${WLAN_IF}
allow-hotplug ${WLAN_IF}
iface ${WLAN_IF} inet static
    address ${AP_NET}
    netmask 255.255.255.0
EOF

# If NetworkManager exists, try to unmanage this interface
if command -v nmcli &>/dev/null; then
  echo "[connection]" > /etc/NetworkManager/conf.d/unmanage-${WLAN_IF}.conf
  echo "interface-name=${WLAN_IF}" >> /etc/NetworkManager/conf.d/unmanage-${WLAN_IF}.conf
  echo "managed=false" >> /etc/NetworkManager/conf.d/unmanage-${WLAN_IF}.conf
  systemctl restart NetworkManager || true
fi

systemctl restart networking || true

### dnsmasq: DHCP + DNS for "game" ###
echo "[*] Configuring dnsmasq..."

if [[ -f /etc/dnsmasq.conf && ! -f /etc/dnsmasq.conf.battlepoint.backup ]]; then
  mv /etc/dnsmasq.conf /etc/dnsmasq.conf.battlepoint.backup
fi

cat > /etc/dnsmasq.conf <<EOF
interface=${WLAN_IF}
bind-interfaces
dhcp-range=10.10.0.10,10.10.0.200,255.255.255.0,24h
address=/game/${AP_NET}
EOF

systemctl enable dnsmasq
systemctl restart dnsmasq

### hostapd: Wi-Fi AP ###
echo "[*] Configuring hostapd..."

cat > /etc/hostapd/hostapd.conf <<EOF
interface=${WLAN_IF}
ssid=${SSID}
hw_mode=g
channel=6
wmm_enabled=0
auth_algs=1
ignore_broadcast_ssid=0

wpa=2
wpa_passphrase=${WIFI_PASS}
wpa_key_mgmt=WPA-PSK
wpa_pairwise=TKIP
rsn_pairwise=CCMP
EOF

if grep -q "^#DAEMON_CONF" /etc/default/hostapd 2>/dev/null; then
  sed -i 's|^#DAEMON_CONF=.*|DAEMON_CONF="/etc/hostapd/hostapd.conf"|' /etc/default/hostapd
elif grep -q "^DAEMON_CONF" /etc/default/hostapd 2>/dev/null; then
  sed -i 's|^DAEMON_CONF=.*|DAEMON_CONF="/etc/hostapd/hostapd.conf"|' /etc/default/hostapd
else
  echo 'DAEMON_CONF="/etc/hostapd/hostapd.conf"' >> /etc/default/hostapd
fi

systemctl unmask hostapd || true
systemctl enable hostapd
systemctl restart hostapd

### SSH ###
#echo "[*] Enabling SSH..."
#systemctl enable ssh || systemctl enable sshd || true
#systemctl start ssh || systemctl start sshd || true

### Avahi (for game.local if needed) ###
systemctl enable avahi-daemon
systemctl restart avahi-daemon

### App directories + venv ###
echo "[*] Setting up app directory and venv..."

sudo -u "${BATTLE_USER}" mkdir -p "${APP_DIR}"

if [[ ! -d "${APP_VENV}" ]]; then
  sudo -u "${BATTLE_USER}" python3 -m venv "${APP_VENV}"
  sudo -u "${BATTLE_USER}" bash -lc "source '${APP_VENV}/bin/activate' && pip install --upgrade pip && pip install -r ${BARE_REPO_DIR}/control_squares/base_station/requirements.txt"
fi

### Systemd service for NiceGUI app ###
echo "[*] Creating battlepoint.service..."

cat > /etc/systemd/system/battlepoint.service <<EOF
[Unit]
Description=BattlePoint NiceGUI App
After=network-online.target
Wants=network-online.target

[Service]
User=${BATTLE_USER}
WorkingDirectory=${APP_DIR}/control_squares/base_station
Environment=VIRTUAL_ENV=${APP_VENV}
Environment=PATH=${APP_VENV}/bin:/usr/bin
ExecStart=${APP_VENV}/bin/python ${APP_DIR}/battlepoint_app.py
Restart=always
RestartSec=3

[Install]
WantedBy=multi-user.target
EOF

systemctl daemon-reload
systemctl enable battlepoint.service

### nginx reverse proxy: port 80 -> 127.0.0.1:8000 ###
echo "[*] Configuring nginx reverse proxy..."

rm -f /etc/nginx/sites-enabled/default || true

cat > /etc/nginx/sites-available/battlepoint <<EOF
server {
    listen 80 default_server;
    server_name ${HOSTNAME};

    location / {
        proxy_pass         http://127.0.0.1:8000;
        proxy_set_header   Host \$host;
        proxy_set_header   X-Real-IP \$remote_addr;
        proxy_set_header   X-Forwarded-For \$proxy_add_x_forwarded_for;
        proxy_http_version 1.1;
        proxy_set_header   Upgrade \$http_upgrade;
        proxy_set_header   Connection "upgrade";
    }
}
EOF

ln -sf /etc/nginx/sites-available/battlepoint /etc/nginx/sites-enabled/battlepoint

systemctl enable nginx
systemctl restart nginx

### Git bare repo + post-receive deployment hook ###
#echo "[*] Setting up git bare repo for deployments..."
#
#sudo -u "${BATTLE_USER}" mkdir -p "$(dirname "${BARE_REPO_DIR}")"
#if [[ ! -d "${BARE_REPO_DIR}" ]]; then
#  sudo -u "${BATTLE_USER}" git init --bare "${BARE_REPO_DIR}"
#fi

# Allow battlepoint to restart the service without password
cat > /etc/sudoers.d/battlepoint-battlepoint <<EOF
${BATTLE_USER} ALL=(ALL) NOPASSWD: /bin/systemctl restart battlepoint.service
EOF
chmod 440 /etc/sudoers.d/battlepoint-battlepoint

# post-receive hook
#cat > "${BARE_REPO_DIR}/hooks/post-receive" <<'EOF'
##!/bin/bash
#APP_DIR="/home/battlepoint/app"
#APP_VENV="${APP_DIR}/venv"
#
#read oldrev newrev ref
#
#if [[ "$ref" = "refs/heads/main" ]]; then
#  echo "[post-receive] Deploying main to app dir..."
#  GIT_WORK_TREE="$APP_DIR" GIT_DIR="$APP_DIR/.git" git checkout -f main || {
#      # first-time clone
#      rm -rf "$APP_DIR"
#      git clone /home/battlepoint/repos/battlepoint.git "$APP_DIR"
#  }
#
#  if [[ -d "$APP_VENV" && -f "$APP_DIR/requirements.txt" ]]; then
#    "$APP_VENV/bin/pip" install -r "$APP_DIR/requirements.txt"
#  fi
#
#  sudo systemctl restart battlepoint.service
#fi
#EOF

#chown "${BATTLE_USER}:${BATTLE_USER}" "${BARE_REPO_DIR}/hooks/post-receive"
#chmod +x "${BARE_REPO_DIR}/hooks/post-receive"

### Kiosk: Openbox + Chromium fullscreen to http://game ###
echo "[*] Setting up kiosk mode (HDMI fullscreen browser)..."

sudo -u "${BATTLE_USER}" mkdir -p "${BATTLE_HOME}/.config/openbox"

# kiosk launcher script
cat > "${BATTLE_HOME}/start-kiosk.sh" <<'EOF'
#!/bin/bash
export DISPLAY=:0

xset -dpms
xset s off
xset s noblank

URL="http://game"

CHROMIUM_BIN="$(command -v chromium || command -v chromium-browser || command -v chromium-browser-stable || true)"

if [[ -z "${CHROMIUM_BIN}" ]]; then
  echo "Chromium not found."
  exit 1
fi

"${CHROMIUM_BIN}" \
  --noerrdialogs \
  --disable-infobars \
  --kiosk \
  --autoplay-policy=no-user-gesture-required \
  "${URL}"
EOF

chown "${BATTLE_USER}:${BATTLE_USER}" "${BATTLE_HOME}/start-kiosk.sh"
chmod +x "${BATTLE_HOME}/start-kiosk.sh"

# openbox autostart to run kiosk
cat > "${BATTLE_HOME}/.config/openbox/autostart" <<EOF
#!/bin/bash
/home/${BATTLE_USER}/start-kiosk.sh &
EOF

chown -R "${BATTLE_USER}:${BATTLE_USER}" "${BATTLE_HOME}/.config"

# systemd service to start X+openbox+kiosk on tty1
cat > /etc/systemd/system/kiosk.service <<EOF
[Unit]
Description=BattlePoint Kiosk
After=systemd-user-sessions.service getty@tty1.service battlepoint.service
Wants=battlepoint.service

[Service]
User=${BATTLE_USER}
PAMName=login
TTYPath=/dev/tty1
TTYReset=yes
TTYVHangup=yes
TTYVTDisallocate=yes
StandardInput=tty
StandardOutput=tty
StandardError=tty
ExecStart=/usr/bin/startx /usr/bin/openbox-session -- :0
Restart=always

[Install]
WantedBy=multi-user.target
EOF

systemctl daemon-reload
systemctl enable kiosk.service

### Audio: crude default to HDMI ###
echo "[*] Configuring default audio (basic) ..."

# Try to set a PulseAudio default sink on first boot via simple script
cat > /usr/local/bin/battlepoint-set-audio.sh <<'EOF'
#!/bin/bash
# Run once at boot; best-effort HDMI preference.
if command -v pactl &>/dev/null; then
  # Pick an HDMI-like sink if present
  SINK=$(pactl list short sinks | awk '/hdmi|HDMI/{print $1; exit}')
  if [[ -n "$SINK" ]]; then
    pactl set-default-sink "$SINK"
  fi
fi
EOF

chmod +x /usr/local/bin/battlepoint-set-audio.sh

cat > /etc/systemd/system/battlepoint-audio.service <<EOF
[Unit]
Description=Set default audio sink for BattlePoint
After=graphical.target sound.target

[Service]
Type=oneshot
ExecStart=/usr/local/bin/battlepoint-set-audio.sh

[Install]
WantedBy=multi-user.target
EOF

systemctl enable battlepoint-audio.service

echo "[*] BattlePoint setup complete."
echo
echo "Next steps on your dev machine:"
echo "  git remote add pi ${BATTLE_USER}@${HOSTNAME}:/home/${BATTLE_USER}/repos/battlepoint.git"
echo "  git push pi main"
echo
echo "Your clients should connect to Wi-Fi SSID '${SSID}' (pass: ${WIFI_PASS}) and open:  http://${HOSTNAME}"
