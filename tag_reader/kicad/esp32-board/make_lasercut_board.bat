REM Generate Initial SVG
REM this has white BG, white drill holes, and a 
REM dark blue background for the board itself
REM that was user-provided in User.Comments layer
REM also assumes a green copper pour ( B.Cu)

REM Things you have to do first in KiCAD
REM 1. set page size to be the same size as the board.
REM    REMEMBER this for when you scale on LightBurn!
REM 2. draw a User.Comments object that covers the ACTUAL board 
REM 3. draw on the B.Cu layer

REM Running this script:
REM after running, import these files into LightBurn:
REM we will run these in this order:
REM
REM File         Power
REM -----------------------------
REM isolate.png  Run at power to  burn off paint
REM ETCH
REM mask.png     Run this at power to burn off more paint
REM edges.png    Run this at cutting power to cut edge and holes

REM this is used to make isolation routes
"C:\Program Files\KiCad\8.0\bin\kicad-cli" pcb export svg --output step0.svg --theme "KiCad Classic" --layers User.Comments,B.Cu,Edge.Cuts --exclude-drawing-sheet c:\users\davec\gitwork\battlepoint\tag_reader\kicad\esp32-board\esp32-board.kicad_pcb

REM used to make masks
"C:\Program Files\KiCad\8.0\bin\kicad-cli" pcb export svg --output mask.svg --theme "KiCad Classic" --layers User.Comments,B.Mask,Edge.Cuts --exclude-drawing-sheet c:\users\davec\gitwork\battlepoint\tag_reader\kicad\esp32-board\esp32-board.kicad_pcb

REM this is to cut holes and edges
"C:\Program Files\KiCad\8.0\bin\kicad-cli" pcb export dxf --output cuts.dxf --layers Edge.Cuts  --output-units mm c:\users\davec\gitwork\battlepoint\tag_reader\kicad\esp32-board\esp32-board.kicad_pcb

REM make front side silkscreen
"C:\Program Files\KiCad\8.0\bin\kicad-cli" pcb export dxf --output silkscreen.dxf --layers Edge.Cuts,F.Silkscreen,F.Courtyard --output-units mm c:\users\davec\gitwork\battlepoint\tag_reader\kicad\esp32-board\esp32-board.kicad_pcb


REM this is a high res version, with red background.In this image:
REM true background is red
REM board isolation traces are blue
REM drill holes are WHITE
REM grnd copper fills (B.Cu) are Green
magick convert -background red -density 1200 step0.svg  step1.png

REM color drill holes -- black
magick step1.png  -fill black -opaque white step2.png
magick step2.png  -fill white +opaque black drills.png


REM make isolation routes
REM flip horizontal since its on the back
magick step1.png -fuzz 40%% -fill black -opaque blue  step5.png
magick step5.png -fill black -opaque white step5a.png
magick step5a.png -fill white +opaque black  step5b.png
magick step5b.png -flop isolate.png

REM make masks
magick -background red -density 1200 mask.svg  step6.png
magick step6.png -fuzz 40%% -fill white -opaque blue  step7.png
magick step7.png -fuzz 40%% -fill white -opaque red  step8.png
magick step8.png -fill black +opaque white  step9.png
magick step9.png -transparent white  step10.png
magick step10.png -flop masks.png

REM get rid of intermiediate ones
REM comment out to troubleshoot
erase step0.png
erase step1.png
erase step2.png
erase step3.png
erase step5.png
erase step5a.png
erase step5b.png
erase step6.png
erase step7.png
erase step8.png
erase step9.png
erase step10.png

