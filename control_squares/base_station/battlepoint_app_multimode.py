"""
BattlePoint - Multi-Mode Game Server
Supports KOTH, 3CP, and CLOCK-ONLY game modes
"""

import asyncio
import sys
from typing import Any

from nicegui import ui, app, Client
from starlette.staticfiles import StaticFiles

from battlepoint_game import EnhancedGameBackend,SOUND_MAP
from threecp_game import ThreeCPBackend
from clock_game import ClockBackend
from settings import UnifiedSettingsManager, ThreeCPOptions
import aiohttp

# Windows event loop policy
if sys.platform == 'win32':
    asyncio.set_event_loop_policy(asyncio.WindowsSelectorEventLoopPolicy())

# Mount sounds
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

    ui.add_head_html("""
    <script>
    (function() {
      if (window.bpSoundBootstrapped) return;
      window.bpSoundBootstrapped = true;

      window.bpChannels = window.bpChannels || {};

      window.bpPlaySound = function(opts) {
        try {
          const a = new Audio(opts.src);
          a.volume = (opts.volume !== undefined) ? opts.volume : 1.0;
          a.play().catch(e => {
            console.warn('bpPlaySound blocked', e);
          });
        } catch (e) {
          console.error('bpPlaySound error', e);
        }
      };

      window.bpPlayChannel = function(name, opts) {
        try {
          window.bpChannels = window.bpChannels || {};
          const existing = window.bpChannels[name];
          if (existing) {
            try {
              existing.pause();
              existing.currentTime = 0;
            } catch (e) {}
          }
          const a = new Audio(opts.src);
          a.volume = (opts.volume !== undefined) ? opts.volume : 1.0;
          a.loop = !!opts.loop;
          a.play().catch(e => {
            console.warn('bpPlayChannel blocked for', name, e);
          });
          window.bpChannels[name] = a;
        } catch (e) {
          console.error('bpPlayChannel error', name, e);
        }
      };

      window.bpStopChannel = function(name) {
        try {
          if (!window.bpChannels) return;
          const a = window.bpChannels[name];
          if (a) {
            try { a.pause(); } catch (e) {}
          }
          delete window.bpChannels[name];
        } catch (e) {
          console.error('bpStopChannel error', name, e);
        }
      };

      async function bpTryPrimeOnce() {
        // silent 1-frame WAV
        const src = "data:audio/wav;base64,UklGRiQAAABXQVZFZm10IBAAAAABAAEAIlYAAESsAAACABAAZGF0YQAAAAA=";
        const a = new Audio(src);
        a.volume = 0.0;
        const p = a.play();
        if (p && typeof p.then === 'function') {
          await p;
        }
        try { a.pause(); } catch (e) {}
      }

      window.bpInitSound = function(cid) {
        const container = document.getElementById('bp-sound-ui-' + cid);
        if (!container) return;

        const stored = (localStorage.getItem('bp_sound_enabled') === '1');

        function renderChip(text, colorClass) {
          container.innerHTML =
            '<div class="q-chip q-chip--dense ' + colorClass + ' text-white" ' +
            'style="padding:2px 8px;display:inline-flex;align-items:center;gap:4px;">' +
            '<span>🔊</span><span>' + text + '</span></div>';
        }

        function renderButton() {
          container.innerHTML =
            '<button class="q-btn q-btn--dense bg-yellow-7 text-black" ' +
            'style="padding:2px 8px;border-radius:6px;">Enable sound</button>';
          const btn = container.querySelector('button');
          if (btn) {
            btn.addEventListener('click', async () => {
              const ok = await attemptEnable(true);
              if (!ok) {
                alert('Your browser blocked sound. Check autoplay / mute settings.');
              }
            }, { once: true });
          }
        }

        async function attemptEnable(fromUser) {
          try {
            await bpTryPrimeOnce();  // if this throws -> not allowed
          } catch (e) {
            console.warn('bpTryPrimeOnce failed', e);
            return false;
          }

          // If we got here, we actually played something -> audio allowed
          localStorage.setItem('bp_sound_enabled', '1');
          renderChip('Sound on', 'bg-green-6');

          const realCid = container.dataset.clientId || cid;
          fetch('/api/sound/enable?cid=' + encodeURIComponent(realCid), {
            method: 'POST',
          }).catch(err => console.warn('sound enable notify failed', err));

          return true;
        }

        (async () => {
          if (stored) {
            // Try silently: if blocked, fall back to button instead of lying.
            const ok = await attemptEnable(false);
            if (!ok) {
              // reset flag so next time we don't auto-assume
              localStorage.removeItem('bp_sound_enabled');
              renderButton();
            }
          } else {
            renderButton();
          }
        })();
      };
    })();
    </script>
    """)

SILENT_WAV = (
    "data:audio/wav;base64,"
    "UklGRiQAAABXQVZFZm10IBAAAAABAAEAIlYAAESsAAACABAAZGF0YQAAAAA="
)

def attach_sound_opt_in():
    """Client-side sound gate with real capability check.

    - If we THINK sound was enabled for this browser, we re-test.
    - If test passes -> mark 'Sound on', enable this client in bus.
    - If not -> show 'Enable sound' button.
    - On click, we only enable if the silent test succeeds.
    """

    client = ui.context.client

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

            print(f"[BROWSER_SOUND] auto-check failed; require click for {client.id}")

    # Hook handlers: no create_task, NiceGUI preserves context
    btn.on('click', try_enable_from_gesture)

    # Run auto-check once after mount
    ui.timer(0.2, auto_check_existing, once=True)






# Create backends for all modes
koth_backend = EnhancedGameBackend(sound_system = browser_sound_bus)

threecp_backend = ThreeCPBackend(
    sound_system=koth_backend.sound_system,
    scanner=koth_backend.scanner,
)

clock_backend = ClockBackend(
    event_manager=koth_backend.event_manager
)

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


async def get_session() -> aiohttp.ClientSession:
    global _session
    if _session is None:
        _session = aiohttp.ClientSession()
    return _session


async def close_session():
    global _session
    if _session is not None:
        await _session.close()
        _session = None


# ========================================================================
# WINNER OVERLAY HELPER (KOTH + 3CP)
# ========================================================================

