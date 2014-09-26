ShadowVPN
=========

[![Build Status]][Travis CI]

ShadowVPN is a fast, safe VPN based on libsodium. Designed for low end
devices, i.e. OpenWRT routers.

ShadowVPN is beta. Please open an issue if you found any bugs.

Install
-------

Make sure to set `--sysconfdir=/etc`. You'll find conf files under `/etc`.

    sudo apt-get install build-essential automake libtool
    git submodule update --init
    ./autogen.sh
    ./configure --enable-static --sysconfdir=/etc
    make && sudo make install

For OpenWRT: TODO

Usage
-----

Client:

    sudo shadowvpn -c /etc/shadowvpn/client.conf -s start
    sudo shadowvpn -c /etc/shadowvpn/client.conf -s stop

Server:

    sudo shadowvpn -c /etc/shadowvpn/server.conf -s start
    sudo shadowvpn -c /etc/shadowvpn/server.conf -s stop


[Build Status]:      https://img.shields.io/travis/clowwindy/ShadowVPN/master.svg?style=flat
[Travis CI]:         https://travis-ci.org/clowwindy/ShadowVPN
