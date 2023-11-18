#!/bin/sh

echo marshall! | sudo -S ip route add 2001:db1::/64 via fe80::a6cf:12ff:fe9a:2dc8 dev wlp0s20f3
sudo ip route add 2001:db2::/64 via fe80::266f:28ff:fe95:d118 dev wlp0s20f3
sudo ip route add 2001:db3::/64 via fe80::963c:c6ff:fe34:d060 dev wlp0s20f3
sudo ip route add 2001:db4::/64 via fe80::caf0:9eff:fe74:ef14 dev wlp0s20f3
sudo ip route add 2001:db5::/64 via fe80::963c:c6ff:fe33:6a64 dev wlp0s20f3
sudo ip route add 2001:db6::/64 via fe80::963c:c6ff:fe32:580 dev wlp0s20f3
sudo ip route add 2001:db7::/64 via fe80::963c:c6ff:fe34:2090 dev wlp0s20f3