def install_winner_overlay(mode: str) -> dict:
    """
    Install a full-screen winner overlay for the current client.

    'mode' is something like 'koth', '3cp', etc., so each mode
    gets its own dismissal memory.
    """
    storage = app.storage.user
    DISMISSED_KEY_NAME = f'bp_winner_dismissed_key_{mode}'

    winner_dismissed = {'flag': False}
    last_winner_key = {'key': None}

    def _team_bg(team: str) -> str:
        t = (team or '').strip().upper()
        if t == 'RED':
            return '#FF0000'
        if t == 'BLU':
            return '#0000FF'
        return 'rgba(0,0,0,0.95)'

    def _winner_text(team: str) -> str:
        t = (team or '').strip().upper()
        if t in ('RED', 'BLU'):
            return f'Winner: {t}'
        return 'Winner'

    with ui.dialog() as winner_dialog:
        with ui.element('div').style(
            'position:fixed;'
            'left:0;'
            'top:52px;'
            'width:100vw;'
            'height:calc(100vh - 52px);'
            'max-width:none;'
            'max-height:none;'
            'border-radius:0;'
            'box-shadow:none;'
            'display:flex;'
            'flex-direction:column;'
            'align-items:center;'
            'justify-content:center;'
            'background:rgba(0,0,0,0.95);'
            'color:white;'
            'z-index:1;'
        ) as winner_overlay:

            winner_label = ui.label('Winner').classes(
                'text-8xl font-extrabold tracking-wide mb-4'
            )
            winner_subtitle = ui.label('Control point secured').classes('text-3xl')

            ui.button(
                'DISMISS',
                on_click=lambda: (
                    winner_dialog.close(),
                    winner_dismissed.__setitem__('flag', True),
                    storage.__setitem__(DISMISSED_KEY_NAME, last_winner_key['key'])
                ),
            ).props('unelevated color=white text-color=black').classes(
                'text-xl px-10 py-4 rounded-xl font-bold'
            ).style(
                'position:absolute;'
                'bottom:40px;'
                'left:50%;'
                'transform:translateX(-50%);'
            )

    def open_winner(team_str: str):
        team_norm = (team_str or '').strip().upper()
        key = f"ended:{team_norm or 'NONE'}"
        stored_key = storage.get(DISMISSED_KEY_NAME, None)

        # If user already dismissed this specific winner in THIS MODE, don't re-show
        if stored_key == key:
            return

        bg = _team_bg(team_str)
        winner_overlay.style(
            'position:fixed;'
            'left:0;'
            'top:52px;'
            'width:100vw;'
            'height:calc(100vh - 52px);'
            'max-width:none;'
            'max-height:none;'
            'border-radius:0;'
            'box-shadow:none;'
            'display:flex;'
            'flex-direction:column;'
            'align-items:center;'
            'justify-content:center;'
            f'background:{bg};'
            'color:white;'
            'z-index:1;'
        )
        winner_label.set_text(_winner_text(team_str))
        winner_subtitle.set_text('Control point secured')

        last_winner_key['key'] = key
        winner_dialog.open()
        winner_dismissed['flag'] = False

    return {
        'open_winner': open_winner,
        'dialog': winner_dialog,
        'dismissed': winner_dismissed,
        'storage_key': DISMISSED_KEY_NAME,
    }


# ========================================================================
# LANDING PAGE
# ========================================================================

@ui.page('/')
def landing_page():
    ui.colors(primary='#1976D2')

    ui.add_head_html("""
    <style>
        .bp-landing {
            min-height: 100vh;
            background: linear-gradient(135deg, #1a1a2e 0%, #0f3460 100%);
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 2rem;
        }
        .bp-card {
            background: rgba(255, 255, 255, 0.05);
            backdrop-filter: blur(10px);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 20px;
            padding: 3rem;
            max-width: 900px;
            color: white;
        }
        .bp-title {
            font-size: 4rem;
            font-weight: 800;
            text-align: center;
            margin-bottom: 1rem;
            background: linear-gradient(90deg, #ff6b6b, #4ecdc4);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            background-clip: text;
        }
        .bp-subtitle {
            text-align: center;
            font-size: 1.3rem;
            color: #b0b0b0;
            margin-bottom: 3rem;
        }
        .bp-mode-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 2rem;
            margin-bottom: 2rem;
        }
        .bp-mode-card {
            background: rgba(255, 255, 255, 0.03);
            border: 2px solid rgba(255, 255, 255, 0.1);
            border-radius: 16px;
            padding: 2rem;
            cursor: pointer;
            transition: all 0.3s ease;
        }
        .bp-mode-card:hover {
            transform: translateY(-5px);
            border-color: rgba(78, 205, 196, 0.5);
            box-shadow: 0 10px 30px rgba(78, 205, 196, 0.2);
        }
        .bp-mode-title {
            font-size: 2rem;
            font-weight: 700;
            margin-bottom: 0.5rem;
        }
        .bp-mode-desc {
            color: #b0b0b0;
            line-height: 1.6;
            margin-bottom: 1rem;
        }
        .bp-info {
            background: rgba(255, 255, 255, 0.03);
            border-radius: 12px;
            padding: 1.5rem;
            margin-top: 2rem;
        }
    </style>
    """)

    with ui.element('div').classes('bp-landing'):
        with ui.element('div').classes('bp-card'):
            ui.html('<div class="bp-title">⚔️ BattlePoint</div>', sanitize=False)
            ui.html('<div class="bp-subtitle">Real-life Team Fortress 2 Combat</div>', sanitize=False)

            with ui.element('div').classes('bp-mode-grid'):
                # KOTH Card
                with ui.element('div').classes('bp-mode-card').on('click', lambda: ui.navigate.to('/koth')):
                    ui.html(
                        '<div class="bp-mode-title" style="color: #ff6b6b;">🏔️ KING OF THE HILL</div>',
                        sanitize=False,
                    )
                    ui.html(
                        '<div class="bp-mode-desc">'
                        '<strong>Classic KOTH:</strong> Control the single point to drain your team\'s timer. '
                        'First team to zero wins!'
                        '</div>',
                        sanitize=False,
                    )
                    ui.button('PLAY KOTH →', on_click=lambda: ui.navigate.to('/koth')).props(
                        'unelevated color=red-7'
                    ).classes('w-full')

                # 3CP Card
                with ui.element('div').classes('bp-mode-card').on('click', lambda: ui.navigate.to('/3cp')):
                    ui.html(
                        '<div class="bp-mode-title" style="color: #4ecdc4;">🎯 THREE CONTROL POINTS</div>',
                        sanitize=False,
                    )
                    ui.html(
                        '<div class="bp-mode-desc">'
                        '<strong>Linear Progression:</strong> Capture points in sequence (A→B→C). '
                        'Fight for the middle, push to victory!'
                        '</div>',
                        sanitize=False,
                    )
                    ui.button('PLAY 3CP →', on_click=lambda: ui.navigate.to('/3cp')).props(
                        'unelevated color=teal-6'
                    ).classes('w-full')

                # CLOCK-ONLY Card
                with ui.element('div').classes('bp-mode-card').on('click', lambda: ui.navigate.to('/clock')):
                    ui.html(
                        '<div class="bp-mode-title" style="color: #ffd166;">⏱️ CLOCK ONLY</div>',
                        sanitize=False,
                    )
                    ui.html(
                        '<div class="bp-mode-desc">'
                        'A simple round clock: set minutes and seconds, start the timer, '
                        'and let BattlePoint handle announcements and sounds.'
                        '</div>',
                        sanitize=False,
                    )
                    ui.button('RUN CLOCK →', on_click=lambda: ui.navigate.to('/clock')).props(
                        'unelevated color=amber-6'
                    ).classes('w-full')

            with ui.element('div').classes('bp-info'):
                ui.label('📋 Game Instructions').classes('text-xl font-bold mb-2')
                ui.html(
                    '''
                <ul style="color: #b0b0b0; line-height: 1.8;">
                    <li><strong>KOTH:</strong> One control point. Own it to drain your timer. Reach 0:00 to win.</li>
                    <li><strong>3CP:</strong> Three points (A-B-C). Capture in sequence to push forward and win.</li>
                    <li><strong>Clock:</strong> Simple round timer with audio cues; no control points.</li>
                    <li><strong>Setup:</strong> Use Bluetooth devices or manual controls (Debug page) to simulate players for KOTH/3CP.</li>
                    <li><strong>Settings:</strong> Configure capture times, game duration, and control square mappings.</li>
                </ul>
                ''',
                    sanitize=False,
                )

            with ui.row().classes('w-full justify-center gap-4 mt-4'):
                ui.button('⚙️ Settings', on_click=lambda: ui.navigate.to('/settings')).props('outline color=white')
                ui.button('🔧 Debug', on_click=lambda: ui.navigate.to('/debug')).props('outline color=white')


