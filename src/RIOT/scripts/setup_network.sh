#!/bin/sh
make -C ~/RIOT/dist/tools/ethos clean all
make -C ~/RIOT/dist/tools/uhcpd clean all
echo <sudo_pw> | sudo -S ~/RIOT/dist/tools/ethos/setup_network.sh riot0 2001:db8::/64
