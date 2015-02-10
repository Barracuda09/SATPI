#!/bin/sh
#
# Update version file (if required)
#

# Path to version file
FILE=$1

# get version from git describe
if [ -d ".git" ]; then
	VER=$(git describe --long --match "v*" 2> /dev/null)
	if [ $? -ne 0 ]; then
		# Git describe failed. Adding "-unknown" postfix to mark this situation
		VER=$(git describe --match "v*" 2> /dev/null)-unknown
	fi
	VER=$(echo $VER | sed "s/^v//" | sed "s/-\([0-9]*\)-\(g[0-9a-f]*\)/.\1~\2/")
else
  VER="0.0.0~unknown"
fi

# update version?
NEW_VER="const char *satpi_version = \"$VER\";"
OLD_VER=$(cat "$FILE" 2> /dev/null)
if [ "$NEW_VER" != "$OLD_VER" ]; then
  echo $NEW_VER > "$FILE"
fi

echo $VER