# ========================================================================
# KOTH GAME PAGE
# ========================================================================

@ui.page('/koth')
async def koth_game_ui():
    """KOTH game interface"""
    ui.colors(primary='#1976D2')

    ui.add_head_html("""
    <style>
      html, body {
        margin: 0;
        padding: 0;
        background: #000;
        height: 100%;
        overflow: hidden;
      }
      .bp-root {
        width: 100vw;
        height: 100vh;
        background: #000;
        display: flex;
        flex-direction: column;
      }
      .bp-topbar {
        height: 52px;
        background: #111;
        display: flex;
        align-items: center;
        justify-content: center;
        padding: 0 1rem;
        gap: 4rem;
      }
      .bp-topbar-left,
      .bp-topbar-center,
      .bp-topbar-right {
        display: flex;
        align-items: center;
        gap: 0.5rem;
      }
      .bp-main {
        height: calc(100vh - 52px - 170px);
        display: flex;
        flex-direction: row;
        justify-content: space-between;
        align-items: center;
        overflow: hidden;
      }
      .bp-side {
        width: 20vw;
        min-width: 200px;
        height: 100%;
        display: flex;
        flex-direction: column;
        gap: 0.75rem;
        align-items: center;
        justify-content: center;
      }
      .bp-vert-shell {
        width: 270px;
        height: 100vh;
        max-height: calc(100vh - 52px - 160px);
        background: #222;
        border: 3px solid #444;
        border-radius: 16px;
        position: relative;
        overflow: hidden;
      }
      .bp-vert-fill {
        position: absolute;
        bottom: 0;
        left: 0;
        width: 100%;
        height: 0%;
        background: #ff0000;
        transition: height 0.15s linear;
      }
      .bp-center {
        flex: 1;
        height: 100%;
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: center;
        gap: 1.0rem;
      }
      .bp-clock {
        flex: 1 1 auto;
        font-size: clamp(10rem, 40rem, 80rem);
        line-height: 1;
        font-weight: 400;
        color: #fff;
        text-align: center;
        max-height: 64%;
      }
      .bp-status-row {
        display: flex;
        flex-wrap: wrap;
        gap: 1.5rem;
        align-items: baseline;
        justify-content: center;
        color: #ddd;
        font-size: 1.2rem;
      }
      .bp-capture-shell {
        width: min(60vw, 900px);
        flex-shrink: 0;
        height: 150px;
        background: #222;
        border: 2px solid #444;
        border-radius: 12px;
        overflow: hidden;
        position: relative;
        margin: 2.5rem;
      }
      .bp-capture-fill {
        position: absolute;
        top: 0;
        left: 0;
        height: 100%;
        width: 0%;
        background: #6600ff;
        transition: width 0.12s linear;
      }
      .bp-capture-mult {
        position: absolute;
        top: 50%;
        left: 50%;
        transform: translate(-50%, -50%);
        font-size: 10rem;
        font-weight: 800;
        color: #ffffff;
        text-shadow: 0 0 12px rgba(0, 0, 0, 0.9);
        pointer-events: none;
      }
      .bp-bottom {
        height: 150px;
        display: flex;
        flex-direction: column;
        gap: 0.4rem;
        padding: 1.4rem;
      }
      .bp-horiz-shell {
        width: 100%;
        height: 100%;
        background: #000000;
        position: relative;
        overflow: hidden;
      }
      .bp-horiz-red {
        position: absolute;
        top: 0;
        left: 0;
        height: 100%;
        width: 50%;
        background: #000000;
        transition: width 0.15s linear;
      }
    </style>
    """)

    # Winner overlay for this client/page
    overlay = install_winner_overlay('koth')
    open_winner = overlay['open_winner']

    with ui.element('div').classes('bp-root'):
        # TOP BAR
        with ui.element('div').classes('bp-topbar'):
            with ui.element('div').classes('bp-topbar-left'):
                ui.button('← HOME', on_click=lambda: ui.navigate.to('/')).props('flat color=white')
                ui.label('KOTH MODE').classes('text-white text-lg font-bold')

            with ui.element('div').classes('bp-topbar-center'):
                blue_toggle = ui.toggle(['B OFF', 'B ON'], value='B OFF').props(
                    'dense push toggle-color=blue color=grey-8 text-color=white'
                )
                red_toggle = ui.toggle(['R OFF', 'R ON'], value='R OFF').props(
                    'dense push toggle-color=red color=grey-8 text-color=white'
                )
                start_btn = ui.button('START').props('color=green')
                stop_btn = ui.button('STOP').props('color=orange')
                stop_btn.set_visibility(False)

            with ui.element('div').classes('bp-topbar-right'):
                attach_sound_opt_in()
                ui.button(
                    'Settings', on_click=lambda: ui.navigate.to('/settings?mode=koth')
                ).props('flat color=white')
                ui.button(
                    'Debug', on_click=lambda: ui.navigate.to('/debug')
                ).props('flat color=white')

        # MAIN CONTENT
        with ui.element('div').classes('bp-main'):
            with ui.element('div').classes('bp-side'):
                ui.label('BLUE').classes('text-8xl font-bold text-blue-400')
                with ui.element('div').classes('bp-vert-shell'):
                    red_fill = ui.element('div').classes('bp-vert-fill')

            with ui.element('div').classes('bp-center'):
                game_clock = ui.label('0:00').classes('bp-clock')
                with ui.element('div').classes('bp-capture-shell'):
                    capture_fill = ui.element('div').classes('bp-capture-fill')
                    capture_mult_label = ui.label('').classes('bp-capture-mult')

                with ui.element('div').classes('bp-status-row'):
                    status_label = ui.label('Waiting...')
                    cp_owner = ui.label('Owner: ---')
                    cp_capturing = ui.label('Capturing: ---')
                    cp_contested = ui.label('Contested: False')

            with ui.element('div').classes('bp-side'):
                ui.label('RED').classes('text-8xl font-bold text-red-400')
                with ui.element('div').classes('bp-vert-shell'):
                    blue_fill = ui.element('div').classes('bp-vert-fill')

        # BOTTOM OWNERSHIP BAR
        with ui.element('div').classes('bp-bottom'):
            with ui.element('div').style('position: relative; width: 100%; height: 100%;'):
                with ui.element('div').classes('bp-horiz-shell'):
                    horiz_red = ui.element('div').classes('bp-horiz-red')

    # Event handlers
    async def start_game():
        s = await get_session()
        await s.post('http://localhost:8080/api/koth/start')

    async def stop_game():
        s = await get_session()
        await s.post('http://localhost:8080/api/koth/stop')

    start_btn.on('click', start_game)
    stop_btn.on('click', stop_game)

    # Manual toggles (KOTH-specific)
    syncing = {'flag': False}

    async def on_red_toggle(_e):
        if syncing['flag']:
            return
        s = await get_session()
        new_val = (red_toggle.value == 'R ON')
        await s.post(f'http://localhost:8080/api/koth/manual/red/{str(new_val).lower()}')

    async def on_blue_toggle(_e):
        if syncing['flag']:
            return
        s = await get_session()
        new_val = (blue_toggle.value == 'B ON')
        await s.post(f'http://localhost:8080/api/koth/manual/blu/{str(new_val).lower()}')

    red_toggle.on('update:model-value', on_red_toggle)
    blue_toggle.on('update:model-value', on_blue_toggle)

    # UI update loop
    async def update_ui():
        try:
            while True:
                try:
                    s = await get_session()
                    async with s.get('http://localhost:8080/api/koth/state') as resp:
                        state = await resp.json()
                except Exception:
                    await asyncio.sleep(0.3)
                    continue

                phase = state.get('phase', 'idle')
                running = state.get('running', False)

                # Update visibility
                if phase in ('running', 'countdown') or running:
                    start_btn.set_visibility(False)
                    stop_btn.set_visibility(True)
                else:
                    start_btn.set_visibility(True)
                    stop_btn.set_visibility(False)

                # Winner overlay
                winner = state.get('winner')
                if winner:
                    open_winner(winner)

                # Update clock
                if phase == 'countdown':
                    cd = state.get('countdown_remaining', 0)
                    game_clock.set_text(f'{cd}')
                    status_label.set_text('COUNTDOWN...')
                else:
                    rem = state.get('remaining_seconds', 0)
                    game_clock.set_text(f'{rem // 60}:{rem % 60:02d}')
                    if running:
                        status_label.set_text('GAME RUNNING')
                    elif state.get('winner'):
                        status_label.set_text(f"Winner: {state['winner']}")
                    else:
                        status_label.set_text('Waiting...')

                # Update meters
                meters = state.get('meters', {})

                m1 = meters.get('timer1')
                if m1:
                    red_fill.style(
                        f'height: {m1.get("percent", 0)}%; background: {m1.get("fg", "#0000FF")};'
                    )

                m2 = meters.get('timer2')
                if m2:
                    blue_fill.style(
                        f'height: {m2.get("percent", 0)}%; background: {m2.get("fg", "#FF0000")};'
                    )

                m_cap = meters.get('capture')
                if m_cap:
                    capture_fill.style(
                        f'width: {m_cap.get("percent", 0)}%; background: {m_cap.get("fg", "#6600ff")};'
                    )

                m_owner = meters.get('owner')
                if m_owner:
                    pct = m_owner.get("percent", 50)
                    fg = m_owner.get("fg") or "#FF0000"
                    if fg.lower() in ("#000000", "black"):
                        pct = 50
                        fg = "#000000"
                    horiz_red.style(f'width: {pct}%; background: {fg};')

                # Update CP info
                cp = state.get('control_point', {})
                cp_owner.set_text(f"Owner: {cp.get('owner', '---')}")
                cp_capturing.set_text(f"Capturing: {cp.get('capturing', '---')}")
                cp_contested.set_text(f"Contested: {cp.get('contested', False)}")

                mult = int(cp.get('capture_multiplier', 0) or 0)
                capture_mult_label.set_text(f"x{mult}" if mult > 0 else '')

                await asyncio.sleep(0.1)
        finally:
            await close_session()

    task = asyncio.create_task(update_ui())
    game_clock.on('disconnect', lambda _: task.cancel())


