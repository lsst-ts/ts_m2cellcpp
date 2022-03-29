include Makefile.inc

.PHONY: all clean deploy tests FORCE doc main

#&&&TARGET_EXEC ?= a.out

BUILD_DIR ?= ./build
SRC_DIRS ?= ./src

SRCS := $(shell find $(SRC_DIRS) -name *.cpp -or -name *.c -or -name *.s)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CPPFLAGS ?= $(INC_FLAGS) $(CPP_FLAGS) -MMD -MP -DVERSION="\"$(VERSION)\""

lib/libm2cellcpp.a: $(BUILD_DIR)/libm2cellcpp.a
	mkdir -p lib
	mv $(BUILD_DIR)/libm2cellcpp.a lib

#&&& $(BUILD_DIR)/libm2cellcpp.a: $(OBJS)
#&&&	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/libm2cellcpp.a: $(OBJS)
	@echo '[AR ] $@'
	${co}$(AR) rs $@ $^

# assembly
$(BUILD_DIR)/%.s.o: %.s
	$(MKDIR_P) $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

# c source
$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# c++ source
#&&&$(BUILD_DIR)/%.cpp.o: %.cpp
#&&&	$(MKDIR_P) $(dir $@)
#&&&	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.cpp.o: %.cpp
	$(MKDIR_P) $(dir $@)
	$(CPP) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@


.PHONY: clean

clean:
	$(RM) -r $(BUILD_DIR)

-include $(DEPS)

MKDIR_P ?= mkdir -p

