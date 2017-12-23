# BattlePoint

A real-life capture the flag controller


# TODO

## Electrical
 * Shield cannot extend over UNO USB block
 * Owner and Capture LED strands reversed
 * More room 
 * more room for heat sink ( was hitting stuff around it)
 * add connx2 for switch 
 * add 2x 1000uf caps near LED strips
 * BUG: vmonitor connects to vin of regulator, not vin of arduino
 * BUG: not enough room for terminal block
 * Change terminal block footprint to conn JST XH
 * BUG: audio: chnage to JSTx05, add ground pin
 * Button board: latch has to go inwards
 * switch to microusb breakout and USB for lights, and TYPEA USB for each led strand
 * merge 620 and 330 ohm (R4 and R5) to 1x 1k instead
 * 

## Mechanical
 * More room between OLED board and Button board ( they overlap now)
 * Bigger Channels for lights. Maybe just keep the back partially open
 * more room around buttons. they tend to get tangled with wires

## Programming
 * make # of lights on each strand configurable
 * change defaults to 20 lights on each strand
 * remove debug
 
