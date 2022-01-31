include Makefile.inc

.PHONY: all clean deploy tests FORCE doc main

# Add inputs and outputs from these tool invocations to the build variables 
#

# All Target
all: lib/libm2cellcpp.a

lib/libm2cellcpp.a: FORCE
	$(MAKE) -C src libm2cellcpp.a
	mkdir -p lib
	mv src/libm2cellcpp.a lib

# Other Targets
clean:
	@$(foreach file,doc,echo '[RM ] ${file}'; $(RM) -r $(file);)
	@$(foreach dir,src tests,$(MAKE) -C ${dir} $@;)

tests: tests/Makefile tests/*.cpp
	@${MAKE} -C tests

run_tests: lib/libm2cellcpp.a tests
	@${MAKE} -C tests run


junit: lib/libm2cellcpp.a tests
	@${MAKE} -C tests junit

doc:
	${co}doxygen Doxyfile
