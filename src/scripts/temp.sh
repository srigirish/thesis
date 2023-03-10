#!/bin/sh

    ip tuntap add riot0 mode tap user ${USER}
    sysctl -w net.ipv6.conf.riot0.forwarding=1
    sysctl -w net.ipv6.conf.riot0.accept_ra=0
    ip link set riot0 up
    ip a a fe80::1/64 dev riot0
    ip a a fd00:dead:beef::1/128 dev lo
ip route add "2001:db8::/64" via fe80::2 dev "riot0"
