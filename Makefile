###############################################################################
## Makefile for compiling code
###############################################################################

# prefix set to /usr -> remove when ./configure is introduced
PREFIX=/usr

# Set the compiler being used.
CXX ?= $(CXXPREFIX)g++$(CXXSUFFIX)

# Check compiler support for some functions
RESULT_HAS_NP_FUNCTIONS := $(shell $(CXX) -o npfunc checks/npfunc.cpp -pthread 2> /dev/null ; echo $$? ; rm -rf npfunc)
RESULT_HAS_ATOMIC_FUNCTIONS := $(shell $(CXX) -o atomic checks/atomic.cpp 2> /dev/null ; echo $$? ; rm -rf atomic)
RESULT_HAS_BACKTRACE_FUNCTIONS := $(shell $(CXX) -o backtrace checks/backtrace.cpp 2> /dev/null ; echo $$? ; rm -rf backtrace)
RESULT_HAS_SYS_DVBS2X := $(shell $(CXX) -o sysdvb2sx checks/sysdvb2sx.cpp 2> /dev/null ; echo $$? ; rm -rf sysdvb2sx)
RESULT_HAS_STRING_VIEW := $(shell $(CXX) -std=c++17 -o string_view checks/string_view.cpp -pthread 2> /dev/null ; echo $$? ; rm -rf string_view)

# Includes needed for proper compilation
INCLUDES +=

LDFLAGS += $(CPU_FLAGS)
CFLAGS += $(CPU_FLAGS)
CFLAGS_OPT += $(CPU_FLAGS)

# Libraries needed for linking
LDFLAGS += -pthread -lrt

# Enable Backport mode (ancient compilers previous to C++17)
ifeq "$(RESULT_HAS_STRING_VIEW)" "1"
	CFLAGS     += -isystem nonstd -DNEED_BACKPORT
  CFLAGS_OPT += -isystem nonstd -DNEED_BACKPORT
endif

ifeq "$(CPP11)" "yes"
	CFLAGS     += -isystem nonstd -DNEED_BACKPORT
  CFLAGS_OPT += -isystem nonstd -DNEED_BACKPORT
endif

# RESULT_HAS_ATOMIC_FUNCTIONS = 1 if compile fails
ifeq "$(RESULT_HAS_ATOMIC_FUNCTIONS)" "1"
  LDFLAGS += -latomic
endif

# RESULT_HAS_BACKTRACE_FUNCTIONS = 1 if compile fails
ifeq "$(RESULT_HAS_BACKTRACE_FUNCTIONS)" "1"
  CFLAGS     += -DHAS_NO_BACKTRACE_FUNCTIONS
  CFLAGS_OPT += -DHAS_NO_BACKTRACE_FUNCTIONS
endif

# RESULT_HAS_SYS_DVBS2X = 1 if compile fails
ifeq "$(RESULT_HAS_SYS_DVBS2X)" "1"
  CFLAGS     += -DDEFINE_SYS_DVBS2X
  CFLAGS_OPT += -DDEFINE_SYS_DVBS2X
endif

# RESULT_HAS_NP_FUNCTIONS = 0 if compile is succesfull which means NP are OK
ifeq "$(RESULT_HAS_NP_FUNCTIONS)" "0"
  CFLAGS     += -DHAS_NP_FUNCTIONS
  CFLAGS_OPT += -DHAS_NP_FUNCTIONS
endif

# Set Compiler Flags
CFLAGS     += -I src -std=c++17 -Werror=vla -Wall -Wextra -Winit-self -Wshadow -pthread $(INCLUDES)
CFLAGS_OPT += -I src -std=c++17 -Werror=vla -Wall -Wextra -Winit-self -Wshadow -pthread $(INCLUDES)

