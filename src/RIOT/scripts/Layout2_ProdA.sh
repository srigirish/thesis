#!/bin/sh

BOARD=nucleo-f767zi SUIT_COAP_SERVER=[2001:db8::1] make -C ~/RIOT/projects/Layout2/ProcessA_UART suit/publish

