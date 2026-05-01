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

# Python tooling for the analysis dashboard.
VENV     := .venv
PYTHON   := $(VENV)/bin/python
PIP      := $(VENV)/bin/pip
PY_DIR   := src/python
REQS     := requirements.txt

# Targets that don't produce a file with the same name.
.PHONY: all run clean venv dashboard clean-venv

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

# Create the venv and install pinned deps. The stamp file is touched on a
# successful install so re-running `make venv` is a no-op until requirements
# change.
$(VENV)/.installed: $(REQS)
	python3 -m venv $(VENV)
	$(PIP) install --upgrade pip
	$(PIP) install -r $(REQS)
	touch $@

venv: $(VENV)/.installed

# Render the analysis dashboard. Requires that the binary has been run at
# least once so tmp/btree.dat exists.
dashboard: $(TARGET) venv | $(TMP_DIR)
	@if [ ! -f $(TMP_DIR)/btree.dat ]; then \
		echo "Run ./main.exe first to populate $(TMP_DIR)/btree.dat"; \
		exit 1; \
	fi
	$(PYTHON) $(PY_DIR)/dashboard.py

clean-venv:
	rm -rf $(VENV)
