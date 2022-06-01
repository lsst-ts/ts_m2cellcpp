# Relative directores need to be defined before Makefile.inc is included,
# as the subdirectories have different relative locations.
RELATIVE_DIR ?= ./

include Makefile.inc

.PHONY: all clean deploy tests FORCE doc m2cell

MKDIR_P ?= mkdir -p

# The -MMD and -MP flags help get the .o, .d, and other files in build/.
# Jenkins overwrites CPPFLAGS, so use CPPARGS instead.
CPPARGS ?= $(INC_FLAGS) $(CPP_FLAGS) -MMD -MP -DVERSION="\"$(VERSION)\""

# TODO: DM-35023  Make the executable in the 'serverMain' directory and copy it to bin.
# Having main in the library broke the Catch2 unit tests.

$(LIB_DIR)/libm2cellcpp.a: $(BUILD_DIR)/libm2cellcpp.a
	mkdir -p lib
	mv $(BUILD_DIR)/libm2cellcpp.a $(LIB_DIR)

$(BUILD_DIR)/libm2cellcpp.a: $(OBJS)
	@echo '[AR ] $@'
	${co}$(AR) rs $@ $^

# all is not the default as it will build documentation.
all: run_tests doc m2cell

clean:
	@$(foreach file,doc/html doc/latex,echo '[RM ] ${file}'; $(RM) -r $(file);)
	@$(foreach dir,tests,$(MAKE) -C ${dir} $@;)
	$(RM) -r $(BUILD_DIR) $(LIB_DIR) $(BIN_DIR)

# The tests should stay out of the BUILD_DIR.
tests: $(LIB_DIR)/libm2cellcpp.a tests/Makefile tests/*.cpp
	@${MAKE} -C tests

run_tests: tests
	@${MAKE} -C tests run

junit: tests
	@${MAKE} -C tests junit

doc:
	${co}doxygen Doxyfile

# c++ source. Jenkins fills in CPPFLAGS and CXXFLAGS.
$(BUILD_DIR)/%.cpp.o: %.cpp
	$(MKDIR_P) $(dir $@)
	$(CPP) $(CPPFLAGS) $(CXXFLAGS) $(CPPARGS) -c $< -o $@ $(CPP_LIBS)

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
