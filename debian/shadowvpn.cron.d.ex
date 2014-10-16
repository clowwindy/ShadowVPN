#
# Regular cron jobs for the shadowvpn package
#
0 4	* * *	root	[ -x /usr/bin/shadowvpn_maintenance ] && /usr/bin/shadowvpn_maintenance
