#!/system/bin/sh

#
# Helper script to block an IP with the firewall
# to test multi-hop functionality.
# Must be run as root.
#

if [ "$1" = "" ]; then
	echo "Usage: $0 <ip> [<ip> ...]"
	exit 1;
fi

for ip in $@; 
do
	echo "[+] Blocking $ip";
	iptables -A INPUT -s $ip -j DROP;
	iptables -A OUTPUT -d $ip -j DROP;
done


