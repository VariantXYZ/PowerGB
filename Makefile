export LC_CTYPE=C

# Configure tools accordingly
CXX := c++
LD := $(CXX)
AR := ar
CC_OPT_FLAGS := -flto=thin -O3
LD_OPT_FLAGS := -flto=thin
PYTHON := python3

# Build parameters
PROJECT_NAME := powergb
BASE_DIR := .
BUILD_DIR := $(BASE_DIR)/build
SRC_DIR := $(BASE_DIR)/src
TEST_DIR := $(SRC_DIR)/test
COMMON_SRC_DIR := $(SRC_DIR)/common
SCRIPTS_DIR := $(BASE_DIR)/scripts

MODULES := \
cpu\
memory

CXX_SOURCE_TYPE := .cpp
CXX_HEADER_TYPE := .hpp
INT_TYPE := .o
LIB_TYPE := .a
EXE_TYPE :=

# Helper
TOUPPER = $(shell echo '$1' | tr '[:lower:]' '[:upper:]')
FILTER = $(strip $(foreach v,$(2),$(if $(findstring $(1),$(v)),$(v),)))
FILTER_OUT = $(strip $(foreach v,$(2),$(if $(findstring $(1),$(v)),,$(v))))
ESCAPE = $(subst ','\'',$(1))

# Common Source
COMMON_SRC := $(wildcard $(COMMON_SRC_DIR)/*)

# Generate files
CXX_OBJECTS := $(foreach MODULE,$(MODULES),$(addprefix $(MODULE)., $(addsuffix .cxx$(INT_TYPE), $(notdir $(basename $(wildcard $(SRC_DIR)/$(MODULE)/*$(CXX_SOURCE_TYPE)))))))
OBJECTS := $(foreach OBJECT,$(CXX_OBJECTS),$(addprefix $(BUILD_DIR)/,$(OBJECT)))
LIBRARIES := $(foreach MODULE,$(MODULES),$(addsuffix $(LIB_TYPE),$(addprefix $(BUILD_DIR)/lib$(PROJECT_NAME)_,$(MODULE))))

# Every test should be a standalone file, so the build is simplified
# Also expect none of the built libraries to conflict with each other when all are linked together
TESTS_SM83 := $(foreach OBJECT,$(notdir $(basename $(wildcard $(TEST_DIR)/sm83_*$(CXX_SOURCE_TYPE)))),$(addprefix $(BUILD_DIR)/test/,$(OBJECT)))
TESTS_BASIC := $(filter-out $(TESTS_SM83), $(foreach OBJECT,$(notdir $(basename $(wildcard $(TEST_DIR)/*$(CXX_SOURCE_TYPE)))),$(addprefix $(BUILD_DIR)/test/,$(OBJECT))))
RUN_ALL_TESTS_SM83 := $(foreach TEST,$(TESTS_SM83),$(addprefix run_test_,$(notdir $(basename $(TEST)))))
RUN_ALL_TESTS_BASIC := $(foreach TEST,$(TESTS_BASIC),$(addprefix run_test_,$(notdir $(basename $(TEST)))))

# Explicit file dependencies
test_ADDITIONAL := $(foreach MODULE,$(MODULES),$(wildcard $(SRC_DIR)/$(MODULE)/**/*$(CXX_HEADER_TYPE)))


# Necessary for patsubst expansion
pc := %

# Rules
.PHONY: default clean run_all_tests generate_tests_sm83 run_all_tests_basic run_all_tests_sm83 build_all_tests
.SECONDARY:

default: $(TESTS_BASIC)
build_all_tests: $(TESTS_BASIC) $(TESTS_SM83)
clean:
	rm -r $(BUILD_DIR) || exit 0

run_all_tests: run_all_tests_basic run_all_tests_sm83
	echo "All tests finished"

run_all_tests_basic: $(RUN_ALL_TESTS_BASIC)
	echo "All basic finished"

run_all_tests_sm83: $(RUN_ALL_TESTS_SM83)
	echo "All sm83 finished"

# exec=never is to prevent child processes from being spawned
run_test_%: $(BUILD_DIR)/test/%
	$< --exec=never

# Auto-generate scripts
generate_tests_sm83: $(TEST_DIR_SM83)
	$(PYTHON) $(SCRIPTS_DIR)/generate_tests_sm83.py "$(TEST_DIR)/" "$(SCRIPTS_DIR)/sm83"

# Link executables
# Dependency is the same named object and all libraries
.SECONDEXPANSION:
$(BUILD_DIR)/%$(EXE_TYPE): $(BUILD_DIR)/$$(firstword $$(subst /, ,$$*)).$$(lastword $$(subst /, ,$$*)).cxx$(INT_TYPE) $(LIBRARIES) | $$(patsubst $$(pc)/,$$(pc),$$(dir $$@))
	$(LD) $(LD_OPT_FLAGS) $(LDFLAGS) -o $@ $^

# Create library archives for each module
$(BUILD_DIR)/lib$(PROJECT_NAME)_%$(LIB_TYPE): $(filter $*%$(INT_TYPE),$(OBJECTS))
	$(AR) rvs $@ $?

# Build objects & don't delete intermediate files
# Note the '-c <>' is expected to be the last part of the command to be picked up by the generate_compile_commands script
# build/module.X depends on module/X.cpp, and optionally: X.hpp, module/include/*.hpp, common/*, and anything flagged under module_X_ADDITIONAL, module_ADDITIONAL
.SECONDEXPANSION:
$(BUILD_DIR)/%.cxx$(INT_TYPE): $(SRC_DIR)/$$(firstword $$(subst ., ,$$*))/$$(lastword $$(subst ., ,$$*))$(CXX_SOURCE_TYPE) $(wildcard $(SRC_DIR)/$$(firstword $$(subst ., ,$$*))/$$(lastword $$(subst ., ,$$*))$(CXX_HEADER_TYPE)) $$(wildcard $(SRC_DIR)/$$(firstword $$(subst ., ,$$*))/include/*$(CPP_HEADER_TYPE)) $(COMMON_SRC) $$($$(firstword $$(subst ., ,$$*))_ADDITIONAL) $$($$(firstword $$(subst ., ,$$*))_$$(lastword $$(subst ., ,$$*))_ADDITIONAL) | $$(patsubst $$(pc)/,$$(pc),$$(dir $$@))
	$(CXX) $(CC_OPT_FLAGS) -fno-exceptions -std=c++20 -Wall -Wextra -Werror -Wno-main -fno-exceptions $(CXXFLAGS) -I$(SRC_DIR) -o $@ -c $<

#Make directories if necessary
$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/test:
	mkdir -p $@
