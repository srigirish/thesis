#!/bin/sh

   echo <sudo_pw> | sudo -S ip tuntap add riot1 mode tap user ${USER}
   sudo sysctl -w net.ipv6.conf.riot1.forwarding=1
   sudo sysctl -w net.ipv6.conf.riot1.accept_ra=0
   sudo ip link set riot1 up
   sudo ip a a fe80::1/64 dev riot1
   sudo ip a a fd00:dead:beef::1/128 dev lo
   sudo ip address add 2001:db8::1/128 dev riot1
   sudo ip route add "2001:db8::/64" via fe80::2 dev "riot1"
   
   sudo ip tuntap add riot2 mode tap user ${USER}
   sudo sysctl -w net.ipv6.conf.riot2.forwarding=1
   sudo sysctl -w net.ipv6.conf.riot2.accept_ra=0
   sudo ip link set riot2 up
   sudo ip a a fe80::1/64 dev riot2
   sudo ip address add 2001:db8::1/128 dev riot2
   sudo ip route add "2001:db8::/64" via fe80::2 dev "riot2"
   
   sudo ip tuntap add riot3 mode tap user ${USER}
   sudo sysctl -w net.ipv6.conf.riot3.forwarding=1
   sudo sysctl -w net.ipv6.conf.riot3.accept_ra=0
   sudo ip link set riot3 up
   sudo ip a a fe80::1/64 dev riot3
   sudo ip address add 2001:db8::1/128 dev riot3
   sudo ip route add "2001:db8::/64" via fe80::2 dev "riot3"
   
   sudo ip tuntap add riot4 mode tap user ${USER}
   sudo sysctl -w net.ipv6.conf.riot4.forwarding=1
   sudo sysctl -w net.ipv6.conf.riot4.accept_ra=0
   sudo ip link set riot4 up
   sudo ip a a fe80::1/64 dev riot4
   sudo ip address add 2001:db8::1/128 dev riot4
   sudo ip route add "2001:db8::/64" via fe80::2 dev "riot4"
   
   sudo ip tuntap add riot5 mode tap user ${USER}
   sudo sysctl -w net.ipv6.conf.riot5.forwarding=1
   sudo sysctl -w net.ipv6.conf.riot5.accept_ra=0
   sudo ip link set riot5 up
   sudo ip a a fe80::1/64 dev riot5
   sudo ip address add 2001:db8::1/128 dev riot5
   sudo ip route add "2001:db8::/64" via fe80::2 dev "riot5"
   
   sudo ip tuntap add riot6 mode tap user ${USER}
   sudo sysctl -w net.ipv6.conf.riot6.forwarding=1
   sudo sysctl -w net.ipv6.conf.riot6.accept_ra=0
   sudo ip link set riot6 up
   sudo ip a a fe80::1/64 dev riot6
   sudo ip address add 2001:db8::1/128 dev riot6
   sudo ip route add "2001:db8::/64" via fe80::2 dev "riot6"
   
   sudo ip address add 2001:db8::1/128 dev riot0
