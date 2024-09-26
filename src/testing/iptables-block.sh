
# Run on machine 10.0.0.1
iptables -A INPUT -s 10.0.0.3 -j DROP
iptables -A OUTPUT -d 10.0.0.3 -j DROP

# Run on machine 10.0.0.3
iptables -A INPUT -s 10.0.0.1 -j DROP
iptables -A OUTPUT -d 10.0.0.1 -j DROP
