#!/bin/bash

cat private.txt chnroute.txt | python3 negate_network.py > foreign.txt
