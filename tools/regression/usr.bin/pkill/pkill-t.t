#!/bin/sh
# $FreeBSD: src/tools/regression/usr.bin/pkill/pkill-t.t,v 1.2 2005/11/07 16:56:16 pjd Exp $

base=`basename $0`

echo "1..2"

name="pkill -t <tty>"
tty=`ps -o tty -p $$ | tail -1`
if [ "$tty" = "??" ]; then
	tty="-"
	ttyshort="-"
else
	ttyshort=`echo $tty | cut -c 4-`
fi
sleep=`mktemp /tmp/$base.XXXXXX` || exit 1
ln -sf /bin/sleep $sleep
$sleep 5 &
sleep 0.3
pkill -f -t $tty $sleep
ec=$?
case $ec in
0)
	echo "ok 1 - $name"
	;;
*)
	echo "not ok 1 - $name"
	;;
esac
$sleep 5 &
sleep 0.3
pkill -f -t $ttyshort $sleep
ec=$?
case $ec in
0)
	echo "ok 2 - $name"
	;;
*)
	echo "not ok 2 - $name"
	;;
esac
rm -f $sleep
