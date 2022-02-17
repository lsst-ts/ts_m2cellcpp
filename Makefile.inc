# define to "" to produce verbose output
ifndef VERBOSE
  co := @
endif

ifdef DEBUG
  c_opts := -g
else
  c_opts := -O3
endif

# Code coverage by gcovr
ifdef NOGCOVR
  GCOVR_FLAGS := 
else
  GCOVR_FLAGS := -fprofile-arcs -ftest-coverage
endif

# compile x86 simulator or real cRIO stuff
ifdef SIMULATOR
  C := gcc -Wall ${c_opts}
  CPP := g++ -std=c++17 -fPIE -pedantic -pthread -Wall -Wno-vla ${c_opts} -DSIMULATOR $(GCOVR_FLAGS)
else
  C := gcc -Wall -fmessage-length=0 ${c_opts}
  CPP := g++ -std=c++17 -fPIE -pthread -Wall -fmessage-length=0 ${c_opts} $(GCOVR_FLAGS)
endif

LIBS += $(shell pkg-config yaml-cpp --libs) -ldl

BOOST_CPPFLAGS := -I$(CONDA_PREFIX)/include/
CPP_FLAGS += $(shell pkg-config yaml-cpp --cflags) 

VERSION := $(shell git describe --tags --dirty)
ifeq ("$(VERSION)","")
  $(warning "Cannot retrieve version by git, using v0.0.0-None")
  VERSION := "v0.0.0-None"
endif
