# ---------------------------------------------------------------------------
# Makefile da implementação da árvore B
#
# Alvos:
#   make         Compila main.exe (alvo padrão).
#   make run     Compila se necessário, garante que tmp/ exista e roda main.exe.
#   make clean   Remove main.exe, arquivos-objeto e tmp/.
# ---------------------------------------------------------------------------

# Compilador e flags.
#   -Wall -Wextra  habilita os avisos mais comuns
#   -std=c++17     necessário para a implementação baseada em classes
#   -Isrc/...      cada diretório dentro de src/ vira um caminho de busca de headers,
CXX      := g++
CXXFLAGS := -Wall -Wextra -std=c++17 -Isrc

TARGET   := main.exe

SRC      := $(wildcard src/*.cpp)
OBJ      := $(SRC:.cpp=.o)
HEADERS  := $(wildcard src/*.h) $(wildcard src/display/*.h)

TMP_DIR  := tmp

# Bateria de testes com dados de mock (BTree é header-only, então compila de
# forma independente e nunca linka src/main.o). Rode com `make test`.
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