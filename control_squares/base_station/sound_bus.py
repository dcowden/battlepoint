from nicegui import ui, app, Client
from starlette.staticfiles import StaticFiles
from typing import Optional, Callable
import random
from battlepoint_game import SOUND_MAP  # or adjust import
from battlepoint_game import SoundSystem
# Mount sounds
from multimode_app_static import AUDIO_JS

app.mount('/sounds', StaticFiles(directory='sounds'), name='sounds')


class BrowserSoundClient:
    def __init__(self, client_id: int, audio: ui.audio):
        self.client_id = client_id
        self.audio = audio
        self.enabled = False

from typing import Dict, Optional
import random

from nicegui import ui, app, background_tasks, Client  # make sure background_tasks, Client are imported

class BrowserSoundBus:
    def __init__(self, base_url: str = '/sounds'):
        self.base_url = base_url.rstrip('/')
        self.clients: dict[str, Client] = {}
        self.enabled: set[str] = set()
        self.volume: int = 10
        self.sound_map = SOUND_MAP
        self._menu_tracks = list(range(18, 26))

    def _src_for_id(self, sound_id: int) -> str | None:
        filename = self.sound_map.get(sound_id)
        if not filename:
            print(f"[BROWSER_SOUND] unknown sound id {sound_id}")
            return None
        return f'{self.base_url}/{filename}'

    def register_client(self, client: Client) -> None:
        self.clients[client.id] = client
        print(f"[BROWSER_SOUND] register client {client.id}")
        # do NOT auto-enable here; let attach_sound_opt_in handle it

    def unregister_client(self, client: Client) -> None:
        cid = client.id
        if cid in self.clients:
            del self.clients[cid]
        self.enabled.discard(cid)
        print(f"[BROWSER_SOUND] unregister client {cid}")

    def enable_for_client(self, client_id: str, from_auto: bool = False) -> None:
        if client_id not in self.clients:
            print(f"[BROWSER_SOUND] enable_for_client({client_id}) skipped (no client)")
            return
        self.enabled.add(client_id)
        print(f"[BROWSER_SOUND] {'auto-' if from_auto else ''}enabled client {client_id}")

    def disable_for_client(self, client_id: str) -> None:
        self.enabled.discard(client_id)
        print(f"[BROWSER_SOUND] disabled client {client_id}")

    def set_volume(self, volume: int) -> None:
        self.volume = max(0, min(30, int(volume)))
        print(f"[BROWSER_SOUND] set_volume({self.volume})")

    def stop(self) -> None:
        # stop "music" channel on all clients
        print("[BROWSER_SOUND] stop() music")
        for cid, client in list(self.clients.items()):
            try:
                with client:
                    client.run_javascript("""
                        (function() {
                          try {
                            if (!window.bpChannels) return;
                            const m = window.bpChannels["music"];
                            if (m) {
                              try { m.pause(); m.currentTime = 0; } catch (e) {}
                              delete window.bpChannels["music"];
                            }
                          } catch (e) {
                            console.error('bp stop music error', e);
                          }
                        })();
                    """)
            except Exception as e:
                print(f"[BROWSER_SOUND] error stop() on {cid}: {e}")
                self.enabled.discard(cid)

    def play_menu_track(self) -> None:
        if not self._menu_tracks:
            return
        sound_id = random.choice(self._menu_tracks)
        src = self._src_for_id(sound_id)
        if not src:
            return
        vol = self.volume / 30.0
        print(f"[BROWSER_SOUND] play_menu_track {src}")
        for cid in list(self.enabled):
            client = self.clients.get(cid)
            if not client:
                self.enabled.discard(cid)
                continue
            try:
                with client:
                    client.run_javascript(f"""
                        (function() {{
                          try {{
                            window.bpChannels = window.bpChannels || {{}};
                            const existing = window.bpChannels["music"];
                            if (existing) {{
                              try {{ existing.pause(); existing.currentTime = 0; }} catch (e) {{}}
                            }}
                            const a = new Audio("{src}");
                            a.volume = {vol:.3f};
                            a.loop = true;
                            a.play().catch(e => console.warn("music blocked", e));
                            window.bpChannels["music"] = a;
                          }} catch (e) {{
                            console.error("music error", e);
                          }}
                        }})();
                    """)
                print(f"[BROWSER_SOUND] menu -> {cid}")
            except Exception as e:
                print(f"[BROWSER_SOUND] error (menu) {cid}: {e}")
                self.enabled.discard(cid)

    def loop(self, sound_id: int) -> None:
        self.play(sound_id)

    def queue(self, sound_id: int) -> None:
        self.play(sound_id)

    def play(self, sound_id: int) -> None:
        src = self._src_for_id(sound_id)
        if not src:
            return
        vol = self.volume / 30.0
        print(f"[BROWSER_SOUND] play {sound_id} -> {src}")

        for cid in list(self.enabled):
            client = self.clients.get(cid)
            if not client:
                self.enabled.discard(cid)
                continue
            try:
                with client:
                    client.run_javascript(f"""
                        (function() {{
                          try {{
                            const a = new Audio("{src}");
                            a.volume = {vol:.3f};
                            a.play().catch(e => console.warn("audio blocked for {cid}", e));
                          }} catch (e) {{
                            console.error("audio error for {cid}", e);
                          }}
                        }})();
                    """)
                print(f"[BROWSER_SOUND] -> {cid}")
            except Exception as e:
                print(f"[BROWSER_SOUND] error on {cid}: {e}")
                self.enabled.discard(cid)

browser_sound_bus = BrowserSoundBus( base_url='/sounds')

def _on_connect(client: Client):
    browser_sound_bus.register_client(client)

def _on_disconnect(client: Client):
    browser_sound_bus.unregister_client(client)

