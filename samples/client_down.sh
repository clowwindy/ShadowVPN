#!/bin/bash

# example client down script
# will be executed when client is down

# all key value pairs in ShadowVPN config file will be passed to this script
# as environment variables, except password

# uncomment if you want to turn off IP forwarding
# sysctl -w net.ipv4.ip_forward=0

# turn off NAT over VPN and eth0
# if you use other interface name that eth0, replace eth0 with it
iptables -t nat -D POSTROUTING -o $intf -j MASQUERADE
iptables -D FORWARD -i $intf -o eth0 -m state --state RELATED,ESTABLISHED -j ACCEPT
iptables -D FORWARD -i eth0 -o $intf -j ACCEPT

# get old gateway
echo reading old gateway from /tmp/old_gw
old_gw=`cat /tmp/old_gw` || ( echo "can not read gateway, check up.sh" && exit 1 )

# change routing table
echo changing default route
route del $server gw $old_gw
route del default
route add default gw $old_gw
echo default route changed to $old_gw

############################################
# remove chnroutes rules here if you need! #
############################################

echo $0 done
