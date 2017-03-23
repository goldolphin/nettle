#!/bin/sh

conf=$1
log=$2

mkdir -p `dirname $log`

(./bin/nt-tunnel $conf 2>&1) | cronolog -S $log $log.%Y%m%d
