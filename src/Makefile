include ../Makefile.inc

all: libcRIOcpp.a

.PHONY: DEPS clean

CPP_SRCS = $(shell find . -name '*.cpp')

OBJS = $(patsubst %.cpp,%.cpp.o,$(CPP_SRCS))

CPP_DEPS = $(patsubst %.cpp,%.cpp.d,$(CPP_SRCS))

GCOVR_A := $(patsubst %.cpp,%.cpp.gcda,$(CPP_SRCS)) $(patsubst %.cpp,%.cpp.gcno,$(CPP_SRCS))

ifneq ($(MAKECMDGOALS),clean)
  -include ${CPP_DEPS}
endif

CPP_FLAGS += -I. \
	-I"../include" \
	-I"./LSST" \
	-I"." \
	-I"/usr/include" \
	-DVERSION="\"$(VERSION)\""

libm2cellcpp.a: $(OBJS)
	@echo '[AR ] $@'
	${co}$(AR) rs $@ $^

clean:
	@$(foreach file,$(OBJS) $(CPP_DEPS) $(GCOVR_A) libcRIOcpp.a $(shell find -name '*.d'), echo '[RM ] ${file}'; $(RM) -r $(file);)

# file targets
%.cpp.o: %.cpp.d
	@echo '[CPP] $(patsubst %.d,%,$<)'
	${co}$(CPP) $(BOOST_CPPFLAGS) $(CPP_FLAGS) -c -fmessage-length=0 -o $@ $(patsubst %.d,%,$<)

%.cpp.d: %.cpp
	@echo '[DPP] $<'
	@echo ${co}$(CPP) $(BOOST_CPPFLAGS) $(CPP_FLAGS) -M $< -MF $@ -MT '$(patsubst %.cpp,%.o,$<) $@'
	${co}$(CPP) $(BOOST_CPPFLAGS) $(CPP_FLAGS) -M $< -MF $@ -MT '$(patsubst %.cpp,%.o,$<) $@'
