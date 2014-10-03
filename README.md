ShadowVPN
=========

[![Build Status]][Travis CI]

ShadowVPN is a fast, safe VPN based on libsodium. Designed for low end
devices, i.e. OpenWRT routers.

ShadowVPN is beta. Please open an issue if you found any bugs.

Install
-------

Linux:

Make sure to set `--sysconfdir=/etc`. You'll find conf files under `/etc`.

    sudo apt-get install build-essential automake libtool
    git submodule update --init
    ./autogen.sh
    ./configure --enable-static --sysconfdir=/etc
    make && sudo make install

OpenWRT:

cd into [SDK] root, then

    pushd package
    git clone https://github.com/clowwindy/ShadowVPN.git
    popd
    make menuconfig # select Network/ShadowVPN
    make V=s
    scp bin/xxx/ShadowVPN-xxx-xxx.ipk root@192.168.1.1
    # then log in your box and use opkg to install that ipk file

Configuration
-------------

- You can find all the conf files under `/etc/shadowvpn`.
- For the client, edit `client.conf`.
- For the server, edit `server.conf`.
- Update `server` and `password` in those files.
- The script file specified by `up` will be executed after VPN is up.
- The script file specified by `down` will be executed after VPN is down.
- If you need to specify routing rules, modify those scripts. You'll see a
placeholder at the end of those scripts.

Notice ShadowVPN is a peer-to-peer VPN, which means you'll have one server
for one client. If you have multiple clients, you should start multiple server
instances, which can be controlled by different configuration files via `-c`
argument. Make sure to use different IP for each instance in each `up` and
`down` scripts.

Usage
-----

Server:

    sudo shadowvpn -c /etc/shadowvpn/server.conf -s start
    sudo shadowvpn -c /etc/shadowvpn/server.conf -s stop

Client:

    sudo shadowvpn -c /etc/shadowvpn/client.conf -s start
    sudo shadowvpn -c /etc/shadowvpn/client.conf -s stop

Client(OpenWRT):

    /etc/init.d/shadowvpn start
    /etc/init.d/shadowvpn stop


[Build Status]:      https://img.shields.io/travis/clowwindy/ShadowVPN/master.svg?style=flat
[Travis CI]:         https://travis-ci.org/clowwindy/ShadowVPN
