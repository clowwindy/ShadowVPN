shadowvpnconfdir = $(sysconfdir)/shadowvpn

shadowvpnconf_DATA = client.conf \
	client_down.sh \
	client_up.sh \
	server.conf \
	server_down.sh \
	server_up.sh

EXTRA_DIST = $(shadowvpnconf_DATA) \
  30-shadowvpn \
	freebsd/freebsd_server_down.sh \
	freebsd/freebsd_server_up.sh \
	freebsd/pf.conf
