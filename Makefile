###############################################################################
## Makefile for compiling code
###############################################################################

# Set the compiler being used.
CXX = $(CXXPREFIX)g++$(CXXSUFFIX)

# Includes needed for proper compilation
INCLUDES =

# Libraries needed for linking
LDFLAGS = -pthread -lrt

# Set Compiler Flags
CFLAGS = -I ./src -std=c++11 -Wall -Wextra -Winit-self -pthread $(INCLUDES)

# Build "debug", "release" or "simu"
ifeq ($(BUILD),debug)
  # "Debug" build - no optimization, with debugging symbols
  CFLAGS += -O0 -g3 -DDEBUG -fstack-protector-all
else ifeq ($(BUILD),simu)
  # "Debug Simu" build - no optimization, with debugging symbols
  CFLAGS += -O0 -g3 -DDEBUG -DSIMU -fstack-protector-all
else
  # "Release" build - optimization, and no debug symbols
  CFLAGS += -O2 -s -DNDEBUG
endif

# List of source to be compiled
SOURCES = Version.cpp \
	Satpi.cpp \
	StreamProperties.cpp \
	StreamProperties_ChannelData_get.cpp \
	StreamProperties_ChannelData_set.cpp \
	Properties.cpp \
	HttpcSocket.cpp \
	TcpSocket.cpp \
	UdpSocket.cpp \
	StreamClient.cpp \
	Streams.cpp \
	Stream.cpp \
	HttpServer.cpp \
	HttpcServer.cpp \
	RtspServer.cpp \
	RtcpThread.cpp \
	StreamThreadBase.cpp \
	RtpStreamThread.cpp \
	HttpStreamThread.cpp \
	Log.cpp \
	StringConverter.cpp \
	base/TimeCounter.cpp \
	base/XMLSupport.cpp \
	input/ChannelData.cpp \
	input/PidTable.cpp \
	input/dvb/Frontend.cpp \
	input/dvb/delivery/DVB_S.cpp \
	mpegts/PacketBuffer.cpp \
	upnp/ssdp/Server.cpp

# Add dvbcsa ?
ifeq ($(LIBDVBCSA),yes)
  LDFLAGS += -ldvbcsa
  CFLAGS  += -DLIBDVBCSA
  SOURCES += decrypt/dvbapi/Client.cpp
  SOURCES += Stream_InterfaceDecrypt.cpp
endif

ifeq ($(HAS_NP_FUNCTIONS),yes)
  CFLAGS  += -DHAS_NP_FUNCTIONS
endif

# Sub dirs for project
OBJ_DIR = obj
SRC_DIR = src

#SOURCES_UNC is identical to Sourcelist except that it adds SRC_DIR
SOURCES_UNC = $(SOURCES:%.cpp=$(SRC_DIR)/%.cpp)

# use wildcard expansion when the variable is assigned.
# (otherwise the variable will just contain the string '*.h' instead of the complete file list)
HEADERS = $(wildcard $(SRC_DIR)/*.h)

#Objectlist is identical to Sourcelist except that all .cpp extensions need to be replaced by .o extension.
# and add OBJ_DIR to path
OBJECTS = $(SOURCES:%.cpp=$(OBJ_DIR)/%.o)

#Resulting Executable name
EXECUTABLE = satpi

# First rule is also the default rule
# generate the Executable from all the objects
# $@ represents the targetname
$(EXECUTABLE):  makeobj $(OBJECTS) $(HEADERS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

# A pattern rule is used to build objects from 'cpp' files
# $< represents the first item in the dependencies list
# $@ represents the targetname
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS)
	$(CXX) -c $(CFLAGS) $< -o $@

# Create Version.cpp
$(OBJ_DIR)/Version.o:
	./version.sh $(SRC_DIR)/Version.cpp > /dev/null
	$(CXX) -c $(CFLAGS) $(SRC_DIR)/Version.cpp -o $@

makeobj:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(OBJ_DIR)/base
	@mkdir -p $(OBJ_DIR)/mpegts
	@mkdir -p $(OBJ_DIR)/input
	@mkdir -p $(OBJ_DIR)/input/dvb
	@mkdir -p $(OBJ_DIR)/input/dvb/delivery
	@mkdir -p $(OBJ_DIR)/upnp/ssdp
	@mkdir -p $(OBJ_DIR)/decrypt/dvbapi

debug:
	$(MAKE) "BUILD=debug"

simu:
	$(MAKE) "BUILD=simu"

testcode:
ifndef HAS_NP_FUNCTIONS
	@echo "#include <stdio.h>" > testcode.c
	@echo "#ifndef _GNU_SOURCE" >> testcode.c
	@echo "#define _GNU_SOURCE _GNU_SOURCE" >> testcode.c
	@echo "#endif" >> testcode.c
	@echo "#include <pthread.h>" >> testcode.c
	@echo "int main(void) {" >> testcode.c
	@echo "  pthread_t _thread;" >> testcode.c
	@echo "  pthread_setname_np(_thread, \"Hello\");" >> testcode.c
	@echo "  return 0;" >> testcode.c
	@echo "}" >> testcode.c
#	@$(CXX) testcode.c -o testcode -pthread 2> /dev/null ; if [ $$? -eq 0 ] ; then $(MAKE) -f Makefile.mk $(MAKECMDGOALS) HAS_NP_FUNCTIONS=yes ; else $(MAKE) -f Makefile.mk $(MAKECMDGOALS) HAS_NP_FUNCTIONS=no ; fi
	-@$(CXX) -o testcode testcode.c -pthread > out 2>&1 ; if [ $$? -eq 0 ] ; then $(MAKE) $(MAKECMDGOALS) HAS_NP_FUNCTIONS=yes ; else $(MAKE) $(MAKECMDGOALS) HAS_NP_FUNCTIONS=no ; fi
	@rm -rf testcode.c testcode
endif

# Install Doxygen and Graphviz/dot
# sudo apt-get install graphviz doxygen
docu:
	doxygen ./doxygen

help:
	@echo "Help, use these command for building this project:"
	@echo " - Make debug version              :  make debug"
	@echo " - Make debug version with DVBAPI  :  make debug LIBDVBCSA=yes"
	@echo " - Make PlantUML graph             :  make plantuml"
	@echo " - Make Doxygen docmumentation     :  make docu"
	@echo " - Make Uncrustify Code Beautifier :  make uncrustify"

# Download PlaneUML from http://plantuml.com/download.html
# and put it into the root of the project directory
plantuml:
	java -jar plantuml.jar -tsvg SatPI.plantuml

uncrustify:
	uncrustify -c SatPI.uncrustify --replace $(SOURCES_UNC)
	uncrustify -c SatPI.uncrustify --replace $(HEADERS)
	@echo uncrustify Done

.PHONY:
	clean

clean:
	rm -rf testcode.c testcode ./obj $(EXECUTABLE) src/Version.cpp /web/*.*~
	rm -rf src/*.*~ src/*~
