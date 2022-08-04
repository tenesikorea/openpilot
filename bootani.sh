#!/usr/bin/bash

export LD_LIBRARY_PATH=/data/data/com.termux/files/usr/lib
export HOME=/data/data/com.termux/files/home
export PATH=/usr/local/bin:/data/data/com.termux/files/usr/bin:/data/data/com.termux/files/usr/sbin:/data/data/com.termux/files/usr/bin/applets:/bin:/sbin:/vendor/bin:/system/sbin:/system/bin:/system/xbin:/data/data/com.termux/files/usr/bin/python
export PYTHONPATH=/data/openpilot

mount -o remount,rw /system
cp -rf /data/openpilot/bootani/userbootanimation.zip /system/media/bootanimation.zip
sleep 3
chmod 644 /system/media/bootanimation.zip
mount -o remount,r /system