import asyncio
import sys


from nicegui import ui, app
from sound_bus import browser_sound_bus,BrowserSoundBus,CompositeSoundSystem
from battlepoint_game import EnhancedGameBackend,SoundSystem
from threecp_game import ThreeCPBackend
from clock_game import ClockBackend
from ad_game import ADBackend
from settings import UnifiedSettingsManager, ThreeCPOptions
import aiohttp


# Windows event loop policy
if sys.platform == 'win32':
    asyncio.set_event_loop_policy(asyncio.WindowsSelectorEventLoopPolicy())

native_sound = SoundSystem(base_dir='sounds')
composite_sound = CompositeSoundSystem(
    local=native_sound,
    browser=browser_sound_bus,
)

# Create backends for all modes
koth_backend = EnhancedGameBackend(sound_system = composite_sound)

threecp_backend = ThreeCPBackend(
    sound_system=koth_backend.sound_system,
    scanner=koth_backend.scanner,
)

clock_backend = ClockBackend(
    event_manager=koth_backend.event_manager
)

ad_backend = ADBackend(
    sound_system=koth_backend.sound_system,
    scanner=koth_backend.scanner,
)
composite_sound.play_menu_track()
settings_manager = UnifiedSettingsManager()

# ---- Load KOTH settings via backend ----
koth_saved = None
if hasattr(koth_backend, "load_settings"):
    koth_saved = koth_backend.load_settings()

if koth_saved:
    opts, vol, _ = koth_saved
    koth_backend.configure(opts)
    koth_backend.sound_system.set_volume(vol)

# ---- Load 3CP settings via global manager (unchanged) ----
threecp_saved = settings_manager.load_3cp_settings()
if threecp_saved:
    opts, vol, _ = threecp_saved
    threecp_backend.configure(opts)

# Shared HTTP session helper (used by pages / debug)
_session: aiohttp.ClientSession | None = None

# ========================================================================
# Pages
# ========================================================================

from pages.landing import landing_page
from pages.koth import koth_game_ui
from pages.overlays import install_winner_overlay
from pages.threecp import threecp_game_ui
from pages.ad import ad_game_ui
from pages.clock import clock_game_ui
from pages.settings import settings_ui
from pages.debug import debug_ui
from pages.mobile_pages import *

@app.get("/api/manual/state")
async def generic_manual_state():
    """Global manual state for debug page (uses KOTH backend)."""
    print("[DEBUG] /api/manual/state called (alias -> KOTH)")
    return koth_backend.get_manual_state()


@app.post("/api/manual/mode/{enabled}")
async def generic_manual_mode(enabled: bool):
    """Global manual mode toggle for debug page (uses KOTH backend)."""
    print(f"[DEBUG] /api/manual/mode/{enabled} called (alias -> KOTH)")
    koth_backend.set_manual_control(enabled)
    return koth_backend.get_manual_state()


@app.post("/api/bluetooth/start")
async def bluetooth_start():
    """Start BLE scanning (delegates to KOTH backend)."""
    print("[DEBUG] /api/bluetooth/start called")
    if hasattr(koth_backend, "start_bluetooth_scanning"):
        await koth_backend.start_bluetooth_scanning()
    else:
        print("[DEBUG] koth_backend has no start_bluetooth_scanning()")
    return {"status": "scanning"}


@app.post("/api/bluetooth/stop")
async def bluetooth_stop():
    """Stop BLE scanning (delegates to KOTH backend)."""
    print("[DEBUG] /api/bluetooth/stop called")
    if hasattr(koth_backend, "stop_bluetooth_scanning"):
        await koth_backend.stop_bluetooth_scanning()
    else:
        print("[DEBUG] koth_backend has no stop_bluetooth_scanning()")
    return {"status": "stopped"}


@app.get("/api/bluetooth/devices")
async def bluetooth_devices():
    """Return BLE devices summary for debug page (ALWAYS from KOTH backend)."""
    print("[DEBUG] /api/bluetooth/devices called (FORCED KOTH)")

    try:
        return koth_backend.get_ble_devices_summary()
    except Exception as e:
        print(f"[DEBUG] /api/bluetooth/devices ERROR: {e}")
        return {"scanning": False, "device_count": 0, "devices": []}


@app.get("/api/koth/manual/state")
async def koth_manual_state():
    print("[DEBUG] /api/koth/manual/state called")
    return koth_backend.get_manual_state()


