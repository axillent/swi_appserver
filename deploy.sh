#!/bin/bash

dir="$HOME/swi_appserver"

# create folder if needed
if [ ! -d $dir ]; then
  echo "creating dir"
  mkdir $dir
fi

# kill running process if any
pid_file="$dir/.pid"
if [ -f $pid_file ]; then
	pid="`cat $pid_file`"
	echo "killing process $pid"
	kill $pid
	rm -f $pid_file
fi

# copy required files
if [ ! -f "$dir/swi_appserver.properties" ]; then
	echo "copy swi_appserver.properties"
	cp swi_appserver.properties $dir
fi

if [ ! -f "$dir/log4cpp.properties" ]; then
	echo "copy log4cpp.properties"
	cp log4cpp.properties $dir
fi

echo "copy swi_appserver etc."
cp -f swi_appserver check_run.sh $dir

cd $dir

# start process
./swi_appserver &> /dev/null &
echo $! > $pid_file

if [ -f .pid ]; then
	echo "process started `cat $pid_file`"
fi
