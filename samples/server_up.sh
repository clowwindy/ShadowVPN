#!/bin/sh

# example server up script
# will be executed when server is up

# all key value pairs in ShadowVPN config file will be passed to this script
# as environment variables, except password

# turn on IP forwarding
sysctl -w net.ipv4.ip_forward=1

# configure IP address and MTU of VPN interface
ifconfig $intf 10.7.0.1 netmask 255.255.255.0
ifconfig $intf mtu $mtu

# get default gateway
gw_intf=`ip route show | grep '^default' | head -n1 | sed -e 's/.* dev \([^ ]*\).*/\1/'`

# turn on NAT over gw_intf and VPN
if !(iptables-save -t nat | grep -q "$gw_intf (shadowvpn)"); then
  iptables -t nat -A POSTROUTING -o $gw_intf -m comment --comment "$gw_intf (shadowvpn)" -j MASQUERADE
fi
iptables -A FORWARD -i $gw_intf -o $intf -m state --state RELATED,ESTABLISHED -j ACCEPT
iptables -A FORWARD -i $intf -o $gw_intf -j ACCEPT

# turn on MSS fix
# MSS = MTU - TCP header - IP header
iptables -t mangle -A FORWARD -p tcp -m tcp --tcp-flags SYN,RST SYN -j TCPMSS --clamp-mss-to-pmtu

echo $0 done
