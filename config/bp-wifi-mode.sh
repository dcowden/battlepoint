#!/bin/bash
set -euo pipefail

MODE="${1:-}"

if [[ "$MODE" != "ap" && "$MODE" != "client" ]]; then
  echo "Usage: $0 ap|client"
  exit 1
fi

# Detect Wi-Fi interface (same trick as your setup script)
WLAN_IF=$(iw dev 2>/dev/null | awk '$1=="Interface"{print $2}' | head -n1 || true)

if [[ -z "$WLAN_IF" ]]; then
  echo "ERROR: No Wi-Fi interface found via 'iw dev'."
  exit 1
fi

echo "[*] Using Wi-Fi interface: ${WLAN_IF}"

case "$MODE" in
  ap)
    echo "[*] Switching to AP mode (hostapd + dnsmasq)..."

    # Kill anything that might own the interface for client mode
    systemctl stop wpa_supplicant.service 2>/dev/null || true
    systemctl stop NetworkManager.service 2>/dev/null || true

    # Reset interface and give it the AP address (in case networking.service
    # or /etc/network/interfaces.d don't bring it up fast enough)
    ip addr flush dev "$WLAN_IF" 2>/dev/null || true
    ip link set "$WLAN_IF" down 2>/dev/null || true
    ip link set "$WLAN_IF" up

    # Ensure static IP for AP
    ip addr add 10.10.0.1/24 dev "$WLAN_IF" 2>/dev/null || true

    # Bring up dnsmasq + hostapd
    systemctl restart dnsmasq.service
    systemctl restart hostapd.service

    echo "[*] AP mode enabled. SSID should be broadcasting via hostapd."
    ;;

  client)
    echo "[*] Switching to client mode (wpa_supplicant / NetworkManager)..."

    # Drop AP services
    systemctl stop hostapd.service 2>/dev/null || true
    systemctl stop dnsmasq.service 2>/dev/null || true

    # Reset interface
    ip addr flush dev "$WLAN_IF" 2>/dev/null || true
    ip link set "$WLAN_IF" down 2>/dev/null || true
    ip link set "$WLAN_IF" up

    # Option A: let NetworkManager handle Wi-Fi (if you’re using it)
    systemctl start NetworkManager.service 2>/dev/null || true

    # Option B: direct wpa_supplicant + DHCP (uncomment if you prefer manual control)
    # wpa_supplicant -B -i "$WLAN_IF" -c /etc/wpa_supplicant/wpa_supplicant.conf
    # dhclient "$WLAN_IF"

    echo "[*] Client mode enabled. Use NM/wifi GUI or wpa_supplicant config to join a network."
    ;;
esac