# ========================================================================
# 3CP GAME PAGE
# ========================================================================

@ui.page('/3cp')
async def threecp_game_ui():
    """3CP game interface with 3 control points"""
    ui.colors(primary='#1976D2')

    ui.add_head_html("""
    <style>
      html, body {
        margin: 0;
        padding: 0;
        background: #000;
        height: 100%;
        overflow: hidden;
      }
      .bp-root {
        width: 100vw;
        height: 100vh;
        background: #000;
        display: flex;
        flex-direction: column;
        color: #fff;
      }
      .bp-topbar {
        height: 52px;
        background: #111;
        display: flex;
        align-items: center;
        justify-content: between;
        padding: 0 1rem;
      }
      .bp-main-3cp {
        flex: 1;
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: center;
        gap: 1rem;
        padding: 1rem;
      }
      .bp-clock-3cp {
        font-size: clamp(4rem, 14vw, 16rem);
        font-weight: 400;
        color: #fff;
        text-align: center;
      }
      .bp-points-container {
        display: flex;
            gap: 10rem;
        align-items: flex-start;
        justify-content: center;
        width: 100%;
        max-width: 1400px;
      }
      .bp-cp-column {
        flex: 1;
        display: flex;
        flex-direction: column;
        align-items: center;
        gap: 1rem;
        max-width: 350px;
      }
      .bp-cp-circle {
        width: 300px;
        height: 300px;
        border-radius: 50%;
        border: 4px solid #444;
        display: flex;
        align-items: center;
        justify-content: center;
        font-size: 7rem;
        font-weight: 800;
        background: #222;
        transition: all 0.3s ease;
      }
      .bp-cp-bar {
        width: 100%;
        height: 60px;
        background: #222;
        border: 2px solid #444;
        border-radius: 8px;
        position: relative;
        overflow: hidden;
      }
      .bp-cp-bar-fill {
        position: absolute;
        top: 0;
        left: 0;
        height: 100%;
        width: 0%;
        transition: width 0.15s linear;
      }
      .bp-cp-toggles {
        display: flex;
        gap: 0.5rem;
      }
      .bp-status-3cp {
        text-align: center;
        font-size: 1.2rem;
        color: #ddd;
      }
    </style>
    """)

    # Winner overlay for this client/page
    overlay = install_winner_overlay('3cp')
    open_winner = overlay['open_winner']

    with ui.element('div').classes('bp-root'):
        with ui.element('div').classes('bp-topbar flex justify-between items-center px-4'):
            attach_sound_opt_in()
            # LEFT SIDE
            with ui.row().classes('gap-2 items-center'):
                ui.button('← HOME', on_click=lambda: ui.navigate.to('/')).props('flat color=white')
                ui.label('3CP MODE').classes('text-white text-lg font-bold')

            # RIGHT SIDE
            with ui.row().classes('gap-3 items-center'):
                start_btn = ui.button('START').props('color=green')
                stop_btn = ui.button('STOP').props('color=orange')
                stop_btn.set_visibility(False)

                ui.button('Settings', on_click=lambda: ui.navigate.to('/settings?mode=3cp')
                          ).props('flat color=white')

                ui.button('Debug', on_click=lambda: ui.navigate.to('/debug')
                          ).props('flat color=white')

        # MAIN CONTENT
        with ui.element('div').classes('bp-main-3cp'):
            game_clock = ui.label('10:00').classes('bp-clock-3cp')
            status_label = ui.label('Waiting...').classes('bp-status-3cp')

            # THREE CONTROL POINTS
            with ui.element('div').classes('bp-points-container'):
                cp_elements: list[dict[str, Any]] = []
                for i, label in enumerate(['R', 'C', 'B']):
                    with ui.element('div').classes('bp-cp-column'):
                        circle = ui.element('div').classes('bp-cp-circle')
                        circle.style('color: #fff;')
                        with circle:
                            ui.label(label)

                        with ui.element('div').classes('bp-cp-bar'):
                            bar_fill = ui.element('div').classes('bp-cp-bar-fill')

                        with ui.element('div').classes('bp-cp-toggles'):
                            red_tog = ui.toggle(['R OFF', 'R ON'], value='R OFF').props(
                                'dense push toggle-color=red color=grey-8 text-color=white'
                            )
                            blu_tog = ui.toggle(['B OFF', 'B ON'], value='B OFF').props(
                                'dense push toggle-color=blue color=grey-8 text-color=white'
                            )

                        cp_elements.append(
                            {
                                'circle': circle,
                                'bar_fill': bar_fill,
                                'red_toggle': red_tog,
                                'blu_toggle': blu_tog,
                            }
                        )

    # Event handlers
    async def start_game():
        s = await get_session()
        await s.post('http://localhost:8080/api/3cp/start')

    async def stop_game():
        s = await get_session()
        await s.post('http://localhost:8080/api/3cp/stop')

    start_btn.on('click', start_game)
    stop_btn.on('click', stop_game)

    def make_toggle_handler(cp_idx: int, team: str):
        async def handler(e):
            new_val = e.sender.value
            print(f"[DEBUG] 3CP UI toggle cp={cp_idx} team={team} new_value={new_val!r}")

            s = await get_session()

            # enable manual mode
            try:
                resp_mode = await s.post('http://localhost:8080/api/3cp/manual/mode/true')
                print(f"[DEBUG] 3CP UI toggle: manual mode ON -> {resp_mode.status}")
            except Exception as ex:
                print(f"[DEBUG] 3CP UI toggle: error enabling manual mode: {ex}")

            # toggle this CP/team state
            try:
                resp = await s.post(f'http://localhost:8080/api/3cp/manual/{cp_idx}/{team}/toggle')
                txt = await resp.text()
                print(
                    f"[DEBUG] 3CP UI toggle: POST /manual/{cp_idx}/{team}/toggle -> "
                    f"{resp.status} {txt}"
                )
            except Exception as ex:
                print(f"[DEBUG] 3CP UI toggle: error posting toggle: {ex}")

        return handler

    for i in range(3):
        cp_elements[i]['red_toggle'].on('update:model-value', make_toggle_handler(i, 'red'))
        cp_elements[i]['blu_toggle'].on('update:model-value', make_toggle_handler(i, 'blu'))

    # UI update loop
    async def update_ui():
        try:
            while True:
                try:
                    s = await get_session()
                    async with s.get('http://localhost:8080/api/3cp/state') as resp:
                        state = await resp.json()
                except Exception:
                    await asyncio.sleep(0.3)
                    continue

                phase = state.get('phase', 'idle')
                running = state.get('running', False)

                # Button visibility
                if phase in ('running', 'countdown') or running:
                    start_btn.set_visibility(False)
                    stop_btn.set_visibility(True)
                else:
                    start_btn.set_visibility(True)
                    stop_btn.set_visibility(False)

                # Winner overlay
                winner = state.get('winner')
                if winner:
                    open_winner(winner)

                # Clock
                if phase == 'countdown':
                    cd = state.get('countdown_remaining', 0)
                    game_clock.set_text(f'{cd}')
                    status_label.set_text('COUNTDOWN...')
                else:
                    rem = state.get('remaining_seconds', 0)
                    game_clock.set_text(f'{rem // 60}:{rem % 60:02d}')
                    if running:
                        status_label.set_text('GAME RUNNING')
                    elif state.get('winner'):
                        status_label.set_text(f"🏆 Winner: {state['winner']}")
                    else:
                        status_label.set_text('Waiting...')

                # Update each CP
                cps = state.get('control_points', [])
                owners = state.get('cp_owners', ['NEUTRAL', 'NEUTRAL', 'NEUTRAL'])

                for i in range(min(3, len(cps))):
                    cp_data = cps[i]
                    owner = owners[i] if i < len(owners) else 'NEUTRAL'

                    # Circle color
                    if owner == 'RED':
                        color = '#FF0000'
                    elif owner == 'BLU':
                        color = '#0000FF'
                    else:
                        color = '#444'

                    cp_elements[i]['circle'].style(f'border-color: {color}; color: {color};')

                    # Bar
                    progress = cp_data.get('progress', 0)
                    capturing = cp_data.get('capturing', '---')

                    if capturing == 'RED':
                        bar_color = '#FF0000'
                    elif capturing == 'BLU':
                        bar_color = '#0000FF'
                    else:
                        bar_color = '#666'

                    cp_elements[i]['bar_fill'].style(f'width: {progress}%; background: {bar_color};')

                await asyncio.sleep(0.1)
        finally:
            await close_session()

    task = asyncio.create_task(update_ui())
    game_clock.on('disconnect', lambda _: task.cancel())


