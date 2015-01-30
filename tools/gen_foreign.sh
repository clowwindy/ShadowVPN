#!/bin/bash

pushd $(dirname $0)

cat private.txt chnroute.txt | python3 negate_network.py > foreign.txt

list=$(grep -E "^([0-9]{1,3}\.){3}[0-9]{1,3}" foreign.txt |\
    sed -e "s/^/route \$action /" -e "s/$/ \$suf/")

cat <<-EOH > foreign.sh
	#!/bin/sh

	if [ "\$1" = "down" -o "\$1" = "del" ]; then
	    action=del
	else
	    action=add
	    suf="via 10.7.0.1"
	fi

	ip -batch - <<EOF
	$list
	EOF
EOH

popd
