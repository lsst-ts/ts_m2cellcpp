# define to "" to produce verbose output
ifndef VERBOSE
  co := @
endif

ifdef DEBUG
  c_opts := -g
else
  c_opts := -O3
endif

# compile x86 simulator or real cRIO stuff
ifdef SIMULATOR
  C := gcc -Wall ${c_opts}
  CPP := g++ -std=c++17 -fPIE -pedantic -pthread -Wall -Wno-vla ${c_opts} -DSIMULATOR
else
  C := gcc -Wall -fmessage-length=0 ${c_opts}
  CPP := g++ -std=c++17 -fPIE -pthread -Wall -fmessage-length=0 ${c_opts}
endif

#LIBS += $(shell pkg-config yaml-cpp --libs) -ldl -lpthread
LIBS += $(shell pkg-config yaml-cpp --libs) -ldl

#BOOST_CPPFLAGS := -I/usr/include/boost169
BOOST_CPPFLAGS := -I$(CONDA_PREFIX)/include/
CPP_FLAGS += $(shell pkg-config yaml-cpp --cflags)

VERSION := $(shell git describe --tags --dirty)
ifeq ("$(VERSION)","")
  $(warning "Cannot retrieve version by git, using v0.0.0-None")
  VERSION := "v0.0.0-None"
endif
