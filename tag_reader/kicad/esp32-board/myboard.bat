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
"C:\Program Files\KiCad\8.0\bin\kicad-cli" pcb export svg --output c:\temp\test.svg --theme "KiCad Classic" --layers User.Comments,B.Cu,Edge.Cuts --exclude-drawing-sheet c:\users\davec\gitwork\battlepoint\tag_reader\kicad\esp32-board\esp32-board.kicad_pcb

REM used to make masks
"C:\Program Files\KiCad\8.0\bin\kicad-cli" pcb export svg --output c:\temp\mask.svg --theme "KiCad Classic" --layers User.Comments,B.Mask,Edge.Cuts --exclude-drawing-sheet c:\users\davec\gitwork\battlepoint\tag_reader\kicad\esp32-board\esp32-board.kicad_pcb

REM this is to cut holes and edges
"C:\Program Files\KiCad\8.0\bin\kicad-cli" pcb export dxf --output c:\temp\cuts.dxf --layers Edge.Cuts  --output-units mm c:\users\davec\gitwork\battlepoint\tag_reader\kicad\esp32-board\esp32-board.kicad_pcb


REM this is a high res version, with red background.In this image:
REM true background is red
REM board isolation traces are blue
REM drill holes are WHITE
REM grnd copper fills (B.Cu) are Green
magick convert -background red -density 1200 c:\temp\test.svg  c:\temp\step1.png

REM color drill holes -- black
magick c:\temp\step1.png  -fill black -opaque white c:\temp\step2.png
magick c:\temp\step2.png  -fill white +opaque black c:\temp\drills.png


REM make isolation routes
magick c:\temp\step1.png -fuzz 40%% -fill black -opaque blue  c:\temp\step5.png
magick c:\temp\step5.png -fill white +opaque black  c:\temp\isolate.png

REM make masks
magick -background red -density 1200 c:\temp\mask.svg  c:\temp\step6.png
magick c:\temp\step6.png -fuzz 40%% -fill white -opaque blue  c:\temp\step7.png
magick c:\temp\step7.png -fuzz 40%% -fill white -opaque red  c:\temp\step8.png
magick c:\temp\step8.png -fill black +opaque white  c:\temp\step9.png
magick c:\temp\step9.png -transparent white  c:\temp\masks.png
