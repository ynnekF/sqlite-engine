# GNU make knows how to execute several recipes at once. Normally, make will execute only one
# recipe at a time, waiting for it to finish before executing the next. However, the ‘-j’ or
# ‘--jobs’ option tells make to execute many recipes simultaneously. Not defining the option
# SINGLE_CPU_EXEC will allow this makefile to use the maximum number of physical CPUs.
# See link for more info (https://www.gnu.org/software/make/manual/html_node/Parallel.html)
ifndef SINGLE_CPU_EXEC
    NCPU 		?= $(shell (nproc --all || sysctl -n hw.physicalcpu) 2>/dev/null || echo 1)
    MAKEFLAGS	+= --jobs=$(NCPU)
endif

ifneq ($(firstword $(shell $(firstword $(MAKE)) --version)),GNU)
$(error GNU make is not installed.)
endif

# Path to executable `gcc` (/usr/bin/gcc). MUST be installed prior to using this makefile.
# (brew install gcc, apt-get install -y gcc, etc.)
CC=$(shell which gcc)

# Directory Structure Options
CWD=$(shell pwd)

# Directory with source files
SRC_DIR=src

# Directory with header files
INC_DIR=include

# Directory for compiled output. This directory's format should match that of the src 
# directory but where the *.c files are, this wll have object files and a top-level exe.
BIN_DIR=bin

# Similar to `FOLDERS` below but this is used when creating the compilation targets
DIRS=$(sort $(SRC_DIR))

# Source file suffix
CEXT=c

# Header file suffix
HEXT=h

#
# Compile Time Options
#
# Here we include any libraries we want to link, prefixed with "-l". The -l option (-l<library>)
# is passed directly to the linker by GCC to search standard libraries and any specified by "-L".
LIBS=

# Library paths specified by "-L/path/to/lib"
LDFLAGS= -g

# C Compiler flags
CFLAGS= -g

# Enforce executable is recompiled when header files are changed. When used with -M or -MM,
# specifies a file to write the dependencies to. If no -MF switch is given the preprocessor
# sends the rules to the same place it would send preprocessed output. When used with the
# driver options -MD or -MMD, -MF overrides the default dependency output file.
C_DEPS=-MMD -MF $(@:.o=.d)

# Preprocessor macro definitions. These values will be predefined on each
# source file before compilation. Currently there are two optional macros.
# ex. -DUNIT_TEST -DNO_COLOR
C_DEFINES=

# Extra preprocessor macro definitions not computed by below logic. Can be
# populated here or during make (i.e., make C_EXTRA_DEFINES=-DNO_COLOR run)
C_EXTRA_DEFINES=

# List of directories and libraries to be searched for header files.
# The files under include will be searched before anything else.
# ex. -I/opt/homebrew/include/
C_INCLUDES=-I $(INC_DIR) $(LIBS)

# Directories to search for targets
FOLDERS=$(SRC_DIR) $(INC_DIR)
MAX_DEPTH=1

ifeq ($(OS),Windows_NT)
    OS = windows
else
    KERNEL=$(shell uname -s)
	ifeq ($(KERNEL),Darwin)
        OS = macos
	else ifeq ($(KERNEL),Linux)
        OS = linux
	else
$(error unsupported OS)
	endif
endif

ifeq ($(OS),$(filter $(OS), macos linux))
    # Assign linker options for shared/std libraries. These paths are passed directly
    # to the linker to search along with the standard system libraries.
    ifeq ($(UNAME_P),i386)
        C_INCLUDES += -I/usr/local/include/ -L/usr/local/lib
	
	else ifeq ($(UNAME_M),aarch64)
        C_INCLUDES += -I/usr/lib -I/usr/include

	else ifeq ($(UNAME_P),arm)
        C_INCLUDES += -I/opt/homebrew/include/ -L/opt/homebrew/lib
    endif

    # Source file query patterns
    t_pattern := $(TP_DIR)/%.$(CEXT)
    c_pattern := $(SRC_DIR)/%.$(CEXT)
    o_pattern := $(BIN_DIR)/obj/%.o
    
    # All source files (*.c) within the computed folders
    SOURCE_FILES := $(patsubst \
					$(c_pattern),\
					$(o_pattern),\
					$(shell find $(FOLDERS) -iname *.$(CEXT)))

    SOURCE_FILES := $(patsubst $(t_pattern), $(o_pattern), $(SOURCE_FILES))

    # All header files (*h) within the computed folders (should only match
    # header files in the include directory.)
    HEADER_FILES := $(shell find $(FOLDERS) -maxdepth $(MAX_DEPTH) -iname *.$(HEXT))

    # All files (*.c and *.h) for clang-format
    FORMAT_FILES := $(shell find $(FOLDERS) -iname *.$(CEXT) -o -iname *.$(HEXT))
