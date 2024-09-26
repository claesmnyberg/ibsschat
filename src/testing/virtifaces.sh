#!/bin/bash

#
# Create virtual interfaces on Linux for testing
#

IFACE="enp10s0"

ifconfig ${IFACE}:1 10.10.0.101 netmask 255.255.255.0
ifconfig ${IFACE}:2 10.10.0.102 netmask 255.255.255.0
ifconfig ${IFACE}:3 10.10.0.103 netmask 255.255.255.0
ifconfig ${IFACE}:4 10.10.0.104 netmask 255.255.255.0
ifconfig ${IFACE}:5 10.10.0.105 netmask 255.255.255.0
ifconfig ${IFACE}:6 10.10.0.106 netmask 255.255.255.0
