ShadowVPN
=========

[![Build Status]][Travis CI]

ShadowVPN is a fast, safe VPN based on libsodium. Designed for OpenWRT.
ShadowVPN is beta. Please open an issue if you found any bugs.

Install
-------

Make sure to set `--sysconfdir=/etc`. You'll find conf files under `/etc`.

    git submodule init
    git submodule update
    ./configure --enable-static --sysconfdir=/etc
    make && sudo make install

For OpenWRT: TODO

Usage
-----

Client:

    sudo shadowvpn -c samples/client.conf -s start
    sudo shadowvpn -c samples/client.conf -s stop

Server:

    sudo shadowvpn -c samples/server.conf -s start
    sudo shadowvpn -c samples/server.conf -s stop


[Build Status]:      https://img.shields.io/travis/clowwindy/ShadowVPN/master.svg?style=flat
[Travis CI]:         https://travis-ci.org/clowwindy/ShadowVPN
