AUDIO_JS = """
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
    """

ROOT_HEAD_HTML="""
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
            max-width: 1800px;
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
            grid-template-columns: 1fr 1fr 1fr 1fr;
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
    """

KOTH_HEAD_HTML="""
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
    """
THREECP_HEAD_HTML="""
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
    """
CLOCK_HEAD_HTML="""
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
    """