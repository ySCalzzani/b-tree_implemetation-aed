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
CXXFLAGS := -Wall -Wextra -std=c++17 -Isrc

TARGET   := main.exe

SRC      := $(wildcard src/*.cpp)
OBJ      := $(SRC:.cpp=.o)
HEADERS  := $(wildcard src/*.h)

TMP_DIR  := tmp

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET) | $(TMP_DIR)
	./$(TARGET)

$(TMP_DIR):
	mkdir -p $(TMP_DIR)

clean:
	rm -f $(TARGET) $(OBJ)
	rm -rf $(TMP_DIR)