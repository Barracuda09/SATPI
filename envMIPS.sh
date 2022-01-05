#!/bin/sh

# use:
#    source ./env.sh
#
# CFLAGS as used for

###############################################################################
# MIPS based BroadCom Chips
export CPU_FLAGS=""

#export CXXPREFIX=~/vu/opt/toolchains/stbgcc-6.3-1.8/bin/mips-linux-
#export CXXSUFFIX=
#export CXX=~/vu/opt/toolchains/stbgcc-6.3-1.8/bin/mips-linux-c++
#export CPP=~/vu/opt/toolchains/stbgcc-6.3-1.8/bin/mips-linux-cpp
#export CC=~/vu/opt/toolchains/stbgcc-6.3-1.8/bin/mips-linux-gcc
#export PYTHON_CPPFLAGS=~/vu/opt/toolchains/stbgcc-6.3-1.8/python-runtime/include/python2.7

export CXXPREFIX=~/vu/opt/toolchains/stbgcc-8.3-0.4/bin/mips-linux-
export CXXSUFFIX=
export CXX=~/vu/opt/toolchains/stbgcc-8.3-0.4/bin/mips-linux-c++
export CPP=~/vu/opt/toolchains/stbgcc-8.3-0.4/bin/mips-linux-cpp
export CC=~/vu/opt/toolchains/stbgcc-8.3-0.4/bin/mips-linux-gcc
export PYTHON_CPPFLAGS=~/vu/opt/toolchains/stbgcc-8.3-0.4/python-runtime/include/python2.7
###############################################################################
