#!/bin/sh

#
# Pull /sdcard/test-data from all devices
#

for device in `adb devices | grep device | grep -v List | cut -f1 | tr '\n' ' '`;
do
	echo "[+] Downloading from $device"
	mkdir $device 2>/dev/null || echo
	cd $device && adb -s $device pull /sdcard/test-data && cd ..
	echo
done
