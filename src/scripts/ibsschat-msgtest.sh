#!/system/bin/sh

#
# Send messages with random strings
#

if [ "$1" = "" ]; then
	echo "Usage: $0 <count> <delay>"
	exit 1;
fi

ibsschat --chat-send-rand wlan0 $1 $2
