#!/bin/sh

set -x
# example server up script
# will be executed when server is up

# all key value pairs in ShadowVPN config file will be passed to this script
# as environment variables, except password

# turn on IP forwarding
sysctl -w net.inet.ip.forwarding=1

# configure IP address and MTU of VPN interface
ifconfig $intf 10.7.0.1 10.7.0.1 netmask 255.255.255.0 mtu $mtu up
route add -net 10.7.0.0/24 10.7.0.1

# copy pf.conf to /etc 
# pfctl -F all -f /etc/pf.conf
# pfctl -e

echo $0 done
