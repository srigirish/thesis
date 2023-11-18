#!/bin/sh

SUIT_COAP_SERVER=[2001:db8::1] SUIT_CLIENT=[fe80::a87c:61ff:fe72:5fc6%riot0] BOARD=nucleo-f767zi make -C ~/RIOT/projects/Layout2/ProcessA_UART suit/notify
SUIT_COAP_SERVER=[2001:db8::1] SUIT_CLIENT=[fe80::dc8f:f8ff:fede:2fd9%riot3] BOARD=nucleo-f767zi make -C ~/RIOT/projects/Layout2/ProductionTT_UART suit/notify
SUIT_COAP_SERVER=[2001:db8::1] SUIT_CLIENT=[fe80::acd9:e9ff:fe9b:1f2c%riot4] BOARD=nucleo-f767zi make -C ~/RIOT/projects/Layout2/Slider suit/notify
SUIT_COAP_SERVER=[2001:db8::1] SUIT_CLIENT=[fe80::a46b:5dff:fed5:6b18%riot5] BOARD=nucleo-f767zi make -C ~/RIOT/projects/Layout2/DispatchTT suit/notify
SUIT_COAP_SERVER=[2001:db8::1] SUIT_CLIENT=[fe80::a42e:c7ff:fefb:d9e%riot6] BOARD=nucleo-f767zi make -C ~/RIOT/projects/Layout2/DispatchA suit/notify
SUIT_COAP_SERVER=[2001:db8::1] SUIT_CLIENT=[fe80::e8c8:61ff:fe14:c102%riot7] BOARD=nucleo-f767zi make -C ~/RIOT/projects/Layout2/DispatchB suit/notify
