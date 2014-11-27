#!/bin/bash

# conn=1
# server=root@x.x.x.x

sudo sed -i -E "s/concurrency=[[:digit:]]+/concurrency=$conn/g" /etc/shadowvpn/client.conf
ssh $server "sed -i -E \"s/concurrency=[[:digit:]]+/concurrency=$conn/g\" /etc/shadowvpn/server.conf"
ssh $server "shadowvpn -c /etc/shadowvpn/server.conf -s restart"
sudo shadowvpn -s restart -c /etc/shadowvpn/client.conf
sleep 5

ping -c 20 -i 0.2 -n 10.8.0.1
curl http://speedtest.tokyo.linode.com/100MB-tokyo.bin >/dev/null
