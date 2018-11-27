#!/bin/bash
# crontab line:
# */30 * * * * /home/orangepi/swi_appserver/check_run.sh >/dev/null 2>&1

#---------------------------
dir="$HOME/swi_appserver"
pid_file=".pid"
server="swi_appserver"

#---------------------------
cd $dir

pid_fromfile="`cat $pid_file`"
pid_fromps="`pgrep $server`"

#echo "file '$pid_fromfile'"
#echo "ps '$pid_fromps'"

#---------------------------
if [ "$pid_fromps" != "" ]; then
	echo "is running"
	if [ "$pid_fromfile" != "$pid_fromps" ]; then
		echo "correct $pid_file"
		echo $pid_fromps > $pid_file
	fi
else
	echo "not running, starting"
	./$server &> /dev/null &
	echo $! > $pid_file
fi

