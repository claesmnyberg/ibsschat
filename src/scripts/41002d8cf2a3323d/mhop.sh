#!/system/bin/sh

#
# Simulate multi-hop for this device in a line of six devices
#

blockip()
{

	echo "[+] Blocking $1";
	iptables -A INPUT -s $1 -j DROP;
	iptables -A OUTPUT -d $1 -j DROP;

}

blockip 10.0.0.1 
blockip 10.0.0.2 
blockip 10.0.0.3 
blockip 10.0.0.4
