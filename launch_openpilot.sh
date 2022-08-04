#!/usr/bin/bash

if [ ! -f "/system/media/ani.txt" ]; then
    sleep 3
    mount -o remount,rw /system
    cp -rf /data/openpilot/bootani/tbootanimation.zip /system/media/bootanimation.zip
    cp -rf /data/openpilot/bootani/ani.txt /system/media/ani.txt
    chmod 644 /system/media/bootanimation.zip
    mount -o remount,r /system
    reboot
fi

export PASSIVE="0"
exec ./launch_chffrplus.sh

