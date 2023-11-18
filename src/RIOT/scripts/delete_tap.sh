#!/bin/sh

   echo marshall! | sudo -S ip tuntap delete riot1 mode tap
	sudo ip tuntap delete riot2 mode tap
	sudo ip tuntap delete riot3 mode tap
	sudo ip tuntap delete riot4 mode tap
	sudo ip tuntap delete riot5 mode tap
	sudo ip tuntap delete riot6 mode tap
