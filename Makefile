###############################################################################
## Makefile for compiling code
###############################################################################

# Set the compiler being used.
CXX = $(CXXPREFIX)g++

# Includes needed for proper compilation
INCLUDES =

# Libraries needed for linking
LDFLAGS = -pthread -lrt

# Set Compiler Flags
CFLAGS = -Wall -Wextra -Winit-self -pthread $(INCLUDES)

# Build "debug", "release" or simu
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
	ChannelData.cpp \
	PidTable.cpp \
	StreamProperties.cpp \
	StreamProperties_ChannelData_get.cpp \
	StreamProperties_ChannelData_set.cpp \
	Properties.cpp \
	StringConverter.cpp \
	Log.cpp \
	HttpcSocket.cpp \
	TcpSocket.cpp \
	UdpSocket.cpp \
	Frontend.cpp \
	StreamClient.cpp \
	Streams.cpp \
	Stream.cpp \
	HttpServer.cpp \
	RtspServer.cpp \
	SsdpServer.cpp \
	RtpThread.cpp \
	RtcpThread.cpp \
	XMLSupport.cpp

# Add dvbcsa ?
ifeq ($(LIBDVBCSA),yes)
  LDFLAGS += -ldvbcsa
  CFLAGS  += -DLIBDVBCSA
  SOURCES += DvbapiClient.cpp
endif

# Sub dirs for project
OBJ_DIR = obj
SRC_DIR = src

# use wildcard expansion when the variable is assigned.
# (otherwise the variable will just contain the string '*.h' instead of the complete file list)
HEADERS = $(wildcard $(SRC_DIR)/*.h)

#Objectlist is identical to Sourcelist except that all .c extensions need to be replaced by .o extension.
# and add OBJ_DIR to path
OBJECTS = $(SOURCES:%.cpp=$(OBJ_DIR)/%.o)

#Resulting Executable name
EXECUTABLE = satpi

# First rule is also the default rule
# generate the Executable from all the objects
# $@ represents the targetname
$(EXECUTABLE): makeobj $(OBJECTS) $(HEADERS)
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

debug:
	make "BUILD=debug"

simu:
	make "BUILD=simu"

# Install Doxygen and Graphviz/dot
# sudo apt-get install graphviz doxygen
docu:
	doxygen ./doxygen

# Download PlaneUML from http://plantuml.com/download.html
# and put it into the root of the project directory
plantuml:
	java -jar plantuml.jar -tsvg SatPI.plantuml

.PHONY:
	clean

clean:
	rm -rf ./obj $(EXECUTABLE) src/Version.cpp src/*.*~ src/*~ /web/*.*~