# Build "debug", "release" or "simu"
ifeq "$(BUILD)" "debug"
  # "Debug" build - no optimization, with debugging symbols
  CFLAGS     += -O0 -g3 -gdwarf-2 -DDEBUG -DDEBUG_LOG -fstack-protector-all -Wswitch-default
  CFLAGS_OPT += -O3 -s -DNDEBUG -DDEBUG_LOG
  LDFLAGS    += -rdynamic
else ifeq "$(BUILD)" "debug1"
  # "Debug" build - with optimization, with debugging symbols
  CFLAGS     += -O0 -g3 -gdwarf-2 -DDEBUG -DDEBUG_LOG -fstack-protector-all -Wswitch-default
  CFLAGS_OPT += -O0 -g3 -gdwarf-2 -DDEBUG -DDEBUG_LOG -fstack-protector-all -Wswitch-default
  LDFLAGS    += -rdynamic
else ifeq "$(BUILD)" "speed"
  # "Debug" build - with optimization, without debugging symbols
  CFLAGS     += -Os -s -DNDEBUG
  CFLAGS_OPT += -Os -s -DNDEBUG
else ifeq "$(BUILD)" "simu"
  # "Debug Simu" build - no optimization, with debugging symbols
  CFLAGS     += -O0 -g3 -DDEBUG -DDEBUG_LOG -DSIMU -fstack-protector-all
  CFLAGS_OPT += -O3 -s  -DNDEBUG -DDEBUG_LOG
  LDFLAGS    += -rdynamic
else ifeq "$(BUILD)" "static"
  CFLAGS     += -O2 -s -DNDEBUG
  CFLAGS_OPT += -O3 -s -DNDEBUG
  LDFLAGS    += -static
else
  # "Release" build - optimization, and no debug symbols
  CFLAGS     += -O2 -s -DNDEBUG
  CFLAGS_OPT += -O3 -s -DNDEBUG
  LDFLAGS    += -rdynamic
endif

# List of source to be compiled
SOURCES = Version.cpp \
	InterfaceAttr.cpp \
	HeaderVector.cpp \
	HttpServer.cpp \
	HttpcServer.cpp \
	Log.cpp \
	Properties.cpp \
	RtspServer.cpp \
	main.cpp \
	Satpi.cpp \
	Stream.cpp \
	StreamManager.cpp \
	StringConverter.cpp \
	TransportParamVector.cpp \
	Utils.cpp \
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
	input/dvb/delivery/DiSEqcLnb.cpp \
	input/dvb/delivery/DiSEqcSwitch.cpp \
	input/dvb/delivery/DVBC.cpp \
	input/dvb/delivery/DVBS.cpp \
	input/dvb/delivery/DVBT.cpp \
	input/dvb/delivery/FBC.cpp \
	input/dvb/delivery/Lnb.cpp \
	input/file/TSReader.cpp \
	input/file/TSReaderData.cpp \
	input/childpipe/TSReader.cpp \
	input/childpipe/TSReaderData.cpp \
	input/stream/Streamer.cpp \
	input/stream/StreamerData.cpp \
	mpegts/Filter.cpp \
	mpegts/Generator.cpp \
	mpegts/NIT.cpp \
	mpegts/PacketBuffer.cpp \
	mpegts/PAT.cpp \
	mpegts/PCR.cpp \
	mpegts/PidTable.cpp \
	mpegts/PMT.cpp \
	mpegts/SDT.cpp \
	mpegts/TableData.cpp \
	output/StreamClient.cpp \
	output/StreamClientOutputHttp.cpp \
	output/StreamClientOutputRtp.cpp \
	output/StreamClientOutputRtpTcp.cpp \
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
  LDFLAGS    += -ldvbcsa -ldl
#  CFLAGS     += -DUSE_DEPRECATED_DVBAPI
  CFLAGS     += -DLIBDVBCSA
  CFLAGS_OPT += -DLIBDVBCSA
  SOURCES    += decrypt/dvbapi/Client.cpp
  SOURCES    += decrypt/dvbapi/ClientProperties.cpp
  SOURCES    += decrypt/dvbapi/Keys.cpp
  SOURCES    += input/dvb/Frontend_DecryptInterface.cpp