# ========================================================================
# CLOCK-ONLY GAME PAGE
# ========================================================================

@ui.page('/clock')
async def clock_game_ui():
    """Simple clock-only game interface (no control points)"""
    ui.colors(primary='#1976D2')

    ui.add_head_html("""
    <style>
      html, body {
        margin: 0;
        padding: 0;
        background: #000;
        height: 100%;
        overflow: hidden;
      }
      .bp-root-clock {
        width: 100vw;
        height: 100vh;
        background: radial-gradient(circle at top, #222 0%, #000 60%);
        display: flex;
        flex-direction: column;
        color: #fff;
      }
      .bp-topbar-clock {
        height: 52px;
        background: #111;
        display: flex;
        align-items: center;
        justify-content: space-between;
        padding: 0 1rem;
      }
      .bp-main-clock {
        flex: 1;
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: center;
        gap: 3rem;
        padding: 2rem;
      }
      .bp-clock-face {
        font-size: clamp(18rem, 30rem, 30rem);
        font-weight: 400;
        color: #ffffff;
        text-shadow: 0 0 40px rgba(0,0,0,0.9);
      }
      .bp-controls-clock {
        display: flex;
        flex-direction: column;
        gap: 1.5rem;
        align-items: center;
        justify-content: center;
        min-width: 380px;
        padding: 1.25rem 1.25rem;
        border-radius: 16px;
        background: rgba(15,15,15,0.95);
        border: 1px solid #444;
      }
      .bp-controls-clock .q-field__control {
        min-height: 4.2rem;
      }
      .bp-controls-clock .q-field__native {
        line-height: 3rem;
        padding-top: 0.2rem;
        padding-bottom: 0.2rem;
      }
      .bp-time-input-row {
        display: flex;
        flex-direction: row;
        gap: 2rem;
        align-items: center;
        justify-content: center;
        width: 100%;
      }
      .bp-buttons-row {
        display: flex;
        flex-direction: row;
        gap: 1.5rem;
        align-items: center;
        justify-content: center;
      }
      .bp-status-clock {
        font-size: 1.6rem;
        color: #cccccc;
      }
    </style>
    """)

    with ui.element('div').classes('bp-root-clock'):
        # TOP BAR
        with ui.element('div').classes('bp-topbar-clock'):
            attach_sound_opt_in()
            with ui.row().classes('gap-2 items-center'):
                ui.button('← HOME', on_click=lambda: ui.navigate.to('/')).props('flat color=white')
                ui.label('CLOCK MODE').classes('text-white text-lg font-bold')
            ui.label('Set round time, then press START').classes('text-sm text-gray-400')

        # MAIN CONTENT
        with ui.element('div').classes('bp-main-clock'):
            # Big clock
            clock_label = ui.label('10:00').classes('bp-clock-face')

            # Time configuration + start/stop
            with ui.element('div').classes('bp-controls-clock'):
                ui.label('Round Time (mm:ss)').classes('text-2xl font-semibold')

                with ui.element('div').classes('bp-time-input-row'):
                    minutes_input = ui.input(
                        label='Minutes',
                        value='10',
                    ).props(
                        'outlined input-style="font-size: 2.8rem; text-align: center; color: white;"'
                    )
                    seconds_input = ui.input(
                        label='Seconds',
                        value='0',
                    ).props(
                        'outlined input-style="font-size: 2.8rem; text-align: center; color: white;"'
                    )

                with ui.element('div').classes('bp-buttons-row'):
                    start_btn = ui.button('START', icon='play_arrow').props('color=green')
                    stop_btn = ui.button('STOP', icon='stop').props('color=orange')
                    stop_btn.set_visibility(False)

            status_label = ui.label('Waiting...').classes('bp-status-clock')

    # Handlers
    async def start_clock():
        try:
            mins = int(minutes_input.value.strip() or '0')
        except Exception:
            mins = 0
        try:
            secs = int(seconds_input.value.strip() or '0')
        except Exception:
            secs = 0

        mins = max(0, min(99, mins))
        secs = max(0, min(59, secs))

        total_seconds = max(1, mins * 60 + secs)

        s = await get_session()
        await s.post(
            'http://localhost:8080/api/clock/configure',
            json={'time_limit_seconds': total_seconds},
        )
        await s.post('http://localhost:8080/api/clock/start')

    async def stop_clock():
        s = await get_session()
        await s.post('http://localhost:8080/api/clock/stop')

    start_btn.on('click', start_clock)
    stop_btn.on('click', stop_clock)

    initialized = {'done': False}

    async def update_ui():
        try:
            while True:
                try:
                    s = await get_session()
                    async with s.get('http://localhost:8080/api/clock/state') as resp:
                        state = await resp.json()
                except Exception:
                    await asyncio.sleep(0.3)
                    continue

                running = bool(state.get('running', False))
                phase = state.get('phase', 'idle')
                remaining = int(state.get('remaining_seconds', 0))
                total = int(state.get('time_limit_seconds', max(remaining, 1)))

                # Initialize inputs from backend once
                if not initialized['done']:
                    base = total if phase == 'idle' else remaining
                    m0, s0 = divmod(base, 60)
                    minutes_input.value = str(m0)
                    seconds_input.value = f'{s0}'
                    minutes_input.update()
                    seconds_input.update()
                    initialized['done'] = True

                # Button visibility
                if running:
                    start_btn.set_visibility(False)
                    stop_btn.set_visibility(True)
                else:
                    start_btn.set_visibility(True)
                    stop_btn.set_visibility(False)

                # Clock display
                mins = remaining // 60
                secs = remaining % 60
                clock_label.set_text(f'{mins}:{secs:02d}')

                # Status
                if phase == 'idle':
                    status_label.set_text('Waiting...')
                elif phase == 'running':
                    status_label.set_text('Clock running')
                elif phase == 'ended':
                    status_label.set_text('Time up')
                else:
                    status_label.set_text(phase)

                await asyncio.sleep(0.1)
        finally:
            await close_session()

    task = asyncio.create_task(update_ui())
    clock_label.on('disconnect', lambda _: task.cancel())


