#!/bin/sh

path=$(dirname $0)

list=$(grep -E "^([0-9]{1,3}\.){3}[0-9]{1,3}" $path/chnroute.txt |\
    sed -e "s/^/route \$action /" -e "s/$/ \$suf/")

cat <<-EOH > $path/chnroutes.sh
	#!/bin/sh

	if [ "\$1" = "down" -o "\$1" = "del" ]; then
	    action=del
	else
	    action=add
	    suf="via \$(ip route show 0/0 | grep via | grep -Eo '([0-9]{1,3}\.){3}[0-9]{1,3}')"
	fi

	ip -batch - <<EOF
	$list
	EOF
EOH
