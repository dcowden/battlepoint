from nicegui import ui

def control_point(owner_color='#888888', progress=0.4):
    circumference = 2 * 3.14159 * 45  # r=45
    offset = circumference * (1 - progress)

    html = f'''
    <svg width="120" height="120" viewBox="0 0 120 120">
        <!-- Outer background ring -->
        <circle cx="60" cy="60" r="45" stroke="#333" stroke-width="10" fill="none"/>

        <!-- Capture progress ring -->
        <circle cx="60" cy="60" r="45"
            stroke="{owner_color}"
            stroke-width="10"
            fill="none"
            stroke-dasharray="{circumference}"
            stroke-dashoffset="{offset}"
            stroke-linecap="round"
            transform="rotate(-90 60 60)"
        />

        <!-- Owner fill -->
        <circle cx="60" cy="60" r="32" fill="{owner_color}" />
    </svg>
    '''
    return ui.html(html,sanitize=False)

with ui.row():
    control_point('#3b82f6', 0.25)  # Blue, 25%
    control_point('#ef4444', 0.75)  # Red, 75%

ui.run()