# ========================================================================
# SETTINGS PAGE
# ========================================================================

@ui.page('/settings')
async def settings_ui(mode: str = 'koth'):
    ui.colors(primary='#1976D2')

    current_mode = mode if mode in ['koth', '3cp'] else 'koth'
    selected_label = 'KOTH' if current_mode == 'koth' else '3CP'

    with ui.column().classes('w-full p-8 max-w-4xl mx-auto'):
        with ui.row().classes('w-full items-center justify-between mb-4'):
            ui.label(f'{current_mode.upper()} Settings').classes('text-3xl font-bold')
            ui.button(
                '← Back',
                on_click=lambda: ui.navigate.to(f'/{current_mode}'),
            ).classes('mb-4')

        # Mode selector
        with ui.tabs().classes('w-full') as mode_tabs:
            ui.tab('KOTH')
            ui.tab('3CP')

        mode_tabs.set_value(selected_label)

        def on_mode_change(e):
            selected = e.sender.value
            new_mode = 'koth' if selected == 'KOTH' else '3cp'
            ui.navigate.to(f'/settings?mode={new_mode}')

        mode_tabs.on('update:model-value', on_mode_change)

        with ui.tab_panels(mode_tabs, value=selected_label).classes('w-full'):
            # -------------------- KOTH Settings --------------------
            with ui.tab_panel('KOTH'):
                koth_mode_select = ui.select(
                    ['KOTH', 'AD', 'CP'],
                    label='Game Mode',
                    value='KOTH',
                ).classes('w-full')

                koth_time_limit = ui.number(
                    'Time Limit (seconds)', value=60, min=10, max=600
                ).classes('w-full')
                koth_capture_time = ui.number(
                    'Capture Time (seconds)', value=20, min=5, max=120
                ).classes('w-full')
                koth_button_threshold = ui.number(
                    'Button Threshold (seconds)', value=5, min=1, max=30
                ).classes('w-full')
                koth_start_delay = ui.number(
                    'Start Delay (seconds)', value=5, min=0, max=60
                ).classes('w-full')

                async def save_koth():
                    mode_map = {'KOTH': 0, 'AD': 1, 'CP': 2}
                    payload = {
                        'mode': mode_map[koth_mode_select.value],
                        'time_limit_seconds': int(koth_time_limit.value),
                        'capture_seconds': int(koth_capture_time.value),
                        'capture_button_threshold_seconds': int(koth_button_threshold.value),
                        'start_delay_seconds': int(koth_start_delay.value),
                    }

                    print('Saving KOTH settings with payload:', payload)

                    try:
                        async with aiohttp.ClientSession() as session:
                            async with session.post(
                                'http://localhost:8080/api/koth/configure', json=payload
                            ) as resp:
                                text = await resp.text()
                                print('KOTH /configure status:', resp.status, 'body:', text)
                                if resp.status != 200:
                                    ui.notify(
                                        f'KOTH configure failed: {resp.status}', type='negative'
                                    )
                                    return

                            async with session.post(
                                'http://localhost:8080/api/koth/settings/save'
                            ) as resp:
                                text = await resp.text()
                                print('KOTH /settings/save status:', resp.status, 'body:', text)
                                if resp.status != 200:
                                    ui.notify(f'KOTH save failed: {resp.status}', type='negative')
                                    return

                        ui.notify('KOTH settings saved!', type='positive')

                    except Exception as e:
                        print(f'Error saving KOTH settings: {e}')
                        ui.notify(f'Error saving KOTH settings: {e}', type='negative')

                ui.button(
                    'Save KOTH Settings', on_click=save_koth
                ).props('color=primary').classes('mt-4')

            # -------------------- 3CP Settings --------------------
            with ui.tab_panel('3CP'):
                threecp_time_limit = ui.number(
                    'Time Limit (seconds)', value=600, min=60, max=3600
                ).classes('w-full')
                threecp_capture_time = ui.number(
                    'Capture Time (seconds)', value=20, min=5, max=120
                ).classes('w-full')
                threecp_button_threshold = ui.number(
                    'Button Threshold (seconds)', value=5, min=1, max=30
                ).classes('w-full')
                threecp_start_delay = ui.number(
                    'Start Delay (seconds)', value=5, min=0, max=60
                ).classes('w-full')
                threecp_time_add = ui.number(
                    'Time Added Per First Capture (seconds)',
                    value=120,
                    min=0,
                    max=600,
                ).classes('w-full')

                ui.separator().classes('my-4')
                ui.label('Control Square Mappings').classes('text-xl font-bold mb-2')
                ui.label(
                    'Assign control squares (CS-XX) to each control point:'
                ).classes('text-sm text-gray-600 mb-4')

                cp1_input = ui.input(
                    label='CP-A Squares (comma-separated IDs)',
                    placeholder='1, 2',
                    value='1',
                ).classes('w-full')

                cp2_input = ui.input(
                    label='CP-B Squares (comma-separated IDs)',
                    placeholder='3, 4',
                    value='3',
                ).classes('w-full')

                cp3_input = ui.input(
                    label='CP-C Squares (comma-separated IDs)',
                    placeholder='5',
                    value='5',
                ).classes('w-full')

                async def save_3cp():
                    def parse_squares(text: str) -> list[int]:
                        out: list[int] = []
                        for token in text.split(','):
                            token = token.strip()
                            if not token:
                                continue
                            digits = ''.join(ch for ch in token if ch.isdigit())
                            if not digits:
                                continue
                            try:
                                out.append(int(digits))
                            except ValueError:
                                continue
                        return out

                    cp1_squares = parse_squares(cp1_input.value)
                    cp2_squares = parse_squares(cp2_input.value)
                    cp3_squares = parse_squares(cp3_input.value)

                    payload = {
                        'time_limit_seconds': int(threecp_time_limit.value),
                        'capture_seconds': int(threecp_capture_time.value),
                        'capture_button_threshold_seconds': int(threecp_button_threshold.value),
                        'start_delay_seconds': int(threecp_start_delay.value),
                        'add_time_per_first_capture_seconds': int(threecp_time_add.value),
                        'control_square_mapping': {
                            'cp_1_squares': cp1_squares,
                            'cp_2_squares': cp2_squares,
                            'cp_3_squares': cp3_squares,
                        },
                    }

                    print('Saving 3CP settings with payload:', payload)

                    async with aiohttp.ClientSession() as session:
                        async with session.post(
                            'http://localhost:8080/api/3cp/settings/save', json=payload
                        ) as resp:
                            text = await resp.text()
                            print('3CP /settings/save ->', resp.status, text)
                            if resp.status != 200:
                                ui.notify(f'3CP save failed: {resp.status}', type='negative')
                                return

                    ui.notify('3CP settings saved!', type='positive')

                ui.button(
                    'Save 3CP Settings', on_click=save_3cp
                ).props('color=primary').classes('mt-4')

        # Load settings on page load
        async def load_initial_settings():
            if current_mode == 'koth':
                # ---- KOTH settings load ----
                try:
                    async with aiohttp.ClientSession() as session:
                        async with session.get(
                            'http://localhost:8080/api/koth/settings/load'
                        ) as resp:
                            data = await resp.json()

                    print("[DEBUG] load_initial_settings(koth): got data =", data)

                    if data.get('status') != 'not_found':
                        # mode int -> label
                        mode_rev = {0: 'KOTH', 1: 'AD', 2: 'CP'}
                        koth_mode_select.value = mode_rev.get(data.get('mode', 0), 'KOTH')

                        koth_time_limit.value = data.get('time_limit_seconds', 60)
                        koth_capture_time.value = data.get('capture_seconds', 20)
                        koth_button_threshold.value = data.get(
                            'capture_button_threshold_seconds', 5
                        )
                        koth_start_delay.value = data.get('start_delay_seconds', 5)

                        print(
                            "[DEBUG] load_initial_settings(koth): "
                            "mode=", koth_mode_select.value,
                            "time_limit=", koth_time_limit.value,
                            "capture=", koth_capture_time.value,
                            "btn_thresh=", koth_button_threshold.value,
                            "start_delay=", koth_start_delay.value,
                        )

                        koth_mode_select.update()
                        koth_time_limit.update()
                        koth_capture_time.update()
                        koth_button_threshold.update()
                        koth_start_delay.update()

                except Exception as e:
                    print(f"[DEBUG] Error loading KOTH settings: {e}")

            else:
                # ---- 3CP settings load (unchanged) ----
                try:
                    async with aiohttp.ClientSession() as session:
                        async with session.get(
                            'http://localhost:8080/api/3cp/settings/load'
                        ) as resp:
                            data = await resp.json()

                    print("[DEBUG] load_initial_settings(3cp): got data =", data)

                    if data.get('status') != 'not_found':
                        threecp_time_limit.value = data.get('time_limit_seconds', 600)
                        threecp_capture_time.value = data.get('capture_seconds', 20)
                        threecp_button_threshold.value = data.get(
                            'capture_button_threshold_seconds', 5
                        )
                        threecp_start_delay.value = data.get(
                            'start_delay_seconds', 5
                        )
                        threecp_time_add.value = data.get(
                            'add_time_per_first_capture_seconds', 120
                        )

                        mapping = data.get('control_square_mapping', {})
                        cp1_input.value = ','.join(
                            map(str, mapping.get('cp_1_squares', [1]))
                        )
                        cp2_input.value = ','.join(
                            map(str, mapping.get('cp_2_squares', [3]))
                        )
                        cp3_input.value = ','.join(
                            map(str, mapping.get('cp_3_squares', [5]))
                        )

                        print(
                            "[DEBUG] load_initial_settings(3cp): "
                            "assigned values ->",
                            "time_limit=", threecp_time_limit.value,
                            "capture=", threecp_capture_time.value,
                            "btn_thresh=", threecp_button_threshold.value,
                            "start_delay=", threecp_start_delay.value,
                            "add_time=", threecp_time_add.value,
                            "cp1=", cp1_input.value,
                            "cp2=", cp2_input.value,
                            "cp3=", cp3_input.value,
                        )

                        threecp_time_limit.update()
                        threecp_capture_time.update()
                        threecp_button_threshold.update()
                        threecp_start_delay.update()
                        threecp_time_add.update()
                        cp1_input.update()
                        cp2_input.update()
                        cp3_input.update()
                except Exception as e:
                    print(f"[DEBUG] Error loading 3CP settings: {e}")

        await load_initial_settings()


