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
#   -Isrc          headers (node.h, printer.h) are resolved from src/
CXX      := g++
CXXFLAGS := -Wall -Wextra -std=c++17 -Isrc

# Output binary name.
TARGET   := main.exe

# Collect every .cpp under src/ (main.cpp, node.cpp, printer.cpp, ...).
# Adding a new .cpp to src/ is picked up automatically — no edit needed here.
SRC      := $(wildcard src/*.cpp)
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
