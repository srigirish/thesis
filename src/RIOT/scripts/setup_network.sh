#!/bin/sh
cd ..
make -C dist/tools/ethos clean all
make -C dist/tools/uhcpd clean all
echo <sudo_pw> | sudo -S dist/tools/ethos/setup_network.sh riot0 2001:db8::/64