# ========================================================================
# DEBUG PAGE (BLE + events)
# ========================================================================

@ui.page('/debug')
def debug_ui(from_page: str = '', admin: str = ''):
    ui.colors(primary='#1976D2')
    came_from_mobile = (from_page == 'mobile')

    with ui.column().classes('w-full p-8 max-w-6xl mx-auto gap-4'):
        ui.label('Debug Console').classes('text-3xl font-bold')

        def go_back():
            if came_from_mobile:
                target = f'/mobile?admin={admin}' if admin else '/mobile'
                ui.navigate.to(target)
            else:
                ui.navigate.to('/')

        ui.button('← BACK TO GAME', on_click=go_back)

        # Manual control switch
        with ui.row().classes('items-center gap-4'):
            manual_switch = ui.switch(
                'Manual Proximity Control (use Red/Blue toggles on main screen)'
            )
            manual_status = ui.label('').classes('text-sm text-gray-400')

        # BLE SCANNER CARD
        with ui.card().classes('w-full p-4'):
            with ui.row().classes('w-full justify-between items-center mb-4'):
                ui.label('🔵 Bluetooth LE Scanner').classes('text-2xl font-bold')
                with ui.row().classes('gap-2'):
                    start_scan_btn = ui.button('▶ START SCAN').props('color=green')
                    stop_scan_btn = ui.button('⏹ STOP').props('color=red')
                    stop_scan_btn.set_visibility(False)

            ble_status = ui.label('Status: Not scanning').classes('text-lg mb-2')
            device_count_label = ui.label('Tracking 0 device(s)').classes(
                'text-md text-gray-400 mb-4'
            )

            ble_columns = [
                {
                    'name': 'name',
                    'label': 'Device Name',
                    'field': 'name',
                    'align': 'left',
                    'sortable': True,
                },
                {
                    'name': 'rssi',
                    'label': 'RSSI (dBm)',
                    'field': 'rssi',
                    'align': 'center',
                    'sortable': True,
                },
                {
                    'name': 'address',
                    'label': 'Address',
                    'field': 'address',
                    'align': 'left',
                },
                {
                    'name': 'last_seen',
                    'label': 'Last Seen (ms ago)',
                    'field': 'last_seen_ms',
                    'align': 'center',
                    'sortable': True,
                },
                {
                    'name': 'mfg_data',
                    'label': 'Manufacturer Data',
                    'field': 'manufacturer_data',
                    'align': 'left',
                },
            ]
            ble_table = ui.table(columns=ble_columns, rows=[], row_key='address').classes(
                'w-full'
            )
            ble_table.add_slot(
                'body-cell-rssi',
                """
                <q-td :props="props">
                    <q-badge :color="props.value > -60 ? 'green' : props.value > -80 ? 'orange' : 'red'">
                        {{ props.value }}
                    </q-badge>
                </q-td>
            """,
            )

        # EVENT LOG CARD
        with ui.card().classes('w-full p-4'):
            with ui.row().classes('w-full items-center justify-between mb-2'):
                ui.label('📋 Event Log').classes('text-xl font-bold')
                event_count_label = ui.label('0 events').classes('text-sm text-gray-400')

            event_box = ui.column().classes(
                'w-full bg-gray-50 rounded p-2 gap-1 dark:bg-gray-900'
            ).style('height: calc(50vh - 360px); overflow-y: auto;')
            event_box_id = f"bp-event-box-{event_box.id}"
            event_box.props(f'id={event_box_id}')

        # GAME STATE CARD
        with ui.card().classes('w-full p-4'):
            ui.label('⚙️ Game State').classes('text-xl font-bold mb-2')
            state_display = ui.json_editor({}).classes('w-full')

    # remember scroll
    ui.run_javascript(
        f"""
    (function() {{
        const el = document.getElementById('{event_box_id}');
        if (!el) return;
        el.addEventListener('scroll', () => {{
            el.dataset.userScroll = (el.scrollTop > 5) ? '1' : '';
        }});
    }})();
    """
    )

    session_holder: dict[str, aiohttp.ClientSession | None] = {'session': None}
    last_events: list[str] = []

    async def _get_session():
        if session_holder['session'] is None:
            session_holder['session'] = aiohttp.ClientSession()
        return session_holder['session']

    async def _cleanup(_msg):
        if session_holder['session'] is not None:
            await session_holder['session'].close()
            session_holder['session'] = None

    event_box.on('disconnect', _cleanup)

    async def sync_manual_from_backend():
        try:
            s = await _get_session()
            async with s.get('http://localhost:8080/api/manual/state') as resp:
                st = await resp.json()
            enabled = bool(st.get('manual_control'))
            manual_switch.value = enabled
            manual_switch.update()
            manual_status.set_text(
                f"Manual control is {'ON' if enabled else 'OFF'} "
                "(BLE scanning still runs; in manual mode BLE does not change proximity)."
            )
        except Exception as e:
            print(f"[debug] manual sync failed: {e}")

    async def on_manual_change(_e):
        try:
            s = await _get_session()
            enabled = bool(manual_switch.value)
            await s.post(
                f'http://localhost:8080/api/manual/mode/{str(enabled).lower()}'
            )
            manual_status.set_text(
                f"Manual control is {'ON' if enabled else 'OFF'} "
                "(BLE scanning still runs; in manual mode BLE does not change proximity)."
            )
            ui.notify(
                f"Manual control {'enabled' if enabled else 'disabled'}", type='info'
            )
        except Exception as ex:
            print(f"[debug] manual toggle failed: {ex}")

    manual_switch.on('update:model-value', on_manual_change)

    async def start_ble_scan():
        s = await _get_session()
        await s.post('http://localhost:8080/api/bluetooth/start')
        start_scan_btn.set_visibility(False)
        stop_scan_btn.set_visibility(True)
        ble_status.set_text('Status: Scanning...')
        ble_status.classes('text-green-400', remove='text-gray-400')
        ui.notify('BLE scanning started', type='positive')

    async def stop_ble_scan():
        s = await _get_session()
        await s.post('http://localhost:8080/api/bluetooth/stop')
        start_scan_btn.set_visibility(True)
        stop_scan_btn.set_visibility(False)
        ble_status.set_text('Status: Not scanning')
        ble_status.classes('text-gray-400', remove='text-green-400')
        ui.notify('BLE scanning stopped', type='info')

    start_scan_btn.on('click', start_ble_scan)
    stop_scan_btn.on('click', stop_ble_scan)

    def _set_scan_buttons_enabled(enabled: bool):
        if enabled:
            start_scan_btn.enable()
            stop_scan_btn.enable()
        else:
            start_scan_btn.disable()
            stop_scan_btn.disable()

    async def update_debug_once():
        if state_display.client is None:
            return

        s = await _get_session()

        try:
            async with s.get('http://localhost:8080/api/koth/state') as resp:
                state = await resp.json()
                state_display.value = state
                state_display.update()

                game_running = bool(state.get('running', False))
                _set_scan_buttons_enabled(not game_running)

                events = state.get('events', [])
                event_count_label.set_text(f'{len(events)} event(s)')

                if events != last_events:
                    event_box.clear()
                    with event_box:
                        for line in reversed(events):
                            ui.label(line).classes(
                                'text-sm text-gray-900 dark:text-gray-100'
                            )
                    last_events[:] = events

                    await ui.run_javascript(
                        f"""
                    (function(){{
                        const el = document.getElementById('{event_box_id}');
                        if (!el) return;
                        if (!el.dataset.userScroll) {{
                            el.scrollTop = 0;
                        }}
                    }})();
                    """
                    )

            await sync_manual_from_backend()

            async with s.get('http://localhost:8080/api/bluetooth/devices') as resp:
                ble_data = await resp.json()

                if ble_data.get('scanning'):
                    ble_status.set_text('Status: Scanning...')
                    ble_status.classes('text-green-400', remove='text-gray-400')
                    start_scan_btn.set_visibility(False)
                    stop_scan_btn.set_visibility(True)
                else:
                    ble_status.set_text('Status: Not scanning')
                    ble_status.classes('text-gray-400', remove='text-green-400')
                    start_scan_btn.set_visibility(True)
                    stop_scan_btn.set_visibility(False)

                device_count = ble_data.get('device_count', 0)
                device_count_label.set_text(f'Tracking {device_count} device(s)')

                devices = ble_data.get('devices', [])
                ble_table.rows = devices
                ble_table.update()

        except Exception as e:
            print(f"Debug update error: {e}")

    ui.timer(0.25, update_debug_once, once=False)


