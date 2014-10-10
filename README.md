ShadowVPN
=========

[![Build Status]][Travis CI]

[中文说明][Chinese Readme]

ShadowVPN is a fast, safe VPN based on libsodium. Designed for low end
devices, i.e. OpenWRT routers.

For more details, [check here][Compare].

Install
-------

#### Unix

Currently Linux, FreeBSD and OS X are supported.
Clone the repo and build. Make sure to set `--sysconfdir=/etc`.
You'll find conf files under `/etc`.

    # For Debian-based Linux
    sudo apt-get install build-essential automake libtool
    git clone https://github.com/clowwindy/ShadowVPN.git
    git submodule update --init
    ./autogen.sh
    ./configure --enable-static --sysconfdir=/etc
    make && sudo make install

#### OpenWRT

[Download precompiled] for OpenWRT trunk and CPU: ar71xx, brcm63xx, brcm47xx,
ramips_24kec.

Or build yourself: cd into [SDK] root, then

    pushd package
    git clone https://github.com/clowwindy/ShadowVPN.git
    popd
    make menuconfig # select Network/ShadowVPN
    make V=s
    scp bin/xxx/ShadowVPN-xxx-xxx.ipk root@192.168.1.1
    # then log in your box and use opkg to install that ipk file

#### Windows

You need to install the TUN/TAP driver first:
* [For 32-bit Windows](http://build.openvpn.net/downloads/releases/tap-windows-9.9.2_3.exe)
* [For 64-bit Windows](http://build.openvpn.net/downloads/releases/tap-windows-9.21.0.exe)

Currently only MinGW compilers are supported. You can compile in Msys or
cross-compile in Linux or Cygwin with 32-bit or 64-bit MinGW toolchains.

For example, if using 64-bit Cygwin, install `libtool`, `autoconf`, `git`
and `mingw64-x86_64-gcc-g++` by Cygwin installer. Then build from Cygwin
terminal by the following commands:

    git clone --recursive https://github.com/clowwindy/ShadowVPN.git
    cd ShadowVPN
    ./autogen.sh
    ./configure --enable-static --host=x86_64-w64-mingw32
    make && make install DESTDIR="$HOME/shadowvpn-build"

Executables will be generated in `$HOME/shadowvpn-build`.

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
- If you are using Windows, the IP address of TUN/TAP device `tunip` is
required to be specified in the conf file.

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

You can also read [LuCI Configuration].

Wiki
----

You can find all the documentation in the wiki:
https://github.com/clowwindy/ShadowVPN/wiki

License
-------
MIT

Bugs and Issues
----------------

- [FAQ]
- [Issue Tracker]
- [Mailing list]


[Build Status]:         https://travis-ci.org/clowwindy/ShadowVPN.svg?branch=master
[Compare]:              https://github.com/clowwindy/ShadowVPN/wiki/Compared-to-Shadowsocks-and-OpenVPN
[Chinese Readme]:       https://github.com/clowwindy/ShadowVPN/wiki/ShadowVPN-%E4%BD%BF%E7%94%A8%E8%AF%B4%E6%98%8E
[Download precompiled]: https://github.com/clowwindy/ShadowVPN/releases
[FAQ]:                  https://github.com/clowwindy/ShadowVPN/wiki/FAQ
[Issue Tracker]:        https://github.com/clowwindy/ShadowVPN/issues?state=open
[LuCI Configuration]:   https://github.com/clowwindy/ShadowVPN/wiki/Configure-Via-LuCI-on-OpenWRT
[Mailing list]:         http://groups.google.com/group/shadowsocks
[SDK]:                  http://wiki.openwrt.org/doc/howto/obtain.firmware.sdk
[Travis CI]:            https://travis-ci.org/clowwindy/ShadowVPN