endif

# Add dvbca ?
ifeq "$(DVBCA)" "yes"
  CFLAGS  += -DADDDVBCA
  SOURCES += decrypt/dvbca/DVBCA.cpp
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
  VER1 = $(shell git describe --long --match "V*" 2> /dev/null)
  ifeq ($(VER1),)
    # Git describe failed. Adding "-unknown" postfix to mark this situation
    VER1 = $(shell git describe --match "V*" 2> /dev/null)-unknown
  endif
  # Parse
  VER = $(shell echo $(VER1) | sed "s/^V//" | sed "s/-\([0-9]*\)-\(g[0-9a-f]*\)/.\1~\2/")
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
$(OBJ_DIR)/mpegts/%.o: $(SRC_DIR)/mpegts/%.cpp $(HEADERS)
	@mkdir -p $(@D)
	$(CXX) -c $(CFLAGS_OPT) $< -o $@

$(OBJ_DIR)/decrypt/dvbapi/%.o: $(SRC_DIR)/decrypt/dvbapi/%.cpp $(HEADERS)
	@mkdir -p $(@D)
	$(CXX) -c $(CFLAGS_OPT) $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS)
	@mkdir -p $(@D)
	$(CXX) -c $(CFLAGS) $< -o $@


# Create debug versions
debug:
	$(MAKE) "BUILD=debug"

debug1:
	$(MAKE) "BUILD=debug1" LIBDVBCSA=yes

static:
	$(MAKE) "BUILD=static"

speed:
	$(MAKE) "BUILD=speed" LIBDVBCSA=yes

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
	@echo " - Make debug version with DVBAPI(ICAM) :  make debug LIBDVBCSA=yes"
	@echo " - Make debug version for ENIGMA        :  make debug ENIGMA=yes"
	@echo " - Make production version with DVBAPI  :  make LIBDVBCSA=yes"
	@echo " - Make production version with DVBAPI  :  make speed LIBDVBCSA=yes"
	@echo " - Make PlantUML graph                  :  make plantuml"
	@echo " - Make Doxygen docmumentation          :  make docu"
	@echo " - Make Uncrustify Code Beautifier      :  make uncrustify"
	@echo " - Enable compatibility with non-C++17  :  make non-c++17"

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

# Enable compatibility with non-C++17 compilers
# wget -P backports/ https://github.com/Barracuda09/SATPI/pull/173.diff
non-c++17:
	patch -p1 < backports/173.diff
	@echo patching Done

checkcpp:
	~/cppcheck/cppcheck -DENIGMA -DLIBDVBCSA -DDVB_API_VERSION=5 -DDVB_API_VERSION_MINOR=5 -I ./src --enable=all --std=posix --std=c++11 ./src 1> cppcheck.log 2>&1

.PHONY:
	clean

clean:
	@echo Clearing project...
	@rm -rf testcode.c testcode ./obj $(EXECUTABLE) src/Version.cpp /web/*.*~
	@rm -rf src/*.*~ src/*~
	@echo ...Done

.PHONY: install
install: $(EXECUTABLE)
# install binary
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp $< $(DESTDIR)$(PREFIX)/bin/$(EXECUTABLE)
# install systemd file
	mkdir -p $(DESTDIR)/lib/systemd/system
	cp data/satpi.service $(DESTDIR)/lib/systemd/system
# install web data
	mkdir -p $(DESTDIR)/usr/share/satpi/web
	cp -r web/*  $(DESTDIR)/usr/share/satpi/web
# create data dir
	mkdir -p $(DESTDIR)/var/lib/satpi

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(EXECUTABLE)
	rm -f $(DESTDIR)/lib/systemd/system/satpi.service
	rm -f $(DESTDIR)/usr/share/satpi
