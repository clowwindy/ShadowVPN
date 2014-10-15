#!/usr/bin/env python3

from ipaddress import ip_address, ip_network
import sys

s = [ip_network('0.0.0.0/0')]

for line in sys.stdin:
    ip = line.strip()
    print(ip, file=sys.stderr)
    # TODO use binary search
    subnet = ip_network(ip)
    new_s = []
    for net in s:
        if net.overlaps(subnet):
            for ss in net.address_exclude(subnet):
                new_s.append(ss)
        else:
            new_s.append(net)
    s = new_s

s.sort()
for ip in s:
    print(ip)
