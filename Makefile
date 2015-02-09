###############################################################################
## Makefile for compiling code
###############################################################################

# Set the compiler being used.
CC = $(CCPREFIX)gcc

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
SOURCES = tune.c \
	ssdp.c \
	rtsp.c \
	rtp.c \
	rtcp.c \
	http.c \
	utils.c \
	connection.c \
	applog.c \
	satpi.c

# use wildcard expansion when the variable is assigned.
# (otherwise the variable will just contain the string '*.h' instead of the complete file list)
HEADERS = $(wildcard *.h)

#Objectlist is identical to Sourcelist except that all .c extensions need to be replaced by .o extension.
OBJECTS = $(SOURCES:.c=.o)

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
%.o: %.c $(HEADERS)
	$(CC) -c $(CFLAGS) $<

debug:
	make "BUILD=debug"

simu:
	make "BUILD=simu"

.PHONY: clean
clean:
	rm -rf *.o $(EXECUTABLE) *.*~ *~ /web/*.*~
