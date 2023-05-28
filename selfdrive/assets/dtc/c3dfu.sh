#!/usr/bin/bash

cd /data/openpilot/panda/board
lsusb
sleep 1s
echo 124 > /sys/class/gpio/export
echo "out" > /sys/class/gpio/gpio124/direction
echo 1 > /sys/class/gpio/gpio124/value

echo 134 > /sys/class/gpio/export
echo "out" > /sys/class/gpio/gpio134/direction
echo 1 > /sys/class/gpio/gpio134/value

echo 124 > /sys/class/gpio/export
echo "out" > /sys/class/gpio/gpio124/direction
echo 0 > /sys/class/gpio/gpio124/value
sleep 2s
lsusb
sleep 2s
./recover.sh
sleep 1s
echo 124 > /sys/class/gpio/export
echo "out" > /sys/class/gpio/gpio124/direction
echo 1 > /sys/class/gpio/gpio124/value

echo 134 > /sys/class/gpio/export
echo "out" > /sys/class/gpio/gpio134/direction
echo 0 > /sys/class/gpio/gpio134/value

echo 124 > /sys/class/gpio/export
echo "out" > /sys/class/gpio/gpio124/direction
echo 0 > /sys/class/gpio/gpio124/value