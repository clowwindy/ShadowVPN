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

# turn on NAT over VPN and eth0
# if you use other interface name that eth0, replace eth0 with it
iptables -t nat -A POSTROUTING -o $intf -j MASQUERADE
iptables -A FORWARD -i $intf -o eth0 -m state --state RELATED,ESTABLISHED -j ACCEPT
iptables -A FORWARD -i eth0 -o $intf -j ACCEPT

# get current gateway
echo reading old gateway from route table
old_gw_intf=`ip route show | grep '^default' | sed -e 's/.* dev \([^ ]*\).*/\1/'`
old_gw_ip=`ip route show | grep '^default' | sed -e 's/.* via \([^ ]*\).*/\1/'`

# if current gateway is tun, it indicates that our gateway is already changed
# read from saved file
if [ $old_gw_intf == "$intf" ]; then
  echo reading old gateway from /tmp/old_gw_intf
  old_gw_intf=`cat /tmp/old_gw_intf` || ( echo "can not read gateway, check up.sh" && exit 1 )
fi

echo saving old gateway to /tmp/old_gw_intf
echo $old_gw_intf > /tmp/old_gw_intf
echo $old_gw_ip > /tmp/old_gw_ip

# change routing table
echo changing default route
if [ pppoe-wan == $old_gw_intf ]; then
  route add $server $old_gw_intf
else
  route add $server gw $old_gw_ip
fi
route del default
route add default gw 10.7.0.1
echo default route changed to 10.7.0.1

echo $0 done
