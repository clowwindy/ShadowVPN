#!/bin/bash

cat private.txt chnroute.txt | python3 negate_network.py > foreign.txt
cat foreign.txt | python3 gen_foreign_sh.py > foreign.sh