@app.get("/api/koth/state")
async def koth_get_state():
    return koth_backend.get_state()


@app.post("/api/koth/start")
async def koth_start():
    koth_backend.start_game()
    return {"status": "started"}


@app.post("/api/koth/stop")
async def koth_stop():
    koth_backend.stop_game()
    return {"status": "stopped"}


@app.post("/api/koth/configure")
async def koth_configure(options: dict):
    from battlepoint_core import GameOptions, GameMode

    go = GameOptions(
        mode=GameMode(options.get('mode', 0)),
        capture_seconds=options.get('capture_seconds', 20),
        capture_button_threshold_seconds=options.get(
            'capture_button_threshold_seconds', 5
        ),
        time_limit_seconds=options.get('time_limit_seconds', 60),
        start_delay_seconds=options.get('start_delay_seconds', 5),
    )
    koth_backend.configure(go)
    return {"status": "configured"}


@app.post("/api/koth/settings/save")
async def koth_save_settings():
    """
    Persist current KOTH options using EnhancedGameBackend's own settings manager.
    """
    if not hasattr(koth_backend, "save_settings"):
        return {"status": "error", "reason": "koth_backend has no save_settings()"}
    success = koth_backend.save_settings()
    return {"status": "saved" if success else "error"}



@app.get("/api/koth/settings/load")
async def koth_load_settings():
    """
    Load KOTH settings via EnhancedGameBackend so we read the same file
    that /api/koth/settings/save writes.
    """
    if not hasattr(koth_backend, "load_settings"):
        return {"status": "not_found"}

    saved = koth_backend.load_settings()
    if not saved:
        return {"status": "not_found"}

    options, volume, _ = saved
    return {
        "mode": options.mode.value,
        "capture_seconds": options.capture_seconds,
        "capture_button_threshold_seconds": options.capture_button_threshold_seconds,
        "time_limit_seconds": options.time_limit_seconds,
        "start_delay_seconds": options.start_delay_seconds,
        "volume": volume,
    }



@app.post("/api/koth/manual/mode/{enabled}")
async def koth_set_manual(enabled: bool):
    print(f"[DEBUG] /api/koth/manual/mode called with enabled={enabled}")
    koth_backend.set_manual_control(enabled)
    state = koth_backend.get_manual_state()
    print(f"[DEBUG] KOTH manual state after set_manual_control: {state}")
    return state


@app.post("/api/koth/manual/red/{on}")
async def koth_manual_red(on: bool):
    koth_backend.set_manual_state(red=on)
    return koth_backend.get_manual_state()


@app.post("/api/koth/manual/blu/{on}")
async def koth_manual_blu(on: bool):
    koth_backend.set_manual_state(blu=on)
    return koth_backend.get_manual_state()


# ========================================================================
# API ENDPOINTS - 3CP
# ========================================================================

@app.get("/api/3cp/state")
async def threecp_get_state():
    return threecp_backend.get_state()


@app.post("/api/3cp/start")
async def threecp_start():
    threecp_backend.start_game()
    return {"status": "started"}


@app.post("/api/3cp/stop")
async def threecp_stop():
    threecp_backend.stop_game()
    return {"status": "stopped"}


@app.post("/api/3cp/configure")
async def threecp_configure(options: dict):
    opts = ThreeCPOptions.from_dict(options)
    threecp_backend.configure(opts)
    return {"status": "configured"}


@app.post("/api/3cp/settings/save")
async def threecp_save_settings(options: dict):
    opts = ThreeCPOptions.from_dict(options)
    threecp_backend.configure(opts)
    success = settings_manager.save_3cp_settings(opts, volume=10)
    return {"status": "saved" if success else "error"}


@app.get("/api/3cp/settings/load")
async def threecp_load_settings():
    saved = settings_manager.load_3cp_settings()
    print("[DEBUG] /api/3cp/settings/load: raw saved =", saved)

    if saved:
        options, volume, brightness = saved
        data = options.to_dict()
        data["volume"] = volume
        data["brightness"] = brightness
        print("[DEBUG] /api/3cp/settings/load: returning =", data)
        return data

    print("[DEBUG] /api/3cp/settings/load: not_found")
    return {"status": "not_found"}


@app.get("/api/3cp/manual/state")
async def threecp_manual_state():
    print("[DEBUG] /api/3cp/manual/state called")
    return threecp_backend.get_manual_state()


