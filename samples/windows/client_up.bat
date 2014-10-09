@ECHO off
REM example client up script for windows
REM will be executed when client is up

REM all key value pairs in ShadowVPN config file will be passed to this script
REM as environment variables, except password

REM user-defined variables
SET remote_tun_ip=10.7.0.1
SET dns_server=8.8.8.8
SET orig_intf="Local Area Connection"

REM exclude remote server in routing table
for /F "tokens=3" %%* in ('route print ^| findstr "\<0.0.0.0\>"') do set "orig_gw=%%*"
route add %server% %orig_gw% metric 5 > NUL

REM configure IP address and MTU of VPN interface
netsh interface ip set interface %orig_intf% ignoredefaultroutes=enabled > NUL
netsh interface ip set address name="%intf%" static %tunip% 255.255.255.0 > NUL
netsh interface ipv4 set subinterface "%intf%" mtu=%mtu% > NUL

REM change routing table
ECHO changing default route
netsh interface ipv4 add route 128.0.0.0/1 "%intf%" %remote_tun_ip% metric=6 > NUL
netsh interface ipv4 add route 0.0.0.0/1 "%intf%" %remote_tun_ip% metric=6 > NUL
ECHO default route changed to %remote_tun_ip%

REM change dns server
netsh interface ip set dns name="%intf%" static %dns_server% > NUL

ECHO %0 done
