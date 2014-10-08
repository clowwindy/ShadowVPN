#!/bin/sh

# example client down script for darwin
# will be executed when client is down

# all key value pairs in ShadowVPN config file will be passed to this script
# as environment variables, except password

# user-defined variables
remote_tun_ip=10.7.0.1

# revert routing table
echo reverting default route
route delete -net 128.0.0.0 $remote_tun_ip -netmask 128.0.0.0
route delete -net 0.0.0.0 $remote_tun_ip -netmask 128.0.0.0
route delete -net $server

# revert dns server
networksetup -setdnsservers Wi-Fi empty

echo $0 done
