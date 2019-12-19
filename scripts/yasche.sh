#!/bin/bash

DIR=${0%/*}
LKMDIR=${DIR}/../lkm
LKM=${LKMDIR}/yasched.ko

MODE="666"
OPS=$1

shift

yasched_start() {
    echo "start yasched"
    /sbin/insmod ${LKM} $* || exit 1
    rm -f /dev/yacdev0
    major=$(awk '$2=="yacdev" {print $1}' /proc/devices)
    mknod /dev/yacdev0 c ${major} 0
    chmod $MODE /dev/yacdev0
}

yasched_stop() {
    echo "stop yasched"
    /sbin/rmmod -f yasched.ko && rm -f /dev/yacdev0
}

case ${OPS} in
    "start")
	yasched_start
	;;
    "stop")
	yasched_stop
	;;
    "restart")
	yasched_stop
	sleep 1
	yasched_start
	;;
    *)
	echo UNKNOWN operation
	exit 1
	;;
esac

