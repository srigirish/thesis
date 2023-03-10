#!/bin/sh
create_iface() {
 echo "creating riot${N}"
            ip tuntap add dev riot${N} mode tap user ${SUDO_USER} || exit 1
            ip link set riot${N} up
            ip address add dev riot${N} scope link fe80::1/64
}

activate_forwarding() {
sysctl -w net.ipv6.conf.riot${N}.forwarding=1 || exit 1
sysctl -w net.ipv6.conf.riot${N}.accept_ra=0 || exit 1
}

add_ipv6_addrs() {
prefix_len=128
address_addr=2001:db8::1
        ip address add ${address_addr}/${prefix_len} dev riot${N} || exit 1
}

cleanup() {
    echo "Cleaning up..."
    for N in $(seq 1 "$((COUNT))"); do
	 ip tuntap del riot${N} mode tap
done
   
}

while true; do
	case "$1" in
	     -c|--create)
            if [ -n "${COMMAND}" ]; then
                usage
                exit 2
            fi
            COMMAND="create"
            case "$2" in
                "")
                    shift ;;
                *[!0-9]*)
                    usage
                    exit 2;;
                *)
                    COUNT="$2"
                    shift 2 ;;
            esac
               break ;;
	esac
done
for N in $(seq 1 "$((COUNT))"); do
	create_iface || exit 1
	activate_forwarding || exit 1
done
for N in $(seq 0 "$((COUNT))"); do
	add_ipv6_addrs || exit 1
done
trap cleanup INT TERM
echo ' --- press ENTER to remove tap interfaces --- '
read var
cleanup

