# ---------------------------------------------------------------------------
# Makefile da implementação da árvore B
#
# Alvos:
#   make                 Compila main.exe (alvo padrão).
#   make run             Compila se necessário, garante que tmp/ exista e roda main.exe.
#   make run_experiments Roda a bateria de experimentos (run_experiments.sh).
#   make graficos        Gera as figuras da analise (slides/img/*.png).
#   make sessions        Captura prints reais da interface (slides/sessions/*.txt).
#   make slides-assets   = graficos + sessions (tudo que o deck inclui).
#   make slides          Gera slides/apresentacao.pdf com pdflatex (Beamer).
#   make clean           Remove main.exe, arquivos-objeto, tmp/ e artefatos LaTeX.
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

# Slides Beamer, compilados com um TeX Live instalado LOCALMENTE no projeto
# (./.texlive/, gerado por `make tex-install`). NAO depende de mise nem de um
# TeX de sistema. Duas passadas resolvem TOC/navegacao.
SLIDES_DIR := slides
SLIDES_SRC := apresentacao.tex
TEX_DIR    := .texlive
# pdflatex local (caminho absoluto p/ funcionar apos 'cd slides'); cai para o
# pdflatex do PATH apenas se a instalacao local ainda nao existir.
TEX_LOCAL  := $(abspath $(firstword $(wildcard $(TEX_DIR)/bin/*/pdflatex)))
LATEX      := $(if $(TEX_LOCAL),$(TEX_LOCAL),pdflatex)
LATEXFLAGS := -interaction=nonstopmode -halt-on-error

# Geracao dos artefatos do deck. As figuras usam so matplotlib+numpy (sem
# pandas/seaborn), entao roda no .venv do projeto; cai para python3 se nao houver.
PYTHON     := $(shell [ -x .venv/bin/python ] && echo .venv/bin/python || echo python3)
FIG_SCRIPT := analysis/gerar_graficos.py
SESS_SCRIPT := slides/capturar_sessao.sh

.PHONY: all run run_experiments test graficos sessions slides-assets tex-install slides slides-clean clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET) | $(TMP_DIR)
	./$(TARGET)

# A própria run_experiments.sh já faz `make clean && make`, então o alvo apenas
# delega para o script (sem depender de $(TARGET) para não recompilar à toa).
run_experiments:
	./run_experiments.sh

test: $(TEST_SRC) $(HEADERS) | $(TMP_DIR)
	$(CXX) $(CXXFLAGS) -o $(TEST_BIN) $(TEST_SRC)
	./$(TEST_BIN)

# Gera as figuras da analise experimental (slides/img/*.png).
graficos:
	$(PYTHON) $(FIG_SCRIPT)

# Captura os prints reais da interface interativa (slides/sessions/*.txt).
# O proprio script compila main.exe antes de capturar.
sessions:
	bash $(SESS_SCRIPT)

# Regenera tudo que o deck inclui (figuras + sessoes). Rode quando os dados ou a
# interface mudarem; os arquivos gerados sao versionados (como o .pdf).
slides-assets: graficos sessions

# Instala um TeX Live minimo (TinyTeX) LOCALMENTE em ./.texlive, sem mise e sem
# root. Rode uma vez; depois `make slides` usa esse TeX. (./.texlive e' ignorado
# pelo git -- e' dependencia de build, nao codigo-fonte.)
tex-install:
	@if [ -n "$(TEX_LOCAL)" ]; then echo "TeX local ja existe em $(TEX_DIR)/."; exit 0; fi
	@echo "Instalando TinyTeX em $(TEX_DIR)/ ..."
	curl -fsSL https://yihui.org/tinytex/install-bin-unix.sh | sh -s - --no-path
	mv "$(HOME)/.TinyTeX" "$(TEX_DIR)"
	"$(TEX_DIR)"/bin/*/tlmgr install beamer translator pgf listings booktabs \
		babel-portuguese xcolor graphics
	@echo "TeX local instalado em $(TEX_DIR)/."

# Gera slides/apresentacao.pdf usando o pdflatex LOCAL do projeto (./.texlive).
# Se ainda nao instalado, rode `make tex-install`.
# Usa as figuras/sessoes ja versionadas; rode `make slides-assets` p/ regenera-las.
slides:
	@$(LATEX) --version >/dev/null 2>&1 || { \
		echo "ERRO: pdflatex nao encontrado ($(LATEX))."; \
		echo "Rode 'make tex-install' p/ instalar o TeX local em $(TEX_DIR)/."; \
		exit 1; }
	cd $(SLIDES_DIR) && $(LATEX) $(LATEXFLAGS) $(SLIDES_SRC)
	cd $(SLIDES_DIR) && $(LATEX) $(LATEXFLAGS) $(SLIDES_SRC)
	@echo "Slides gerados em $(SLIDES_DIR)/$(SLIDES_SRC:.tex=.pdf)"

# Remove os artefatos intermediarios do LaTeX (mantem o .pdf).
slides-clean:
	rm -f $(addprefix $(SLIDES_DIR)/apresentacao., aux log nav out toc snm vrb)

$(TMP_DIR):
	mkdir -p $(TMP_DIR)

clean: slides-clean
	rm -f $(TARGET) $(OBJ)
	rm -rf $(TMP_DIR)