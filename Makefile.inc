# define to "" to produce verbose output
ifndef VERBOSE
  co := @
  silence := --silence-errors
endif

BUILD_DIR ?= $(RELATIVE_DIR)build
SRC_DIRS ?= $(RELATIVE_DIR)src
LIB_DIR ?= $(RELATIVE_DIR)lib
BIN_DIR ?= $(RELATIVE_DIR)bin

SRCS := $(shell find $(SRC_DIRS) -name *.cpp -or -name *.c -or -name *.s)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

# The optimizer is doing somthing at -O3 that causes segfaults when -g is not used.
ifdef DEBUG
  c_opts := -g
else
  c_opts := -O2
endif

# Code coverage by gcovr
ifdef NOGCOVR
  GCOVR_FLAGS :=
else
  GCOVR_FLAGS := -fprofile-arcs -ftest-coverage
endif

# g++ flags used in all g++ calls.
CPP_COMMON := -std=c++17 -fPIE -pthread -Wall ${c_opts} $(GCOVR_FLAGS)
CPP_LIBS += -L/usr/local/lib $(shell pkg-config yaml-cpp --libs) $(shell pkg-config spdlog --libs) -ldl

# compile x86 simulator or real cRIO stuff
ifdef SIMULATOR
  C := gcc -Wall ${c_opts}
  CPP := g++ $(CPP_COMMON) -pedantic -Wno-vla -DSIMULATOR
else
  C := gcc -Wall -fmessage-length=0 ${c_opts}
  CPP := g++ $(CPP_COMMON) -fmessage-length=0
endif


BOOST_CPPFLAGS := -I$(CONDA_PREFIX)/include/
#FUTURE: apply silence to CPP_FLAGS := $(shell pkg-config yaml-cpp --cflags $(silence)) $(shell pkg-config spdlog --cflags $(silence))
CPP_FLAGS += $(shell pkg-config yaml-cpp --cflags)  $(shell pkg-config spdlog --cflags)


VERSION := $(shell git describe --tags --dirty)
ifeq ("$(VERSION)","")
  $(warning "Cannot retrieve version by git, using v0.0.0-None")
  VERSION := "v0.0.0-None"
endif