endif

define log
[ "$<" ] && \
	( printf "Makelog %-30s %-30s %s\n" "$@" "$<" "$(1)" )\
	|| ( printf "Makelog %-10s %s\n" "$@" "$(1)" )
endef

# C_WARNINGS= -Wno-int-conversion -Wno-pointer-to-int-cast

# Finalize compile/link options
LDFLAGS+=$(C_WARNINGS)
LDFLAGS+=$(C_DEFINES)
LDFLAGS+=$(C_INCLUDES)
LDFLAGS+=$(EXTRA_C_DEFINES)
TARGET=boilerplate

args		:= `arg="$(filter-out $@,$(MAKECMDGOALS))" && echo $${arg:-${1}}`
fmt_file  	:= --style=file:.clang-format --verbose

$(VERBOSE).SILENT:
# "Phony" targets are not files. They are just names for commands.
.PHONY: all list config clean run format

# Build the target
all: $(BIN_DIR)/$(TARGET)

# Below is the "template" for defining our targets to compile object files. Since we 
# have two "sources", the src directory and the unit test directory we can evaluate the 
# result of the below foreach call to generate a target for each directory.
#
# During the secondary expansion of explicit rules, $$@ and $$% evaluate, respectively,
# to the file name of the target and, when the target is an archive member, the target
# member name. The $$< variable evaluates to the first prerequisite in the first rule
# for this target. $$^ and $$+ evaluate to the list of all prerequisites of rules that
# have already appeared for the same target ($$+ with repetitions and $$^ without).
define compiler_target_template
    $(BIN_DIR)/obj/%.o: $(1)/%.$(CEXT) $(HEADER_FILES) | $(BIN_DIR)
		$$(call log,compiling)
		$$(eval obj_dir := $$(shell dirname $$@))
		mkdir -p $$(obj_dir) 2> /dev/null
		$$(CC) $(CFLAGS) -c -o $$@ $$< $$(LDFLAGS)
		$$(call log,compiled object file $$@)
endef

# Build compiler targets
$(eval $(foreach f,$(DIRS),$(eval $(call compiler_target_template,$(f)))))

# Link object files
$(BIN_DIR)/$(TARGET): $(SOURCE_FILES)
    # Surface assembled machine code.
	$(eval BINS	:= $(shell find $(BIN_DIR) -name '*.o'))

    # Link files + shared libraries
	$(CC) -o $@ $(BINS) $(LDFLAGS)
	$(call log,built executable $@)

# Create the bin object directory.
$(BIN_DIR):
	mkdir -p $(BIN_DIR)/obj

DEFAULT_DB ?= mydb.db

# Run built executables with the given args.
run: $(BIN_DIR)/$(TARGET)
	$(call log,)
	$(BIN_DIR)/$(TARGET) $(DEFAULT_DB) $(args)

clean:
	-@$(RM) -rf ${BIN_DIR}
	-@$(RM) $(DEFAULT_DB)

# Execute `clang-format` against all source files
format:
	clang-format $(fmt_file) -i $(FORMAT_FILES)

# List all target names
list:
	@LC_ALL=C $(MAKE) -pRrq -f \
	$(firstword $(MAKEFILE_LIST)) : 2>/dev/null \
	| awk -v RS= -F: '/(^|\n)# Files(\n|$$)/,/(^|\n)# Finished Make data base/ {if ($$1 !~ "^[#.]") {print $$1}}' \
	| sort | grep -E -v -e '^[^[:alnum:]]' -e '^$@$$'

tests: clean
	rspec --format documentation 

# This prevents Make from trying to run nonexistent targets named after your args
%:
	@: