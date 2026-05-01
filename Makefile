# ---------------------------------------------------------------------------
# Makefile for the B-Tree implementation
#
# Targets:
#   make         Build main.exe (default target).
#   make run     Build if needed, ensure tmp/ exists, and run main.exe.
#   make clean   Remove main.exe, object files, and tmp/.
# ---------------------------------------------------------------------------

# Compiler and flags.
#   -Wall -Wextra  enable common warnings
#   -std=c++17     required for the class-based implementation
#   -Isrc/...      every directory under src/ becomes a header search path,
CXX      := g++
INC_DIRS := $(shell find src -type d)
CXXFLAGS := -Wall -Wextra -std=c++17 $(addprefix -I,$(INC_DIRS))

# Output binary name.
TARGET   := main.exe

# Recursively collect every .cpp under src/ and subdirectories.
SRC      := $(shell find src -name '*.cpp')
OBJ      := $(SRC:.cpp=.o)

# Runtime directory for the on-disk B-Tree file (tmp/btree.dat).
TMP_DIR  := tmp

# Targets that don't produce a file with the same name.
.PHONY: all run clean

# Default target.
all: $(TARGET)

# Link step: build the final binary from the object files.
$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Pattern rule: compile any .cpp into its matching .o.
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run the program. The order-only prerequisite "| $(TMP_DIR)" ensures tmp/
# exists before execution without forcing a rebuild when its mtime changes.
run: $(TARGET) | $(TMP_DIR)
	./$(TARGET)

# Create the runtime directory on demand.
$(TMP_DIR):
	mkdir -p $(TMP_DIR)

# Remove every build artifact and the runtime directory.
clean:
	rm -f $(TARGET) $(OBJ)
	rm -rf $(TMP_DIR)
