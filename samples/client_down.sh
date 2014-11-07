#!/bin/sh

# example client down script
# will be executed when client is down

# all key value pairs in ShadowVPN config file will be passed to this script
# as environment variables, except password

# uncomment if you want to turn off IP forwarding
# sysctl -w net.ipv4.ip_forward=0

# get old gateway
echo reading old gateway from /tmp/old_gw_intf
old_gw_ip=`cat /tmp/old_gw_ip` && old_gw_intf=`cat /tmp/old_gw_intf`
rm -f /tmp/old_gw_intf /tmp/old_gw_ip
if [ -z "$old_gw_intf" ]; then
  echo "can not read gateway, check up.sh"
  exit 1
fi

# turn off NAT over VPN and old_gw_intf
iptables -t nat -D POSTROUTING -o $intf -j MASQUERADE
iptables -D FORWARD -i $intf -o $old_gw_intf -m state --state RELATED,ESTABLISHED -j ACCEPT
iptables -D FORWARD -i $old_gw_intf -o $intf -j ACCEPT

# change routing table
echo changing default route
route del $server $old_gw_intf
route del default
if [ pppoe-wan = "$old_gw_intf" ]; then
  route add default $old_gw_intf
else
  route add default gw $old_gw_ip
fi
echo default route changed to $old_gw_intf

echo $0 done
