#!/usr/bin/bash

cd /data/openpilot
git pull
sleep 3s

if [ -f "/data/openpilot/prebuilt" ]; then
  pkill -f thermald
  rm -f /data/openpilot/prebuilt
fi

sleep 2s
