#!/system/bin/sh

#
# Helper script to configure the IBSS chat service
# Because it is combuersome to write in the
# terminal ...
#

if [ "$1" = "" ]; then
	echo "Usage: $0 <ip>"
	exit 1;
fi

echo "[+] Configuring interface"
ibsschat --conf wlan0 $1 255.255.255.0 myadhoc 11 cryptokey
