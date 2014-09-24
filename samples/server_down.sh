#!/bin/bash

# example server down script
# will be executed when server is down

# all key value pairs in ShadowVPN config file will be passed to this script
# as environment variables, except password

# uncomment if you want to turn off IP forwarding
# sysctl -w net.ipv4.ip_forward=0

# turn off NAT over eth0 and VPN
# if you use other interface name that eth0, replace eth0 with it
iptables -t nat -D POSTROUTING -o eth0 -j MASQUERADE
iptables -D FORWARD -i eth0 -o $intf -m state --state RELATED,ESTABLISHED -j ACCEPT
iptables -D FORWARD -i $intf -o eth0 -j ACCEPT

# turn off MSS fix
declare -i mss
mss=$mth-40
iptables -t mangle -D FORWARD -p tcp --tcp-flags SYN,RST SYN -j TCPMSS --set-mss 1400

