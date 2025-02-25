export LC_CTYPE=C

# Configure tools accordingly
# (Note that we don't conditionally set LD)
CXX ?= g++
LD := $(CXX)
AR ?= ar

# Build parameters
PROJECT_NAME := powergb
BASE_DIR := .
BUILD_DIR := $(BASE_DIR)/build
SRC_DIR := $(BASE_DIR)/src
COMMON_SRC_DIR := $(SRC_DIR)/common

MODULES := \
cpu

CXX_SOURCE_TYPE := .cpp
CXX_HEADER_TYPE := .hpp
INT_TYPE := .o
LIB_TYPE := .a
EXE_TYPE :=

# Common Source
COMMON_SRC := $(wildcard $(COMMON_SRC_DIR)/*)

# Generate files
CXX_OBJECTS := $(foreach MODULE,$(MODULES),$(addprefix $(MODULE)., $(addsuffix .cxx$(INT_TYPE), $(notdir $(basename $(wildcard $(SRC_DIR)/$(MODULE)/*$(CXX_SOURCE_TYPE)))))))
OBJECTS := $(foreach OBJECT,$(CXX_OBJECTS),$(addprefix $(BUILD_DIR)/,$(OBJECT)))
LIBRARIES := $(foreach MODULE,$(MODULES),$(addsuffix $(LIB_TYPE),$(addprefix $(BUILD_DIR)/lib$(PROJECT_NAME)_,$(MODULE))))

# Every test should be a standalone file, so the build is simplified
# Also expect none of the built libraries to conflict with each other when all are linked together
TESTS := $(foreach OBJECT,$(notdir $(basename $(wildcard $(SRC_DIR)/test/*$(CXX_SOURCE_TYPE)))),$(addprefix $(BUILD_DIR)/test/,$(OBJECT)))

# Explicit file dependencies
test_ADDITIONAL := $(foreach MODULE,$(MODULES),$(wildcard $(SRC_DIR)/$(MODULE)/**/*$(CXX_HEADER_TYPE)))

# Helper
TOUPPER = $(shell echo '$1' | tr '[:lower:]' '[:upper:]')
FILTER = $(strip $(foreach v,$(2),$(if $(findstring $(1),$(v)),$(v),)))
FILTER_OUT = $(strip $(foreach v,$(2),$(if $(findstring $(1),$(v)),,$(v))))
ESCAPE = $(subst ','\'',$(1))

# Necessary for patsubst expansion
pc := %

# Rules
.PHONY: default clean
.SECONDARY:

default: $(TESTS)
clean:
	rm -r $(BUILD_DIR) || exit 0

# Link executables
# Dependency is the same named object and all libraries
.SECONDEXPANSION:
$(BUILD_DIR)/%$(EXE_TYPE): $(BUILD_DIR)/$$(firstword $$(subst /, ,$$*)).$$(lastword $$(subst /, ,$$*)).cxx$(INT_TYPE) $(LIBRARIES) | $$(patsubst $$(pc)/,$$(pc),$$(dir $$@))
	$(LD) -o $@ $^

# Create library archives for each module
$(BUILD_DIR)/lib$(PROJECT_NAME)_%$(LIB_TYPE): $(filter $*%$(INT_TYPE),$(OBJECTS))
	$(AR) rvs $@ $?

# Build objects & don't delete intermediate files
# Note the '-c <>' is expected to be the last part of the command to be picked up by the generate_compile_commands script
# build/module.X depends on module/X.cpp, and optionally: X.hpp, module/include/*.hpp, common/*, and anything flagged under module_X_ADDITIONAL, module_ADDITIONAL
.SECONDEXPANSION:
$(BUILD_DIR)/%.cxx$(INT_TYPE): $(SRC_DIR)/$$(firstword $$(subst ., ,$$*))/$$(lastword $$(subst ., ,$$*))$(CXX_SOURCE_TYPE) $(wildcard $(SRC_DIR)/$$(firstword $$(subst ., ,$$*))/$$(lastword $$(subst ., ,$$*))$(CXX_HEADER_TYPE)) $$(wildcard $(SRC_DIR)/$$(firstword $$(subst ., ,$$*))/include/*$(CPP_HEADER_TYPE)) $(COMMON_SRC) $$($$(firstword $$(subst ., ,$$*))_ADDITIONAL) $$($$(firstword $$(subst ., ,$$*))_$$(lastword $$(subst ., ,$$*))_ADDITIONAL) | $$(patsubst $$(pc)/,$$(pc),$$(dir $$@))
	$(CXX) -fno-exceptions -std=c++20 -Wall -Wextra $(CXXFLAGS) -I$(SRC_DIR) -o $@ -c $<

#Make directories if necessary
$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/test:
	mkdir -p $@
