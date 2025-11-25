#!/bin/bash
export DISPLAY=:0

# Make sure a DBus session exists (Chromium needs this to load its Pulse backend)
if [ -z "$DBUS_SESSION_BUS_ADDRESS" ]; then
    eval "$(dbus-launch --sh-syntax)"
fi

# Make sure Chromium sees the correct pulse socket
#export PULSE_SERVER=unix:/run/user/$(id -u)/pulse/native

xset -dpms
xset s off
xset s noblank

URL="http://game?kiosk=1"

#pactl set-default-sink HDMI-Playback
#pactl suspend-sink HDMI-Playback false

bash /home/orangepi/battlepoint/start_bp.sh  &
BACKEND_PID=$!

echo "** Backend started with PID ${BACKEND_PID} **"

cleanup() {
  echo "** Cleaning up backend (PID ${BACKEND_PID}) **"
  if kill -0 "${BACKEND_PID}" 2>/dev/null; then
    kill "${BACKEND_PID}" 2>/dev/null || true
    # optional: wait so it doesn't become a zombie
    wait "${BACKEND_PID}" 2>/dev/null || true
  fi
}
trap cleanup EXIT INT TERM


echo "** Waiting for backend server to start ... ***"
sleep 5
CHROMIUM_BIN="$(command -v chromium || command -v chromium-browser || command -v chromium-browser-stable || true)"

if [[ -z "${CHROMIUM_BIN}" ]]; then
  echo "Chromium not found."
  exit 1
fi

"${CHROMIUM_BIN}" \
  --no-first-run \
  --no-default-browser-check \
  --noerrdialogs \
  --disable-infobars \
  --kiosk \
  --use-pulseaudio \
  --enable-features=UsePulseAudio \
  --disable-features=MediaEngagementBypassAutoplayPolicies \
  --disable-features=PreloadmediaEngagementData \
  --autoplay-policy=no-user-gesture-required \
  "${URL}"
