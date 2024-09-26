#!/system/bin/sh

#
# Helper script to start the IBSS chat service
# Because it is combuersome to write in the
# terminal ...
#

if [ "$1" = "" ]; then
	echo "Usage: $0 <ip>"
	exit 1;
fi

echo "[+] Attempting to kill existing process"
pkill ibsschat 
sleep 1
echo "[+] Starting daemon"
ibsschat --daemon wlan0
echo "[+] Configuring interface"
sleep 5
ibsschat --conf wlan0 $1 255.255.255.0 myadhoc 11 cryptokey
echo "[+] Done"