@app.post("/api/3cp/manual/mode/{enabled}")
async def threecp_set_manual(enabled: bool):
    print(f"[DEBUG] /api/3cp/manual/mode called with enabled={enabled}")
    threecp_backend.set_manual_control(enabled)
    state = threecp_backend.get_manual_state()
    print(f"[DEBUG] 3CP manual state after set_manual_control: {state}")
    return state


@app.post("/api/3cp/manual/{cp_index}/{team}/{on}")
async def threecp_manual_set(cp_index: int, team: str, on: bool):
    """Set manual state for a CP/team explicitly (no server-side toggling)."""
    if 0 <= cp_index < 3 and team in ['red', 'blu']:
        threecp_backend.set_manual_state(cp_index, **{team: on})
    return threecp_backend.get_manual_state()



# ========================================================================
# API ENDPOINTS - AD (Attack / Defend)
# ========================================================================

@app.get("/api/ad/state")
async def ad_get_state():
    return ad_backend.get_state()


@app.post("/api/ad/start")
async def ad_start():
    ad_backend.start_game()
    return {"status": "started"}


@app.post("/api/ad/stop")
async def ad_stop():
    ad_backend.stop_game()
    return {"status": "stopped"}


@app.post("/api/ad/configure")
async def ad_configure(options: dict):
    # Reuse ThreeCPOptions structure for AD
    opts = ThreeCPOptions.from_dict(options)
    ad_backend.configure(opts)
    return {"status": "configured"}


@app.post("/api/ad/settings/save")
async def ad_save_settings(options: dict):
    # Configure from incoming options and then let AD backend persist if it supports it
    o = ThreeCPOptions.from_dict(options)
    ad_backend.configure(o)

    success = settings_manager.save_ad_settings(o)
    return {"status": "saved" if success else "error"}


@app.get("/api/ad/settings/load")
async def ad_load_settings():

    saved = settings_manager.load_ad_settings()
    print("[DEBUG] /api/ad/settings/load: raw saved =", saved)

    if saved:
        options= saved
        data = options.to_dict()
        print("[DEBUG] /api/ad/settings/load: returning =", data)
        return data

    print("[DEBUG] /api/ad/settings/load: not_found")
    return {"status": "not_found"}


@app.get("/api/ad/manual/state")
async def ad_manual_state():
    print("[DEBUG] /api/ad/manual/state called")
    return ad_backend.get_manual_state()


@app.post("/api/ad/manual/mode/{enabled}")
async def ad_set_manual(enabled: bool):
    print(f"[DEBUG] /api/ad/manual/mode called with enabled={enabled}")
    ad_backend.set_manual_control(enabled)
    state = ad_backend.get_manual_state()
    print(f"[DEBUG] AD manual state after set_manual_control: {state}")
    return state


@app.post("/api/ad/manual/{cp_index}/{team}/{on}")
async def ad_manual_set(cp_index: int, team: str, on: bool):
    if 0 <= cp_index < 3 and team in ['red', 'blu']:
        ad_backend.set_manual_state(cp_index, **{team: on})
    return ad_backend.get_manual_state()




# ========================================================================
# API ENDPOINTS - CLOCK
# ========================================================================

@app.get("/api/clock/state")
async def clock_get_state():
    return clock_backend.get_state()


@app.post("/api/clock/configure")
async def clock_configure(options: dict):
    total = int(options.get('time_limit_seconds', 60) or 60)
    clock_backend.configure(total)
    return {"status": "configured"}


@app.post("/api/clock/start")
async def clock_start():
    clock_backend.start_game()
    return {"status": "started"}


@app.post("/api/clock/stop")
async def clock_stop():
    clock_backend.stop_game()
    return {"status": "stopped"}


# ========================================================================
# GAME LOOPS
# ========================================================================
def start_backend_loop(backend, interval: float = 0.1):
    async def loop():
        while True:
            backend.update()
            await asyncio.sleep(interval)
    asyncio.create_task(loop())

async def start_game_loops():
    start_backend_loop(koth_backend)
    start_backend_loop(threecp_backend)
    start_backend_loop(ad_backend)
    start_backend_loop(clock_backend)


app.on_startup(start_game_loops)


# ========================================================================
# RUN SERVER
# ========================================================================

if __name__ in {"__main__", "__mp_main__"}:
    ui.run(
        host='0.0.0.0',
        title='BattlePoint - Multi-Mode',
        port=8080,
        storage_secret='battlepoint_secret',
        reload=False,
        show=False
    )
