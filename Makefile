###############################################################################
## Makefile for compiling code
###############################################################################

# Set the compiler being used.
CXX ?= $(CXXPREFIX)g++$(CXXSUFFIX)

# Check compiler support for some functions
RESULT_HAS_NP_FUNCTIONS := $(shell $(CXX) -o npfunc checks/npfunc.cpp -pthread 2> /dev/null ; echo $$? ; rm -rf npfunc)
RESULT_HAS_ATOMIC_FUNCTIONS := $(shell $(CXX) -o atomic checks/atomic.cpp 2> /dev/null ; echo $$? ; rm -rf atomic)

# Includes needed for proper compilation
INCLUDES +=

LDFLAGS += $(CPU_FLAGS)
CFLAGS += $(CPU_FLAGS)

# Libraries needed for linking
LDFLAGS += -pthread -lrt
# RESULT_HAS_ATOMIC_FUNCTIONS = 1 if compile fails
ifeq "$(RESULT_HAS_ATOMIC_FUNCTIONS)" "1"
  LDFLAGS += -latomic
endif

# Set Compiler Flags
CFLAGS += -I ./src -std=c++11 -Wall -Wextra -Winit-self -pthread $(INCLUDES)

# Build "debug", "release" or "simu"
ifeq "$(BUILD)" "debug"
  # "Debug" build - no optimization, with debugging symbols
  CFLAGS += -O0 -g3 -DDEBUG -fstack-protector-all -Wswitch-default
  LDFLAGS += -rdynamic
else ifeq "$(BUILD)" "debug1"
  # "Debug" build - with optimization, with debugging symbols
  CFLAGS += -O2 -g3 -DDEBUG -fstack-protector-all -Wswitch-default
  LDFLAGS += -rdynamic
else ifeq "$(BUILD)" "simu"
  # "Debug Simu" build - no optimization, with debugging symbols
  CFLAGS += -O0 -g3 -DDEBUG -DSIMU -fstack-protector-all
  LDFLAGS += -rdynamic
else
  # "Release" build - optimization, and no debug symbols
  CFLAGS += -O2 -s -DNDEBUG
endif

# List of source to be compiled
SOURCES = Version.cpp \
	InterfaceAttr.cpp \
	HttpServer.cpp \
	HttpcServer.cpp \
	Log.cpp \
	Properties.cpp \
	RtspServer.cpp \
	main.cpp \
	Satpi.cpp \
	Stream.cpp \
	StreamClient.cpp \
	StreamManager.cpp \
	StringConverter.cpp \
	base/M3UParser.cpp \
	base/Thread.cpp \
	base/ThreadBase.cpp \
	base/TimeCounter.cpp \
	base/XMLSaveSupport.cpp \
	base/XMLSupport.cpp \
	input/DeviceData.cpp \
	input/Transformation.cpp \
	input/dvb/Frontend.cpp \
	input/dvb/FrontendData.cpp \
	input/dvb/delivery/DiSEqc.cpp \
	input/dvb/delivery/DiSEqcEN50494.cpp \
	input/dvb/delivery/DiSEqcEN50607.cpp \
	input/dvb/delivery/DiSEqcSwitch.cpp \
	input/dvb/delivery/DVBC.cpp \
	input/dvb/delivery/DVBS.cpp \
	input/dvb/delivery/DVBT.cpp \
	input/dvb/delivery/Lnb.cpp \
	input/file/TSReader.cpp \
	input/file/TSReaderData.cpp \
	input/childpipe/TSReader.cpp \
	input/childpipe/TSReaderData.cpp \
	input/stream/Streamer.cpp \
	input/stream/StreamerData.cpp \
	mpegts/Filter.cpp \
	mpegts/PacketBuffer.cpp \
	mpegts/PAT.cpp \
	mpegts/PCR.cpp \
	mpegts/PidTable.cpp \
	mpegts/PMT.cpp \
	mpegts/SDT.cpp \
	mpegts/TableData.cpp \
	output/StreamThreadBase.cpp \
	output/StreamThreadHttp.cpp \
	output/StreamThreadRtcpBase.cpp \
	output/StreamThreadRtcp.cpp \
	output/StreamThreadRtcpTcp.cpp \
	output/StreamThreadRtp.cpp \
	output/StreamThreadRtpTcp.cpp \
	output/StreamThreadTSWriter.cpp \
	socket/HttpcSocket.cpp \
	socket/TcpSocket.cpp \
	socket/SocketAttr.cpp \
	socket/UdpSocket.cpp \
	upnp/ssdp/Server.cpp

# Add gprof, use gprof-helper.c for multithreading profiling
ifeq "$(GPROF)" "yes"
  LDFLAGS += -pg
  CFLAGS  += -pg
endif

# Add dvbcsa ?
ifeq "$(LIBDVBCSA)" "yes"
  LDFLAGS += -ldvbcsa
  CFLAGS  += -DLIBDVBCSA
  SOURCES += decrypt/dvbapi/Client.cpp
  SOURCES += decrypt/dvbapi/ClientProperties.cpp
  SOURCES += decrypt/dvbapi/Keys.cpp
  SOURCES += input/dvb/Frontend_DecryptInterface.cpp
endif

# Add dvbca ?
ifeq "$(DVBCA)" "yes"
  CFLAGS  += -DADDDVBCA
  SOURCES += decrypt/dvbca/DVBCA.cpp
endif