app.on_connect(_on_connect)
app.on_disconnect(_on_disconnect)


_audio_js_injected = False

def inject_audio_js_once():
    global _audio_js_injected
    if _audio_js_injected:
        return
    _audio_js_injected = True

    ui.add_head_html(AUDIO_JS)

SILENT_WAV = (
    "data:audio/wav;base64,"
    "UklGRiQAAABXQVZFZm10IBAAAAABAAEAIlYAAESsAAACABAAZGF0YQAAAAA="
)

def attach_sound_opt_in(on_enabled: Optional[Callable[[Client], None]] = None):
    """Client-side sound gate with real capability check.

    - If we THINK sound was enabled for this browser, we re-test.
    - If test passes -> mark 'Sound on', enable this client in bus.
    - If not -> show 'Enable sound' button.
    - On click, we only enable if the silent test succeeds.
    """

    client = ui.context.client

    # If this browser is marked as kiosk / no browser sound, bail out
    if app.storage.user.get('browser_sound_disabled'):
        # clear any persisted "enabled" flag for this browser
        app.storage.user['bp_sound_enabled'] = False

        # make sure bus doesn't think this client is enabled
        browser_sound_bus.disable_for_client(client.id)

        # also kill any music already running in this tab
        with client:
            client.run_javascript("""
                (function () {
                  try {
                    if (!window.bpChannels) return;
                    const m = window.bpChannels["music"];
                    if (m) {
                      try { m.pause(); m.currentTime = 0; } catch (e) {}
                      delete window.bpChannels["music"];
                    }
                  } catch (e) {
                    console.error('bp kiosk stop music error', e);
                  }
                })();
            """)

        return

    # Small inline UI in whatever bar you call this from
    with ui.row().classes('items-center gap-1') as container:
        status = ui.label().classes('text-xs')
        btn = ui.button(
            'Enable sound',
        ).props('color=yellow-7 unelevated').classes('text-xs')

    async def try_enable_from_gesture(_e):
        """Run when user taps the button."""
        # Use THIS client's JS context directly
        ok = await client.run_javascript(f"""
        (async () => {{
          try {{
            const a = new Audio();
            a.src = "{SILENT_WAV}";
            a.volume = 0.0;
            await a.play();
            return true;
          }} catch (e) {{
            console.warn('bp prime failed', e);
            return false;
          }}
        }})();
        """, timeout=3.0)

        if ok:
            with client:
                app.storage.user['bp_sound_enabled'] = True
                browser_sound_bus.enable_for_client(client.id)

                status.set_text('🔊 Sound on')
                status.style(
                    'padding:2px 8px;'
                    'border-radius:999px;'
                    'background:#2e7d32;'
                    'color:#fff;'
                    'font-size:0.7rem;'
                )
                btn.set_visibility(False)

                if on_enabled is not None:
                    on_enabled(client)

            print(f"[BROWSER_SOUND] primed+enabled {client.id}")
        else:
            with client:
                status.set_text('🔇 Sound blocked. Check site/audio settings.')
                status.style('font-size:0.7rem;color:#fdd835;')
                btn.set_visibility(True)
            print(f"[BROWSER_SOUND] prime FAILED for {client.id}")

    async def auto_check_existing():
        """On load: if flag is set, confirm it's still valid."""
        # If no previous flag: just show the prompt
        if not app.storage.user.get('bp_sound_enabled'):
            with client:
                status.set_text('🔇 Tap to enable sound')
                status.style('font-size:0.7rem;color:#fdd835;')
                btn.set_visibility(True)
            return

        ok = await client.run_javascript(f"""
        (async () => {{
          try {{
            const a = new Audio();
            a.src = "{SILENT_WAV}";
            a.volume = 0.0;
            await a.play();
            return true;
          }} catch (e) {{
            console.warn('bp auto-check failed', e);
            return false;
          }}
        }})();
        """, timeout=3.0)

        if ok:
            with client:
                browser_sound_bus.enable_for_client(client.id, from_auto=True)

                status.set_text('🔊 Sound on')
                status.style(
                    'padding:2px 8px;'
                    'border-radius:999px;'
                    'background:#2e7d32;'
                    'color:#fff;'
                    'font-size:0.7rem;'
                )
                btn.set_visibility(False)

            print(f"[BROWSER_SOUND] auto-validated {client.id}")
        else:
            # Stored flag is wrong; force explicit click
            with client:
                app.storage.user['bp_sound_enabled'] = False
                browser_sound_bus.disable_for_client(client.id)

                status.set_text('🔇 Tap to enable sound')
                status.style('font-size:0.7rem;color:#fdd835;')
                btn.set_visibility(True)

                if on_enabled is not None:
                    on_enabled(client)

            print(f"[BROWSER_SOUND] auto-check failed; require click for {client.id}")

    # Hook handlers: no create_task, NiceGUI preserves context
    btn.on('click', try_enable_from_gesture)

    # Run auto-check once after mount
    ui.timer(0.2, auto_check_existing, once=True)


class CompositeSoundSystem:
    """Fan out all sound calls to both local (pygame) and browser bus."""

    def __init__(self, local: SoundSystem, browser: BrowserSoundBus):
        self.local = local
        self.browser = browser

    def play(self, sound_id: int):
        self.local.play(sound_id)
        self.browser.play(sound_id)

    def queue(self, sound_id: int):
        self.local.queue(sound_id)
        self.browser.queue(sound_id)

    def loop(self, sound_id: int):
        self.local.loop(sound_id)
        self.browser.loop(sound_id)

    def play_menu_track(self):
        self.local.play_menu_track()
        self.browser.play_menu_track()

    def set_volume(self, volume: int):
        self.local.set_volume(volume)
        self.browser.set_volume(volume)

    def stop(self):
        self.local.stop()
        self.browser.stop()