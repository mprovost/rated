#!/bin/ksh
# $Id: updatetargets.sh,v 1.1 2008/01/19 03:22:14 btoneill Exp $
# $Author: btoneill $
################################################
# I run this script out of cron to update my
# targets.cfg file on a nightly basis and to 
# restart rtgpoll after it's been updated.
################################################

. /opt/rtg/targetmaker/etc/.shellsetup

echo Running findsnmp_hosts...
$d $FINDSNMP
echo Running targetmaker
$d $TARGETMAKER --output $RTGTARGET
echo Killing existing rtgpoll application
$d kill -9 `cat $RTGPOLLPID`
$d sleep 15
echo Starting rtgpoll application
$d $RTGPOLL $RTGPOLLARGS $RUNASDAEMON $ZERODELTA -c $RTGCONF -t $RTGTARGET -p $RTGPOLLPID > $OUTPUT 2>&1

exit 0;
