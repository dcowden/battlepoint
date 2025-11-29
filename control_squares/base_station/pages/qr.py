# pages/qr.py

import io
import socket
import fastapi
import psutil           # pip install psutil
import qrcode           # pip install qrcode (NO pillow needed)
import qrcode.image.svg
from nicegui import app, ui

# ---------------------------------------------------------
# CONFIG
# ---------------------------------------------------------

PORT = 8080
QR_SIZE = 500   # QR image size in pixels


# ---------------------------------------------------------
# IP SELECTION
# ---------------------------------------------------------

def get_local_ips_psutil():
    """Return all non-loopback IPv4 addresses using psutil."""
    ips = []
    for iface, addrs in psutil.net_if_addrs().items():
        for addr in addrs:
            if addr.family == socket.AF_INET and not addr.address.startswith('127.'):
                ips.append(addr.address)
    return ips


def choose_ip_with_prefix():
    """Prefer 10.* then 192.*, then fallback, then 8.8.8.8 fallback."""
    ips = get_local_ips_psutil()

    for prefix in ('10.', '192.'):
        for ip in ips:
            if ip.startswith(prefix):
                return ip

    if ips:
        return ips[0]

    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
        s.connect(("8.8.8.8", 80))
        return s.getsockname()[0]


# ---------------------------------------------------------
# QR GENERATION (SVG, NO PIL)
# ---------------------------------------------------------

def generate_qr_svg_bytes(data: str) -> bytes:
    """Generate a QR code SVG with high error correction (H)."""
    factory = qrcode.image.svg.SvgImage
    qr = qrcode.QRCode(
        version=None,
        box_size=10,
        border=4,
        error_correction=qrcode.constants.ERROR_CORRECT_H,
    )
    qr.add_data(data)
    qr.make(fit=True)

    img = qr.make_image(image_factory=factory)

    buf = io.BytesIO()
    img.save(buf)
    buf.seek(0)
    return buf.getvalue()


def svg_response(svg_bytes: bytes):
    return fastapi.Response(
        content=svg_bytes,
        media_type="image/svg+xml",
        headers={"Cache-Control": "public, max-age=86400"},  # optional, but helps browsers
    )


# ---------------------------------------------------------
# GLOBAL INFO
# ---------------------------------------------------------

class QRInfo:
    ip = choose_ip_with_prefix()
    app_url = f"http://{ip}:{PORT}/mobile"

    wifi1_ssid = 'bluedirt-mobile'
    wifi1_pass = 'bluedirt4281!'
    wifi1_payload = f'WIFI:T:WPA;S:{wifi1_ssid};P:{wifi1_pass};;'

    wifi2_ssid = 'battlepoint'
    wifi2_pass = 'BattlePoint123'
    wifi2_payload = f'WIFI:T:WPA;S:{wifi2_ssid};P:{wifi2_pass};;'


# ---------------------------------------------------------
# PRE-GENERATED SVG CACHE
# ---------------------------------------------------------

class QRCache:
    app_svg = generate_qr_svg_bytes(QRInfo.app_url)
    wifi1_svg = generate_qr_svg_bytes(QRInfo.wifi1_payload)
    wifi2_svg = generate_qr_svg_bytes(QRInfo.wifi2_payload)


# ---------------------------------------------------------
# QR IMAGE ENDPOINTS
# ---------------------------------------------------------

@app.get('/qr/app.svg')
def qr_app_svg():
    return svg_response(QRCache.app_svg)


@app.get('/qr/wifi1.svg')
def qr_wifi1_svg():
    return svg_response(QRCache.wifi1_svg)


@app.get('/qr/wifi2.svg')
def qr_wifi2_svg():
    return svg_response(QRCache.wifi2_svg)


# ---------------------------------------------------------
# NICEGUI PAGE AT /qr (unchanged except uses QR_SIZE, QRInfo)
# ---------------------------------------------------------

@ui.page('/qr')
def qr_page():
    ui.label('SCAN A QR CODE TO CONNECT').classes(
        'text-6xl text-center mt-6 font-bold'
    )

    with ui.row().classes('justify-center gap-20 mt-12'):
        # -----------------------------------------------------
        # 1) App URL QR
        # -----------------------------------------------------
        with ui.column().classes('items-center gap-4'):
            ui.label('Join Game').classes('text-4xl font-bold')
            ui.image('/qr/app.svg').style(f'width: {QR_SIZE}px; height: {QR_SIZE}px;')
            ui.label(QRInfo.app_url).classes(
                'text-2xl font-bold max-w-xs break-all text-center'
            )

        # -----------------------------------------------------
        # 2) WiFi 1
        # -----------------------------------------------------
        with ui.column().classes('items-center gap-4'):
            ui.label(f'WiFi: {QRInfo.wifi1_ssid}').classes('text-3xl font-bold')
            ui.image('/qr/wifi1.svg').style(f'width: {QR_SIZE}px; height: {QR_SIZE}px;')
            ui.label(f'{QRInfo.wifi1_pass}').classes('text-2xl font-bold text-center')

        # -----------------------------------------------------
        # 3) WiFi 2
        # -----------------------------------------------------
        with ui.column().classes('items-center gap-4'):
            ui.label(f'WiFi: {QRInfo.wifi2_ssid}').classes('text-3xl font-bold')
            ui.image('/qr/wifi2.svg').style(f'width: {QR_SIZE}px; height: {QR_SIZE}px;')
            ui.label(f'{QRInfo.wifi2_pass}').classes('text-2xl font-bold text-center')

    ui.label(f'Using IP: {QRInfo.ip}').classes(
        'text-md text-gray-600 text-center mt-10'
    )
