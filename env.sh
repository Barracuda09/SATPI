#!/bin/sh

# use:
#    source ./env.sh
#
# CFLAGS as used for Vu+ uno 4K

#export INCLUDES=--sysroot=~/git/pi/tools/arm-bcm2708/arm-rpi-4.9.3-linux-gnueabihf/arm-linux-gnueabihf/sysroot
#export INCLUDES=


#export CXXPREFIX=~/vu/opt/toolchains/stbgcc-4.8-1.7/bin/arm-linux-
#export CXXSUFFIX=
#export CXX=~/vu/opt/toolchains/stbgcc-4.8-1.7/bin/arm-linux-c++
#export CPP=~/vu/opt/toolchains/stbgcc-4.8-1.7/bin/arm-linux-cpp
#export CC=~/vu/opt/toolchains/stbgcc-4.8-1.7/bin/arm-linux-gcc
#PYTHON_CPPFLAGS=~/vu/opt/toolchains/stbgcc-4.8-1.7/python-runtime/include/python2.7

export CPU_FLAGS="-march=armv7ve -mtune=cortex-a15 -mfpu=vfpv4"

export CXXPREFIX=~/vu/opt/toolchains/stbgcc-6.3-1.7/bin/arm-linux-
export CXXSUFFIX=
export CXX=~/vu/opt/toolchains/stbgcc-6.3-1.7/bin/arm-linux-c++
export CPP=~/vu/opt/toolchains/stbgcc-6.3-1.7/bin/arm-linux-cpp
export CC=~/vu/opt/toolchains/stbgcc-6.3-1.7/bin/arm-linux-gcc
export PYTHON_CPPFLAGS=~/vu/opt/toolchains/stbgcc-6.3-1.7/python-runtime/include/python2.7
