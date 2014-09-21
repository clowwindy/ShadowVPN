#!/bin/bash

ifconfig tun0 10.7.0.1 netmask 255.255.255.0
ifconfig tun0 mtu 1400

