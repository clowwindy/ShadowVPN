#!/usr/bin/env python3
#
# Copyright (c) 2014 clowwindy
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

from ipaddress import ip_network
import sys
import bisect

s = [ip_network('0.0.0.0/0')]

for line in sys.stdin:
    line = line.strip()
    print(line, file=sys.stderr)
    ex_item = ip_network(line)
    i = bisect.bisect_right(s, ex_item) - 1
    while i < len(s):
        subnet = s[i]
        if subnet.overlaps(ex_item):
            # since chnroute.txt is sorted, here we are always operating
            # the last few objects in s, which is almost O(1)
            del s[i]
            sub_subnets = list(subnet.address_exclude(ex_item))
            sub_subnets.sort()
            for sub_subnet in sub_subnets:
                s.insert(i, sub_subnet)
                i += 1
        else:
            break

s.sort()

for subnet in s:
    print(subnet)
