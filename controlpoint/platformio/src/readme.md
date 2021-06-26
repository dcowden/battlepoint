# BattlePoint KOTH Station

The koth station allows for KOTH games in laser tag, nerf, or airsoft.

The station behaves similar to a capture point in TeamFortressII.

When power is reset, a game starts.  

Time is managed in increments of 1 second. During a given second, a team owns
the point if the teams' corresponding button was pressed in that interval.

## Visualizations -- KOTH mode

**Victory progress** each time has a colored strip of LEDs

**capture progress** a small meter shows progress towards capture

**occupation** a small light above each button shows whether a team
will receive occupation credit for a given cycle

**victory** the winning team's progress bar flashes when victory is achieved

**overtime** the capture progress meter flashes when the game is in overtime

## Visualizations -- CP mode

Visualziations are the same as in KOTH mode, except:

  * **victory progress** begins with the point owned 100% by one team, and
  is then captured by the other team.  


## Operation Modes

#### standalone mode
 the station operates as a KOTH point, using default game parameters:
  * 3 minutes to win
  * 5 second occupaton interval
  * 15 seconds to capture
  * team colors red and blue

#### network mode
the station receives instructions from a network
server, which provides game parameters.

## Electronics

 * arduino or similar, with wifi network support
 * 2 x LED light strips, for score meters
 * 1 x LED light strip, for capture meters
 * 1 x pc speaker
 * 1 x lipo battery
 * 2 x micro limit switches
 * 1 x power switch ( also acts as reset button)

## Hardware
 * case
 * 3x clear plastic tubes for light meters
 * 1 x battery holder
 * 2 x button assembly ( red and blue)

## Future improvement-- automatic proximity

Using DW1000 units, the need to press a button can be eliminated if players
carry a device that can be used to determine if players are close enough.

It is also **possible** that wifi signal strenght and cheap esp8266 chips
could fill this role

## Future improvement -- network game server

Each point can communicate with a network server, which can provide realtime game status
via web browsers on players' phones/devices.

The server can also be used to set game parameters, and set up CP games taht involve
more than one capture point  
