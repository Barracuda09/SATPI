#!/bin/sh

## example: 
## ./run 192.168.0.121 0 auto:386000000 0,1,16,17,18,2000,2001,2011 | vlc -
##            |        |   |    |             |
##            |        |   |    |             +-- PIDs
##            +-- IP   |   |    +-- freq in hz
##                     |   +-- modulation
##                     +-- tuner ID

## Execute cleanup() when these signals are trapped
trap cleanup TERM INT KILL EXIT HUP QUIT STOP ABRT

## Set here your variables
IP=$1
TUNE_ID=$2
CHANNEL=$3
PIDSTR=$4
pid=0
## End Set

preexec() {
	hexstr=" "

	## Input Field Separator
	IFS=","
	for row in ${PIDSTR}
	do
		cols=$row
		for col in ${cols}
		do
			hexstr=$(printf "%s 0x%.4X" $hexstr $col)
		done
	done

	## Execute here your initialization commands
	hdhomerun_config $IP set /tuner$TUNE_ID/channel $CHANNEL
	hdhomerun_config $IP set /tuner$TUNE_ID/filter "$hexstr"
	## End Execute
}

runcmd() {
   ## Execute here your your running command (with PIPEs -) in background with &
   ## and save pid of tool
   hdhomerun_config $IP save /tuner$TUNE_ID - &
   pid=$!
   ## End Execute
}

cleanup() {
   if [ -d /proc/$pid ] ; then
     kill -9 $pid
   fi
   ## Execute here your cleanup commands
   hdhomerun_config  $IP set /tuner$TUNE_ID/channel none
   ## End Execute
   ## trap EXIT
   if [ -z $pid ] || [ "$pid" -eq 0 ] ; then
     exit 1
   else
     exit 0
   fi
}

## main

preexec
runcmd
if [ -z $pid ] || [ "$pid" -eq 0 ] ; then
   cleanup
else
   wait $pid
   cleanup
fi
