from nicegui import ui


from nicegui import ui


def install_winner_overlay(mode: str) -> dict:
    """
    Install a full-screen winner overlay for the current client.
    Now *dumb*: it always shows when open_winner() is called.
    The per-game suppression is handled in the page's update loop.

    Extended: optionally shows elapsed game time (for competitive AD, etc.).
    """
    winner_dismissed = {'flag': False}
    last_winner_key = {'key': None}  # kept in case you're using it elsewhere

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

    def _format_elapsed(seconds: float | int | None) -> str:
        """Return a simple M:SS string from elapsed seconds."""
        if seconds is None:
            return ''
        try:
            total = int(seconds)
        except (TypeError, ValueError):
            return ''
        if total < 0:
            total = 0
        m = total // 60
        s = total % 60
        return f'{m}:{s:02d}'

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

            # NEW: elapsed time label (hidden unless we have a value)
            elapsed_label = ui.label('').classes(
                'text-8xl mt-6'
            )
            elapsed_label.visible = False

            ui.button(
                'DISMISS',
                on_click=lambda: (
                    winner_dialog.close(),
                    winner_dismissed.__setitem__('flag', True),
                ),
            ).props('unelevated color=white text-color=black').classes(
                'text-xl px-10 py-4 rounded-xl font-bold'
            ).style(
                'position:absolute;'
                'bottom:40px;'
                'left:50%;'
                'transform:translateX(-50%);'
            )

    def open_winner(team_str: str, elapsed_seconds: float | int | None = None):
        """
        Show the winner overlay.

        :param team_str: 'RED', 'BLU', or anything else for neutral.
        :param elapsed_seconds: elapsed game time in seconds (optional).
                                If provided, it will be shown on the overlay
                                as M:SS for competitive AD timing.
        """
        team_norm = (team_str or '').strip().upper()
        key = f"ended:{team_norm or 'NONE'}"
        last_winner_key['key'] = key

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

        # NEW: handle elapsed time display
        formatted = _format_elapsed(elapsed_seconds)
        if formatted:
            elapsed_label.set_text(f'Elapsed time: {formatted}')
            elapsed_label.visible = True
        else:
            elapsed_label.visible = False

        winner_dialog.open()
        winner_dismissed['flag'] = False

    return {
        'open_winner': open_winner,
        'dialog': winner_dialog,
        'dismissed': winner_dismissed,
    }
