#!/bin/sh

# This script will be executed when server is down. All key value pairs (except
# password) in ShadowVPN config file will be passed to this script as
# environment variables.

# Turn off IP forwarding
#sysctl -w net.ipv4.ip_forward=0

# Get default gateway interface
gw_intf=$(ip route show 0/0 | awk '{print $5}')

# Turn off NAT over default gateway interface and VPN
iptables -t nat -D POSTROUTING -o $gw_intf -m comment --comment "$gw_intf (shadowvpn)" -j MASQUERADE
iptables -D FORWARD -i $gw_intf -o $intf -m state --state RELATED,ESTABLISHED -j ACCEPT
iptables -D FORWARD -i $intf -o $gw_intf -j ACCEPT

# Turn off MSS fix (MSS = MTU - TCP header - IP header)
iptables -t mangle -D FORWARD -p tcp -m tcp --tcp-flags SYN,RST SYN -j TCPMSS --clamp-mss-to-pmtu

echo $0 done
