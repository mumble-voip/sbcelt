#!/usr/bin/env bash

UNAME=$(uname -s | tr [:upper:] [:lower:])
case "${UNAME}" in
	"linux") ;;
	"freebsd") ;;
	*)
		echo skip
		exit
		;;
esac

rm -f _test
cc -I ../../lib futex_test.c ../../lib/futex-${UNAME}.c -o _test
./_test
if [ $? -eq 0 ]
then
	echo ok
else
	echo fail
fi
