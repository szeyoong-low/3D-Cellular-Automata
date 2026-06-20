# Adapted from Luca Vallin's Makefile template
# https://www.lucavall.in/blog/crafting-clean-maintainable-understandable-makefile-for-c-project

# ":=" means simple assignment, so the right-hand-side is evaluated exactly once
# Read more at https://stackoverflow.com/questions/4879592/whats-the-difference-between-and-in-makefile

# Directory to store intermediate build files (e.g. *.o, *.a)
BUILD_DIR := build

# Directory to store final executables (build targets, compiled and linked)
BIN_DIR := bin

# Filenames of final executables (build targets)
EXT_TARGET := cellular_automata

# Main entry points of programmes (main function)
EXT_MAIN := $(EXT_TARGET).c

# Main source files post-compilation, pre-linking
EXT_MAIN_OBJ := $(BUILD_DIR)/$(EXT_TARGET).o

# Directories containing source files (.c)
EXT_SRC_DIR := src

# Directories containing header files (.h)
EXT_INC_DIR := include

# Directories containing shaders (.glsl)
EXT_SHADER_DIR := shaders

# Directory containing custom cell files
CELL_SRC_DIR := custom_cell

# Finds all source files
# Note that this pattern only matches source files 1 level deep 
# For more information on wildcards: https://www.gnu.org/software/make/manual/html_node/Wildcard-Function.html
EXT_SRC_FILES := $(wildcard $(EXT_SRC_DIR)/*.c)

# Find all custom cell C source files
CELL_SRC_FILES := $(wildcard $(CELL_SRC_DIR)/*.c)

# Finds all header files
EXT_HEADERS := $(wildcard $(EXT_INC_DIR)/*.h)

# Finds all header files
EXT_SHADERS := $(wildcard $(EXT_SHADER_DIR)/*.glsl)

# Third-party generated sources — compiled but excluded from linting/formatting
THIRD_PARTY_SRCS := $(EXT_SRC_DIR)/glad.c

# All first-party files for linting and formatting
CODEBASE := $(EXT_MAIN) $(filter-out $(THIRD_PARTY_SRCS),$(EXT_SRC_FILES)) $(EXT_HEADERS) $(CELL_SRC_FILES)

# Maps files e.g. 'emulate_src/foo.c' to 'build/emulate_src/foo.o'
# See https://www.gnu.org/software/make/manual/make.html#Text-Functions
# Assumes directory structure in build directory mirrors src directory (flat
# for now).
EXT_OBJS := $(patsubst %.c, $(BUILD_DIR)/%.o, $(EXT_SRC_FILES))
ALL_OBJS := $(EXT_MAIN_OBJ) $(EXT_OBJS)

# Maps 'custom_cell/foo.c' to 'bin/foo.so'
CELL_SOS := $(patsubst $(CELL_SRC_DIR)/%.c, $(BIN_DIR)/%.so, $(CELL_SRC_FILES))

# Compiler flags
# -g: Include debugging information
# -Og: Optimise while keeping in mind debugging experience
# D_POSIX_SOURCE: enable POSIX extension functions in the standard headers
# D_DEFAULT_SOURCE: enable GNU extension functions in the standard headers
# Werror: treat warnings as errors
# Wstric-prototypes: functions that take no arguments must have void as a
# 					 parameter in their prototype
# Wconversion: implicit conversions that may alter a value (e.g. real and integer
#			   signed and unsigned, to smaller types)
# Wpedantic: all warnings demanded by strict ISO C, reject forbidden extensions
# -I<directory>: search for header files in this directory
CC      := gcc
CFLAGS  := -std=c18 -O3\
	-D_POSIX_SOURCE -D_DEFAULT_SOURCE\
	-Wall -Wextra -Werror -Wstrict-prototypes -Wconversion -Wpedantic\
	-I$(EXT_INC_DIR)
# Linker flags for external libraries
# -lglfw: GLFW window and input library
# -lGL: system OpenGL (Mesa on Linux/WSL)
# -lm: C math library, required by cglm
LDFLAGS := -lglfw -lGL -lm

LINTER	:= clang-tidy
FORMATTER	:= clang-format

# Phony targets don't correspond to actual files. They will always execute
# even if a file of the same name exists
.PHONY: all clean lint format build bear cells

all: lint format build

build: $(EXT_TARGET) cells

# The format of a Makefile rule is:
# <target>: <dependencies>...
# 	<command> << offset by TABS (not spaces)

# Automatic variables:
# $@ is the target name
# $< is the first listed dependency
# $^ is all listed dependencies
# GNU docs: https://www.gnu.org/software/make/manual/html_node/Automatic-Variables.html#Automatic-Variables
# Stack Overflow: https://stackoverflow.com/questions/3220277/what-do-the-makefile-symbols-and-mean

# Linking of object files into final executable
# -o: output destination
$(EXT_TARGET): $(EXT_MAIN_OBJ) $(EXT_OBJS)
	@mkdir -p $(BIN_DIR)
	@$(CC) $^ $(LDFLAGS) -o $(BIN_DIR)/$@
	@printf "\nExtension built\n"

# % is a wildcard that matches any number of characters. This will be passed to
# the dependency on the right-hand side.
# See https://www.gnu.org/software/make/manual/make.html#Text-Functions

# glad is generated third-party code — compile without strict warning flags
$(BUILD_DIR)/$(EXT_SRC_DIR)/glad.o: $(EXT_SRC_DIR)/glad.c
	@mkdir -p $(@D)
	@$(CC) -std=c18 -g -Og -I$(EXT_INC_DIR) -c $< -o $@

# Compiles any source file. Assumes build and src directories have the same
# structure. This structure is captured by the % wildcard.
# $(@D) gives the directory part of the target (e.g. dir/foo.o -> dir)
# -c: Compile but do not link
# -p: No-op if directory already exists
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c $< -o $@

cells: $(CELL_SOS)

# -shared tells GCC to build a library
# -fPIC tells GCC to make the code position independent
# which allows for dynamic loading
$(BIN_DIR)/%.so: $(CELL_SRC_DIR)/%.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -shared -fPIC $< -o $@

# -header-filter=.*: Display errors from all non-system headers
# @: Don't echo this command on the output
lint:
	@$(LINTER) -checks=-*,bugprone-*,clang-analyzer-*,llvm-*,\
	performance-*,portability-*,readability-* -header-filter=.*\
	$(CODEBASE) -- $(CFLAGS)
	@printf "\nLinting complete\n"

# -i: Apply all changes identified
format:
	@$(FORMATTER) -style=llvm -i $(CODEBASE) $(EXT_SHADERS)
	@printf "\nFormatting complete\n"

clean:
	@rm -rf $(BUILD_DIR) $(BIN_DIR)
	@printf "\nCleaning complete\n"

# Lifesaver if you're using the Clangd language server on your IDE. Generates
# a `compile_commands.json` config file.
# Run this manually, otherwise it'll cause a redundant build. Build must be
# from scratch, otherwise the config file generated will be empty.
# --output places the file in project root, as required by Clangd
# -- separates flags for bear with the command it should run
bear:
	@bear --output compile_commands.json -- make clean build > /dev/null
	@printf "\nUpdated Clangd configurations\n"
