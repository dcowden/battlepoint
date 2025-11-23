#!/bin/bash

source  /home/orangepi/battlepoint/venv/bin/activate
cd /home/orangepi/battlepoint/control_squares/base_station || exit 1
exec python battlepoint_app_multimode.py > /home/orangepi/battlepoint/bp.log 2>&1