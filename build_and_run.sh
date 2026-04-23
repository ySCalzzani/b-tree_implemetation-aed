#!/bin/bash
set -e

mkdir -p tmp
g++ -Wall -o main.exe main.cpp $(find src -name '*.cpp' 2>/dev/null)
./main.exe
