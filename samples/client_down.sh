#!/bin/sh

# example client down script
# will be executed when client is down

# all key value pairs in ShadowVPN config file will be passed to this script
# as environment variables, except password

# uncomment if you want to turn off IP forwarding
# sysctl -w net.ipv4.ip_forward=0

# turn off NAT over VPN
iptables -t nat -D POSTROUTING -o $intf -j MASQUERADE
iptables -D FORWARD -i $intf -m state --state RELATED,ESTABLISHED -j ACCEPT
iptables -D FORWARD -o $intf -j ACCEPT

#NFQUEUE
if [ "$tcp_mode" = 1 ]; then
	iptables -D INPUT -p tcp --sport $port -j NFQUEUE --queue-num $queue_num
fi

# change routing table
echo rollback the default route
ip route del $server
ip route del 0.0.0.0/1
ip route del 128.0.0.0/1
echo default route changed to original route

echo $0 done
