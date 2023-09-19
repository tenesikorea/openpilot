#!/usr/bin/bash

#cd /data/openpilot/selfdrive/debug;pkill -f boardd
cd /data/openpilot/selfdrive/assets/dtc;pkill -f boardd
sleep 1s

#./clear_dtc.py
./dhmdps_clear_dtc.py
sleep 5s
