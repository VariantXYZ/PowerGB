export LC_CTYPE=C

# Configure tools accordingly
CXX ?= g++

# Build parameters
BASE_DIR := .
BUILD_DIR := $(BASE_DIR)/build
SRC_DIR := $(BASE_DIR)/src

MODULES := \
state

CXX_SOURCE_TYPE := .cpp
CXX_HEADER_TYPE := .hpp
INT_TYPE := .o
LIB_TYPE := .a
EXE_TYPE :=

# Generate files
CXX_OBJECTS := $(foreach MODULE,$(MODULES),$(addprefix $(MODULE)., $(addsuffix .cxx$(INT_TYPE), $(notdir $(basename $(wildcard $(SRC_DIR)/$(MODULE)/*$(CXX_SOURCE_TYPE)))))))

OBJECTS := $(foreach OBJECT,$(CXX_OBJECTS),$(addprefix $(BUILD_DIR)/,$(OBJECT)))

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

default: $(BUILD_DIR)/state.state.cxx.o
clean:
	rm -r $(BUILD_DIR) || exit 0

# Build objects & don't delete intermediate files
# Note the '-c <>' is expected to be the last part of the command to be picked up by the generate_compile_commands script
.SECONDEXPANSION:
$(BUILD_DIR)/%.cxx$(INT_TYPE): $(SRC_DIR)/$$(firstword $$(subst ., ,$$*))/$$(lastword $$(subst ., ,$$*))$(CXX_SOURCE_TYPE) $(wildcard $(SRC_DIR)/$$(firstword $$(subst ., ,$$*))/$$(lastword $$(subst ., ,$$*))$(CXX_HEADER_TYPE)) $$(wildcard $(SRC_DIR)/$$(firstword $$(subst ., ,$$*))/include/*$(CPP_HEADER_TYPE)) $(COMMON_SRC) $$($$(firstword $$(subst ., ,$$*))_ADDITIONAL) $$($$(firstword $$(subst ., ,$$*))_$$(lastword $$(subst ., ,$$*))_ADDITIONAL) | $$(patsubst $$(pc)/,$$(pc),$$(dir $$@))
	$(CXX) $(CXXFLAGS) -o $@ -c $<

#Make directories if necessary
$(BUILD_DIR):
	mkdir -p $@