# ========================================================================
# API ENDPOINTS - ALIASES / MANUAL / BLE
# ========================================================================

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


# ========================================================================
# API ENDPOINTS - KOTH
# ========================================================================

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


@app.post("/api/3cp/manual/{cp_index}/{team}/toggle")
async def threecp_manual_toggle(cp_index: int, team: str):
    if 0 <= cp_index < 3 and team in ['red', 'blu']:
        current = threecp_backend.manual_states[cp_index][team]
        threecp_backend.set_manual_state(cp_index, **{team: not current})
    return threecp_backend.get_manual_state()


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

async def koth_game_loop():
    while True:
        koth_backend.update()
        await asyncio.sleep(0.1)


async def threecp_game_loop():
    while True:
        threecp_backend.update()
        await asyncio.sleep(0.1)


async def clock_game_loop():
    while True:
        clock_backend.update()
        await asyncio.sleep(0.1)


async def start_game_loops():
    asyncio.create_task(koth_game_loop())
    asyncio.create_task(threecp_game_loop())
    asyncio.create_task(clock_game_loop())


app.on_startup(start_game_loops)


# ========================================================================
# RUN SERVER
# ========================================================================

if __name__ in {"__main__", "__mp_main__"}:
    ui.run(
        title='BattlePoint - Multi-Mode',
        port=8080,
        storage_secret='battlepoint_secret',
        reload=False,
    )
