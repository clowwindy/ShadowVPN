#!/bin/sh

# example server down script
# will be executed when server is down

ifconfig $intf down

# stop pf nat
#pfctl -d 

echo $0 done
