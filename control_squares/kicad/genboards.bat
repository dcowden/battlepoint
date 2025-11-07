REM =============================
REM Overall Steps -- laser processing a 2 sided board
REM ================================
REM 1. Draw the board in kicad
REM 2. get pre-cut two sided copper boards, measure size, and paint both sides
REM 3. run this script, which generates:
REM      * dxf for cuts
REM      * front and back isolate images
REM      * front and back mask images
REM 4. import above into lightburn.
REM    4.1 use edge.cuts board outline to cut a holder square-- HOLDER layer
REM    4.2 position back and back mask at upper right corner of board area
REM    4.3 positoin front and front mask at upper LEFT corner of board area
REM    4.4 settings: for cuts, 40mm/sec, 50% power. for images, 200dpi, 300mm/s, 40% power
REM 5. Do laser processing
REM    5.1 mount cardboard
REM    5.2. cut holder
REM    5.3 place board in holder
REM    5.4 run back isolation job
REM    5.5 flip board horizontally
REM    5.6 run front isolation job
REM    5.7 REMOVE board and etch. (note at this time the board is not its final shape)
REM    5.8 place board in holder with back side first
REM    5.9 run back mask job, and optionally back skilscreen job
REM    5.10 flip board horizontally
REM    5.11 run front mask job and optionally front silkscreen job
REM    5.12 run user.drawings cuts to cut the holes and the final board outline
REM    

REM Tricks
REM  * The board is initially painted, and is larger than the target board size.
REM  * We want to avoid ablating paint that's outside of our target board,
REM  * we can generally solve this by making a whole-board ground plane.
REM  * BUT filled zones only go as far as the edge.cuts!
REM  * The workaround is to DO NOT use an outer edge.cuts border--instead draw the board outline on User.Drawings, draw copper fills over whole board,
REM       then export both User.Drawings and Edge.Cuts for making the cuts dxf.
REM       This produces a board that will ablate only the paint we need in the board area, and leave the paint on the unused board portion.
REM  * when importing the dxf, just remove the outside edge.cuts border
REM  * NOTE: you have to export edge.cuts because that's how you get your holes for THT components.
REM  * NOTE2: make sure to make ruled zones to prevent copper in the areas for the silk screen
REM  * NOTE3: make sure to remove copper along the edges where the board cuts go. otherwise, cant be laser cut
REM Tricks for making sure we have a silkscreen/etch
REM to mark silkscreen geometry, we need bare board where it will go.
REM 1. dont run traces over were the silkscreen area
REM 2. make a keep out area for the ground plan fill
REM 3. you typically only want one side silkscreen

REM layout
REM The page/board size needs to be the same size as the raw boards
REM put the target board in one corner of the thing total board size
REM we'll cut a holder before running the programs, so that we can flip horizontally

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

set PROG="C:\Program Files\KiCad\9.0\bin\kicad-cli"
set SOURCE_FILE="%1"
set OUT_FILE=".\board_art"


REM this is to cut holes and edges
%PROG% pcb export dxf --output %OUT_FILE%\cuts.dxf --layers Edge.Cuts,User.Drawings  --output-units mm %SOURCE_FILE%

REM =========================
REM FRONT SIDE
REM ==========================
%PROG% pcb export svg --output %OUT_FILE%\back.svg --negative --black-and-white --theme "KiCad Classic" --layers B.Cu,Edge.Cuts --exclude-drawing-sheet %SOURCE_FILE%
magick convert -density 1200 %OUT_FILE%\back.svg  %OUT_FILE%\back.png
%PROG% pcb export svg --output %OUT_FILE%\mask-back.svg --black-and-white --theme "KiCad Classic" --layers B.Mask,Edge.Cuts --exclude-drawing-sheet %SOURCE_FILE%
magick convert -density 1200 %OUT_FILE%\mask-back.svg  %OUT_FILE%\mask-back.png
%PROG% pcb export dxf --output %OUT_FILE%\back-silkscreen.dxf --layers B.Silkscreen,Edge.Cuts  --output-units mm %SOURCE_FILE%

REM isolation routes for front
REM not mirrored because the board will flip over during processing
%PROG% pcb export svg --output %OUT_FILE%\front.svg --negative --mirror --black-and-white --theme "KiCad Classic" --layers F.Cu,Edge.Cuts --exclude-drawing-sheet %SOURCE_FILE%
magick convert -density 1200 %OUT_FILE%\front.svg  %OUT_FILE%\front.png
%PROG% pcb export svg --output %OUT_FILE%\mask-front.svg --black-and-white --mirror --theme "KiCad Classic" --layers F.Mask,Edge.Cuts --exclude-drawing-sheet %SOURCE_FILE%
magick convert -density 1200 %OUT_FILE%\mask-front.svg  %OUT_FILE%\mask-front.png
%PROG% pcb export dxf --output %OUT_FILE%\front-silkscreen.dxf --layers F.Silkscreen,Edge.Cuts  --output-units mm %SOURCE_FILE%