# Has np functions
# RESULT_HAS_NP_FUNCTIONS = 0 if compile is succesfull which means NP are OK
ifeq "$(HAS_NP_FUNCTIONS)" "yes"
  CFLAGS  += -DHAS_NP_FUNCTIONS
else ifeq "$(RESULT_HAS_NP_FUNCTIONS)" "0"
  CFLAGS  += -DHAS_NP_FUNCTIONS
endif

# Need to build for Enigma support
ifeq "$(ENIGMA)" "yes"
  CFLAGS += -DENIGMA
endif

# Sub dirs for project
OBJ_DIR = obj
SRC_DIR = src

# Get Version from git describe
ifneq ($(wildcard .git),)
  VER1 = $(shell git describe --long --match "v*" 2> /dev/null)
  ifeq ($(VER1),)
    # Git describe failed. Adding "-unknown" postfix to mark this situation
    VER1 = $(shell git describe --match "v*" 2> /dev/null)-unknown
  endif
  # Parse
  VER = $(shell echo $(VER1) | sed "s/^v//" | sed "s/-\([0-9]*\)-\(g[0-9a-f]*\)/.\1~\2/")
else
  VER = "1.6.2~unknown"
endif

# Do we build for ENIGMA, then add it to version string
FOUND_ENIGMA = $(findstring DENIGMA,$(CFLAGS))
ifneq ($(FOUND_ENIGMA),)
  VER += "Enigma"
endif

# Create/Update Version.cpp
VER_FILE = "Version.cpp"
NEW_VER = "const char *satpi_version = \"$(VER)\";"
OLD_VER = $(shell cat $(SRC_DIR)/$(VER_FILE) 2> /dev/null | sed -n -e "/const / s/.*\= \"*//p" | sed -n -e "s/-*\";//p")
#OLD_VER = $(shell cat $(SRC_DIR)/$(VER_FILE) 2> /dev/null)
ifneq ($(NEW_VER), $(OLD_VER))
  $(shell echo $(NEW_VER) > $(SRC_DIR)/$(VER_FILE))
endif

# Find all source (*.cpp) across the directories and make a list
SOURCES_UNC = $(shell find $(SRC_DIR)/ -type f -name '*.cpp')

# Find all headers (*.h) across the directories and make a list
HEADERS = $(shell find $(SRC_DIR)/ -type f -name '*.h')

# Objectlist is identical to Sourcelist except that all .cpp extensions need to be replaced by .o extension.
# and add OBJ_DIR to path
OBJECTS = $(SOURCES:%.cpp=$(OBJ_DIR)/%.o)

# Resulting Executable name
EXECUTABLE = satpi

# First rule is also the default rule
# generate the Executable from all the objects
# $@ represents the targetname
$(EXECUTABLE): $(OBJECTS) $(HEADERS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

# A pattern rule is used to build objects from 'cpp' files
# $< represents the first item in the dependencies list
# $@ represents the targetname
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS)
	@mkdir -p $(@D)
	$(CXX) -c $(CFLAGS) $< -o $@

# Create debug versions
debug:
	$(MAKE) "BUILD=debug"

debug1:
	$(MAKE) "BUILD=debug1" LIBDVBCSA=yes

# Create a 'simulation' version
simu:
	$(MAKE) "BUILD=simu"

# Create a 'test suite' Compile
testsuite:
	$(MAKE) clean
	$(MAKE) debug LIBDVBCSA=yes
	$(MAKE) clean
	$(MAKE) debug
	$(MAKE) clean
	$(MAKE) LIBDVBCSA=yes
	$(MAKE) clean
	$(MAKE)
	$(MAKE) clean

# Install Doxygen and Graphviz/dot
# sudo apt-get install graphviz doxygen
docu:
	doxygen ./doxygen

help:
	@echo "Help, use these command for building this project:"
	@echo " - Make project clean                   :  make clean"
	@echo " - Make debug version                   :  make debug"
	@echo " - Make debug version with DVBAPI       :  make debug LIBDVBCSA=yes"
	@echo " - Make debug version for ENIGMA        :  make debug ENIGMA=yes"
	@echo " - Make production version with DVBAPI  :  make LIBDVBCSA=yes"
	@echo " - Make PlantUML graph                  :  make plantuml"
	@echo " - Make Doxygen docmumentation          :  make docu"
	@echo " - Make Uncrustify Code Beautifier      :  make uncrustify"

# Download PlantUML from http://plantuml.com/download.html
# and put it into the root of the project directory
# sudo apt-get install graphviz openjdk-7-jre
plantuml:
	java -jar plantuml.jar -tsvg SatPI.plantuml
	java -jar plantuml.jar -tpng SatPI.plantuml

uncrustify:
	uncrustify -c SatPI.uncrustify --replace $(SOURCES_UNC)
	uncrustify -c SatPI.uncrustify --replace $(HEADERS)
	@echo uncrustify Done

checkcpp:
	~/cppcheck/cppcheck -DENIGMA -DLIBDVBCSA -DDVB_API_VERSION=5 -DDVB_API_VERSION_MINOR=5 -I ./src --enable=all --std=posix --std=c++11 ./src 1> cppcheck.log 2>&1

.PHONY:
	clean

clean:
	@echo Clearing project...
	@rm -rf testcode.c testcode ./obj $(EXECUTABLE) src/Version.cpp /web/*.*~
	@rm -rf src/*.*~ src/*~
	@echo ...Done
