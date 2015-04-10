ShadowVPN
=========

[![Build Status]][Travis CI]

[中文说明][Chinese Readme]

ShadowVPN is a fast, safe VPN based on libsodium. Designed for low end
devices, i.e. OpenWRT routers.

For more details, [check here][Compare].

ShadowVPN is currently beta and have much work to do. If you want a stable version, 
please come by here a few months later.

Install
-------

#### Debian & Ubuntu

For Debian 7 and Ubuntu 12+, add the following line to `/etc/apt/sources.list`

    deb https://shadowvpn.org/debian wheezy main

Then

    apt-get update
    apt-get install shadowvpn
    service shadowvpn restart

Or see [Build deb Package].

#### Unix

Currently Linux, FreeBSD and OS X are supported.
Download a [release] and build. Do not clone the repo, since it's not stable.
Make sure to set `--sysconfdir=/etc`. You'll find conf files under `/etc`.

    # For Debian-based Linux
    sudo apt-get update
    sudo apt-get install build-essential automake libtool git -y
    ./configure --enable-static --sysconfdir=/etc
    make && sudo make install

#### OpenWRT

[Download precompiled] for OpenWRT Barrier Breaker.

Or build yourself: cd into [SDK] root, then

    pushd package
    git clone https://github.com/clowwindy/ShadowVPN.git
    popd
    make menuconfig # select Network/ShadowVPN
    make V=s
    scp bin/xxx/ShadowVPN-xxx-xxx.ipk root@192.168.1.1
    # then log in your box and use opkg to install that ipk file

#### Windows

See [Build for Windows].

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

If you installed using apt-get, you can use `sudo service shadowvpn start` instead.

Client:

    sudo shadowvpn -c /etc/shadowvpn/client.conf -s start
    sudo shadowvpn -c /etc/shadowvpn/client.conf -s stop

Client(OpenWRT):

    /etc/init.d/shadowvpn start
    /etc/init.d/shadowvpn stop

TCP mode:

    sudo shadowvpn -c /etc/shadowvpn/server.conf -s start -t -q 0
    sudo shadowvpn -c /etc/shadowvpn/server.conf -s stop -t -q 0

Use "-t" to enable TCP mode, and "-q" to specify queue to bind(the default is 0).

You can also read [LuCI Configuration].

Wiki
----

You can find all the documentation in the wiki:
https://github.com/clowwindy/ShadowVPN/wiki

License
-------

Copyright (C) 2015 clowwindy

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Bugs and Issues
----------------

- [FAQ]
- [Issue Tracker]
- [Mailing list]


[Build Status]:         https://travis-ci.org/clowwindy/ShadowVPN.svg?branch=master
[Build deb Package]:    https://github.com/clowwindy/ShadowVPN/wiki/Building-deb-Package
[Build for Windows]:    https://github.com/clowwindy/ShadowVPN/wiki/Build-for-Windows
[Compare]:              https://github.com/clowwindy/ShadowVPN/wiki/Compared-to-Shadowsocks-and-OpenVPN
[Chinese Readme]:       https://github.com/clowwindy/ShadowVPN/wiki/ShadowVPN-%E4%BD%BF%E7%94%A8%E8%AF%B4%E6%98%8E
[Download precompiled]: https://github.com/clowwindy/ShadowVPN/releases
[FAQ]:                  https://github.com/clowwindy/ShadowVPN/wiki/FAQ
[Issue Tracker]:        https://github.com/clowwindy/ShadowVPN/issues?state=open
[LuCI Configuration]:   https://github.com/clowwindy/ShadowVPN/wiki/Configure-Via-LuCI-on-OpenWRT
[Mailing list]:         http://groups.google.com/group/shadowsocks
[release]:              https://github.com/clowwindy/ShadowVPN/releases
[SDK]:                  http://wiki.openwrt.org/doc/howto/obtain.firmware.sdk
[Travis CI]:            https://travis-ci.org/clowwindy/ShadowVPN
