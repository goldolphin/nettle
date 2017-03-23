#!/bin/sh

if [ "$#" -lt 5 ]; then
    echo "Invalid arguments.
Usage: $0 <local(host:port)> <tunnel(host:port)> <dev-name> <mtu> <target(host) ...>
"
exit 1
fi

# ./bin/nt-tuntap create nt-tun0
cwd=$(dirname $(readlink -f $0))

local=$1
tunnel=$2
dev=$3
mtu=$4
buffer_size=$((1000+$mtu))
shift 4

log=nt-proxy.log.`date +%Y%m%d_%H%M%S`
config=nt-proxy.conf.$local
echo "
tun_tap_name = $dev
tun_tap_buffer_size = $buffer_size

local = $local
tunnel = $tunnel
" > $config

$cwd/nt-proxy $config &> $log &
sleep 1
ifconfig $dev 127.0.0.6 mtu $mtu up

for target in $@; do
    ip route add $target dev nt-tun0
done
