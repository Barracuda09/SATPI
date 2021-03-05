#!/bin/sh

trap cleanup TERM INT KILL EXIT HUP QUIT STOP ABRT;

## Set here your variables
CHANNEL=$1
## End Set
pid=0

preexec() {
  ## Execute here your initialization commands
  tool_config select $CHANNEL && \
  tool_config filter all
  ## End Execute
}

runcmd() {
  ## Execute here your your running command (with PIPEs) in background
  tool_config stream - &
  ## End Execute
  pid=$!
}

cleanup() {
  if [ -z $pid ] || [ "$pid" -eq 0 ] ; then
    exit 1
  else
    kill -9 $pid
    ## Execute here your cleanup commands
    tool_config stop
    ## End Execute
  fi
  exit 0
}

preexec
runcmd
if [ -z $pid ] || [ "$pid" -eq 0 ] ; then
  exit 1
else
  wait $pid
fi
