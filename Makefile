BUILD_DIR ?= ./build
SRC_DIRS ?= ./src
LIB_DIR ?= ./lib

include Makefile.inc

.PHONY: all clean deploy tests FORCE doc main

#&&& BUILD_DIR ?= ./build
#&&& SRC_DIRS ?= ./src
#&&& LIB_DIR ?= ./lib

MKDIR_P ?= mkdir -p

#&&&SRCS := $(shell find $(SRC_DIRS) -name *.cpp -or -name *.c -or -name *.s)
#&&&OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
#&&&DEPS := $(OBJS:.o=.d)

#&&&INC_DIRS := $(shell find $(SRC_DIRS) -type d)
#&&&INC_FLAGS := $(addprefix -I,$(INC_DIRS))

# The -MMD and -MP flags help get the .o, .d, and other files in build/.
CPPFLAGS ?= $(INC_FLAGS) $(CPP_FLAGS) -MMD -MP -DVERSION="\"$(VERSION)\""

# At some point there will be a main that depends on libm2cellcpp.a
#main: $(LIB_DIR)/libm2cellcpp.a
#	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(LIB_DIR)/libm2cellcpp.a: $(BUILD_DIR)/libm2cellcpp.a
	mkdir -p lib
	mv $(BUILD_DIR)/libm2cellcpp.a $(LIB_DIR)

$(BUILD_DIR)/libm2cellcpp.a: $(OBJS)
	@echo '[AR ] $@'
	${co}$(AR) rs $@ $^

# all is not the default.
all: run_tests doc

.PHONY: clean

clean:
	@$(foreach file,doc/html doc/latex,echo '[RM ] ${file}'; $(RM) -r $(file);)
	@$(foreach dir,tests,$(MAKE) -C ${dir} $@;)
	$(RM) -r $(BUILD_DIR) $(LIB_DIR)

# The tests should stay out of the BUILD_DIR.
tests: $(LIB_DIR)/libm2cellcpp.a tests/Makefile tests/*.cpp
	@${MAKE} -C tests

run_tests: tests
	@${MAKE} -C tests run

junit: tests
	@${MAKE} -C tests junit

doc:
	${co}doxygen Doxyfile

# c++ source
$(BUILD_DIR)/%.cpp.o: %.cpp
	$(MKDIR_P) $(dir $@)
	@echo 'src_cppflags= $(CPPFLAGS)'
	@echo 'src_cxxflags= $(CXXFLAGS)'
	$(CPP) $(CPPFLAGS) $(CXXFLAGS) -DHMMSRC -c $< -o $@

# assembly, kept for reference
#$(BUILD_DIR)/%.s.o: %.s
#	$(MKDIR_P) $(dir $@)
#	$(AS) $(ASFLAGS) -c $< -o $@

# c source, kept for reference
#$(BUILD_DIR)/%.c.o: %.c
#	$(MKDIR_P) $(dir $@)
#	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# This helps the compiler out with dependancies.
-include $(DEPS)


