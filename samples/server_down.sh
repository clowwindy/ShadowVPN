#!/bin/sh

# This script will be executed when server is down. All key value pairs (except
# password) in ShadowVPN config file will be passed to this script as
# environment variables.

# Turn off IP forwarding
#sysctl -w net.ipv4.ip_forward=0

# Turn off NAT over VPN
iptables -t nat -D POSTROUTING -s $net ! -d $net -m comment --comment "shadowvpn" -j MASQUERADE
iptables -D FORWARD -s $net -m state --state RELATED,ESTABLISHED -j ACCEPT
iptables -D FORWARD -d $net -j ACCEPT

# Turn off MSS fix (MSS = MTU - TCP header - IP header)
iptables -t mangle -D FORWARD -p tcp -m tcp --tcp-flags SYN,RST SYN -j TCPMSS --clamp-mss-to-pmtu

echo $0 done
