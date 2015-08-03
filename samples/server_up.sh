#!/bin/sh

# This script will be executed when server is up. All key value pairs (except
# password) in ShadowVPN config file will be passed to this script as
# environment variables.

# Turn on IP forwarding
sysctl -w net.ipv4.ip_forward=1

# Configure IP address and MTU of VPN interface
ip addr add $net dev $intf
ip link set $intf mtu $mtu
ip link set $intf up

# Get default gateway interface
gw_intf=$(ip route show 0/0 | awk '{print $3}')

# turn on NAT over gw_intf and VPN
if !(iptables-save -t nat | grep -q "$gw_intf (shadowvpn)"); then
  iptables -t nat -A POSTROUTING -o $gw_intf -m comment --comment "$gw_intf (shadowvpn)" -j MASQUERADE
fi
iptables -A FORWARD -i $gw_intf -o $intf -m state --state RELATED,ESTABLISHED -j ACCEPT
iptables -A FORWARD -i $intf -o $gw_intf -j ACCEPT

# Turn on MSS fix (MSS = MTU - TCP header - IP header)
iptables -t mangle -A FORWARD -p tcp -m tcp --tcp-flags SYN,RST SYN -j TCPMSS --clamp-mss-to-pmtu

echo $0 done
