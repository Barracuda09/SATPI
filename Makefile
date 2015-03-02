###############################################################################
## Makefile for compiling code
###############################################################################

# Set the compiler being used.
CC = $(CCPREFIX)g++

# Includes needed for proper compilation
INCLUDES =

# Libraries needed for linking
LDFLAGS = -lpthread -lrt

# Set Compiler Flags
CFLAGS = -Wall -Wextra -Winit-self $(INCLUDES)

# Build "debug", "release" or simu
ifeq ($(BUILD),debug)
  # "Debug" build - no optimization, with debugging symbols
  #CFLAGS += -O0 -g
  CFLAGS += -O0 -g3 -DDEBUG -fstack-protector-all
else
  ifeq ($(BUILD),simu)
    # "Debug Simu" build - no optimization, with debugging symbols
    CFLAGS += -O0 -g3 -DDEBUG -DSIMU -fstack-protector-all
  else
    # "Release" build - optimization, and no debug symbols
    CFLAGS += -O2 -s -DNDEBUG
  endif
endif

# List of source to be compiled
SOURCES = Version.cpp \
	Satpi.cpp \
	ChannelData.cpp \
	StreamProperties.cpp \
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
	RtcpThread.cpp

# use wildcard expansion when the variable is assigned.
# (otherwise the variable will just contain the string '*.h' instead of the complete file list)
HEADERS = $(wildcard *.h)

#Objectlist is identical to Sourcelist except that all .c extensions need to be replaced by .o extension.
OBJECTS = $(SOURCES:.cpp=.o)

#Resulting Executable name
EXECUTABLE = satpi

# First rule is also the default rule
# generate the Executable from all the objects
# $@ represents the targetname
$(EXECUTABLE): $(OBJECTS)
	$(CC)  $(OBJECTS) -o $@ $(LDFLAGS)

# A pattern rule is used to build objects from 'cpp' files
# $< represents the first item in the dependencies list
# $@ represents the targetname
%.o: %.cpp $(HEADERS)
	$(CC) -c $(CFLAGS) $<

# Create version.cpp
Version.cpp: FORCE
	./version.sh $@ > /dev/null

FORCE:


debug:
	make "BUILD=debug"

simu:
	make "BUILD=simu"

.PHONY: clean
clean:
	rm -rf *.o $(EXECUTABLE) version.cpp *.*~ *~ /web/*.*~
