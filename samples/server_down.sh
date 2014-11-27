#!/bin/sh

# example server down script
# will be executed when server is down

# all key value pairs in ShadowVPN config file will be passed to this script
# as environment variables, except password

# uncomment if you want to turn off IP forwarding
# sysctl -w net.ipv4.ip_forward=0

# get default gateway
gw_intf=`ip route show | grep '^default' | sed -e 's/.* dev \([^ ]*\).*/\1/'`

# turn off NAT over gw_intf and VPN
iptables -t nat -D POSTROUTING -o $gw_intf -m comment --comment "$gw_intf (shadowvpn)" -j MASQUERADE
iptables -D FORWARD -i $gw_intf -o $intf -m state --state RELATED,ESTABLISHED -j ACCEPT
iptables -D FORWARD -i $intf -o $gw_intf -j ACCEPT

# turn off MSS fix
# MSS = MTU - TCP header - IP header
iptables -t mangle -D FORWARD -p tcp -m tcp --tcp-flags SYN,RST SYN -j TCPMSS --clamp-mss-to-pmtu

echo $0 done
