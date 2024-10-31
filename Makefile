#OS_DEFS SOEXE,SOFLAGS,DEFS
PACKAGE=$(shell basename `pwd`)
UNAME=$(shell uname)

ifeq ($(UNAME), Linux)
SOFLAGS  = -shared
DEFS     = -DOS_LINUX
LIB_EXT := .so
endif

ifeq ($(UNAME), Darwin)
SOFLAGS  = -dynamiclib
DEFS     = -DOS_DARWIN
LIB_EXT := .dylib
endif

CXX	      = g++ -std=c++11
CC        = gcc
ROOTCINT  = $(ROOTSYS)/bin/rootcint
DEFS	 += -DCERNLIB_TYPE -DCERNLIB_LINUX -DCERNLIB_UNIX -DHEP_SHORT_NAMES

CXX_SOURCES = $(wildcard src/*.cc)
CXX_DICT    = $(wildcard include/*LinkDef.h)
CXX_DICTSRC = $(CXX_DICT:include/%LinkDef.h=src/%Dict.cxx)
CXX_OBJECTS = $(filter-out %Dict.o,$(CXX_SOURCES:src/%.cc=tmp/%.o))
CXX_DICTOBJ = $(CXX_DICTSRC:src/%.cxx=tmp/*.o)

CXXFLAGS  = -fPIC -Wall
CXXFLAGS += $(shell $(ROOTSYS)/bin/root-config --cflags)
CXXFLAGS += -O2 -g -fmessage-length=0 -fpermissive

CC_SOURCES = $(wildcard src/*.cc)
CC_DICT    = $(wildcard include/*LinkDef.h)
CC_DICTSRC = $(CXX_DICT:include/%LinkDef.h=src/%Dict.cxx)
CC_OBJECTS = $(filter-out %Dict.o,$(CC_SOURCES:src/%.cc=tmp/%.o))
CC_DICTOBJ = $(CC_DICTSRC:src/%.cxx=tmp/*.o)

CCFLAGS  = -fPIC -Wall
CCFLAGS += $(shell $(ROOTSYS)/bin/root-config --cflags)
CCFLAGS += -O2 -g -fmessage-length=0 -fpermissive

LIBS += $(shell $(ROOTSYS)/bin/root-config --glibs) -lEG
LIBS += -L/usr/lib64/ -lusb-1.0
LIBS += -L./lib
LIBS += -L$(NKHOME)/lib -lNoticeMINITCB_V2ROOT -lNKUSBROOT -lusb3comroot -lNoticeNKFADC500ROOT

INCLUDEDIR += .  ./include
INCLUDEDIR += $(ROOTSYS)/include
INCLUDEDIR += $(NKHOME)/include

ifeq ($(UNAME), Linux)
INCLUDEDIR += /usr/include/libusb-1.0
endif

ifeq ($(UNAME), Darwin)
INCLUDEDIR += /usr/local/Cellar/libusb/1.0.20/include/libusb-1.0
endif

INCLUDES     = $(INCLUDEDIR:%=-I%)
COMPILE_CC  := $(CC)  -c $(DEFS) $(INCLUDES) $(CFLAGS)
COMPILE_CXX := $(CXX) -c $(DEFS) $(INCLUDES) $(CXXFLAGS)
DEPEND_CC   := $(CC) -MM $(DEFS) $(INCLUDES) $(filter-out -fPIC, $(CFLAGS))
DEPEND_CXX  := $(CC) -MM $(DEFS) $(INCLUDES) $(filter-out -fPIC, $(CXXFLAGS))
LINK_CC     := $(CC)
LINK_CXX    := $(CXX)

TARGET_SRC = $(wildcard *.cc)
TARGET_BIN = $(TARGET_SRC:%.cc=bin/%)
#TARGET_OBJS = $(TARGET_SRC:%.cc=tmp/%.o)

all: rootDict makelib target

target: $(TARGET_BIN)
	@ basename `pwd`

makelib: $(CXX_OBJECTS)
	@ echo $(ROOTSYS)
	@ echo $(CXX_OBJECTS)
	@ [ -d ./lib ] || mkdir -p ./lib
	$(CXX) $(SOFLAGS) -o lib/lib$(PACKAGE)$(LIB_EXT) \
	$(CXX_OBJECTS) $(CXX_DICTSRC) $(CXXFLAGS) $(CC_OBJECTS) $(INCLUDES) $(LIBS)

rootDict:
ifneq "$(shell ls ./include/*LinkDef.h 2>/dev/null)" ""
	@ echo making rootdicts
	@ for i in `ls ./include/*LinkDef.h | sed -n "s/\(.*\)LinkDef.h/\1.h/p"`;do \
	j=`basename $$i .h`; \
	$(ROOTCINT) -f src/$${j}Dict.cxx -c $(INCLUDES) ./include/$${j}.h ./include/$${j}LinkDef.h; \
	echo $$i; \
	done;
	@ echo End making rootdicts
endif

bin/%: tmp/%.o $(CXX_OBJECTS)
	@ [ -d ./bin ] || mkdir -p ./bin
	@ echo ""
	@ echo COMPILE $(@F)
	@ echo "##########################################################"
	$(CXX) -o $@ $^ $(filter-out -fPIC,$(CXXFLAGS)) $(CXX_DICTSRC) $(INCLUDES) $(LIBS)

tmp/%.o: %.cc
	@ echo $(INCLUDEDIR)
	@ [ -d ./tmp ] || mkdir -p ./tmp
	@ echo making $(@F)
	$(COMPILE_CXX) $< -o $@

.PHONY: clean
clean:
	rm -f ./lib/*
	rm -f ./tmp/*.d
	rm -f ./tmp/*.o
	rm -f ./src/*Dict.*
	rm -r ./lib ./tmp
