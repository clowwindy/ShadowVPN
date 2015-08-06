@ECHO off
REM example client down script for windows
REM will be executed when client is down

REM all key value pairs in ShadowVPN config file will be passed to this script
REM as environment variables, except password

REM user-defined variables
SET remote_tun_ip=10.7.0.0
SET orig_intf="Ethernet"

REM revert ip settings
netsh interface ip set interface %orig_intf% ignoredefaultroutes=disabled > NUL
netsh interface ip set address name="%intf%" dhcp > NUL

REM revert routing table
ECHO reverting default route
route delete 0.0.0.0 mask 128.0.0.0 %remote_tun_ip% > NUL
route delete 128.0.0.0 mask 128.0.0.0 %remote_tun_ip% > NUL
route delete %server% > NUL

REM revert dns server
netsh interface ip set dns name="%intf%" source=dhcp > NUL
netsh interface ip set dns name="%orig_intf%" source=dhcp > NUL

ECHO %0 done
