#!/bin/sh

# example client up script
# will be executed when client is up

# all key value pairs in ShadowVPN config file will be passed to this script
# as environment variables, except password

# turn on IP forwarding
sysctl -w net.ipv4.ip_forward=1

# configure IP address and MTU of VPN interface
ifconfig $intf 10.7.0.2 netmask 255.255.255.0
ifconfig $intf mtu $mtu

# get current gateway
echo get the original default gateway
gateway=$(ip route show 0/0 | grep via | grep -Eo '([0-9]{1,3}\.){3}[0-9]{1,3}')

# turn on NAT over VPN
iptables -t nat -A POSTROUTING -o $intf -j MASQUERADE
iptables -I FORWARD 1 -i $intf -m state --state RELATED,ESTABLISHED -j ACCEPT
iptables -I FORWARD 1 -o $intf -j ACCEPT

# change routing table
echo override the default route
ip route add $server via $gateway
ip route add 0.0.0.0/1 via 10.7.0.1
ip route add 128.0.0.0/1 via 10.7.0.1
echo default route changed to 10.7.0.1

echo $0 done
