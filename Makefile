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
HEADERS  := $(wildcard src/*.h) $(wildcard src/display/*.h)

TMP_DIR  := tmp

# Mock-data test harness (header-only BTree, so it builds standalone and never
# links src/main.o). Run with `make test`.
TEST_SRC := tests/test_btree.cpp
TEST_BIN := $(TMP_DIR)/test_btree.exe

.PHONY: all run test clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET) | $(TMP_DIR)
	./$(TARGET)

test: $(TEST_SRC) $(HEADERS) | $(TMP_DIR)
	$(CXX) $(CXXFLAGS) -o $(TEST_BIN) $(TEST_SRC)
	./$(TEST_BIN)

$(TMP_DIR):
	mkdir -p $(TMP_DIR)

clean:
	rm -f $(TARGET) $(OBJ)
	rm -rf $(TMP_DIR)