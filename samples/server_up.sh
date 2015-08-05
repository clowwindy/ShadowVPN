#!/bin/sh

# This script will be executed when server is up. All key value pairs (except
# password) in ShadowVPN config file will be passed to this script as
# environment variables.

# Turn on IP forwarding
sysctl -w net.ipv4.ip_forward=1

# Configure IP address and MTU of VPN interface
ip addr add $net dev $intf
ip link set $intf mtu $mtu

# turn on NAT over VPN
if !(iptables-save -t nat | grep -q "shadowvpn"); then
  iptables -t nat -A POSTROUTING -s $net ! -d $net -m comment --comment "shadowvpn" -j MASQUERADE
fi
iptables -A FORWARD -s $net -m state --state RELATED,ESTABLISHED -j ACCEPT
iptables -A FORWARD -d $net -j ACCEPT

# Turn on MSS fix (MSS = MTU - TCP header - IP header)
iptables -t mangle -A FORWARD -p tcp -m tcp --tcp-flags SYN,RST SYN -j TCPMSS --clamp-mss-to-pmtu

echo $0 done
