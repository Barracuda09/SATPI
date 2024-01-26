#!/bin/sh

# use:
#    source ./env.sh [gcc12]
#
# CFLAGS as used for Vu+ uno 4K

# ARM based Chips
export CPU_FLAGS="-march=armv7-a -mtune=cortex-a15 -mfpu=neon-vfpv4"

if [[ -n $1 && $1 = "gcc12" ]] ; then
  export CXXPREFIX=arm-linux-gnueabihf-
  export CXXSUFFIX=-12
  export CXX=$CXXPREFIX"g++"$CXXSUFFIX
elif [[ -n $1 && $1 = "gcc4" ]] ; then
  export CXXPREFIX=~/vu/opt/toolchains/stbgcc-4.8-1.7/bin/arm-linux-gnueabihf-
  export CXXSUFFIX=
  export CXX=$CXXPREFIX"c++"$CXXSUFFIX
else
  export CXXPREFIX=~/vu/opt/toolchains/stbgcc-8.3-0.4/bin/arm-linux-gnueabihf-
  export CXXSUFFIX=
  export CXX=$CXXPREFIX"c++"$CXXSUFFIX
fi
echo Using: $CXX
$CXX --version
