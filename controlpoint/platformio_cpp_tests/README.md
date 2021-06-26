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
 * The bezel needs to cover the non-interactive part of the OLED display

## Programming
 * make # of lights on each strand configurable
 * change defaults to 20 lights on each strand
 * remove debug
 * in cp mode, we have captured the control point shouldnt play, since victory is immediate
 * in KOTH, the countdown gets behind the numbers. better to just play the countdown as one recording
 * record new sounds

 # Enhancements

 ## Easier to make and higher mast
 The led strips should be in a mast that extends upwards, making it easier to make and better visibilty too

 ## Three Player Mode
Three player mode would require another LED strip and another button. The rules are as follows:

in **KOTH** each color tracks separately. the first time to run their timer out wins
in **AD** mode  two teams are attacking and one is defending. But the winning team is the one which accumulates the most time towards the capture.
in **CP** mode the point tracks all three colors separately


 
