#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# capturar_sessao.sh
#
# Captura "prints" REAIS do modo interativo (./main.exe) para os slides.
# O modo interativo desenha um "dashboard" com sequencias ANSI (limpa a tela e
# salva/restaura o cursor). Para obter uma tela limpa, capturamos a saida bruta,
# dividimos no codigo de limpar-tela (ESC[2J) e ficamos com o ULTIMO segmento que
# contem o painel RESULTADO — exatamente o que o usuario ve ao final — e entao
# removemos os codigos ANSI restantes.
#
# Gera slides/sessions/*.txt, incluidos nos slides via \lstinputlisting.
# Uso: make sessions  (ou ./slides/capturar_sessao.sh)
# -----------------------------------------------------------------------------
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

OUT="slides/sessions"
DAT="tmp/interactive_btree_m3.dat"
mkdir -p "$OUT" tmp

echo "Compilando main.exe..."
make >/dev/null

# Roda ./main.exe com 'entrada' (linhas de stdin), isola a tela final e a grava
# em $OUT/$saida. Comeca sempre de uma arvore vazia (remove o .dat). O 3o
# argumento controla o recorte: "full" mantem o dashboard inteiro (menu +
# painel); "panel" mantem so o painel RESULTADO (para layouts em duas colunas).
capturar() {
    local saida="$1" entrada="$2" modo="${3:-full}"
    rm -f "$DAT"
    printf '%s' "$entrada" | timeout 15 ./main.exe > tmp/raw_sessao.txt 2>/dev/null || true
    MODO="$modo" perl -0777 -ne '
        my @seg = split /\e\[2J/;                 # divide nas limpezas de tela
        my @res = grep { /RESULTADO/ } @seg;       # so paineis com resultado
        my $s = @res ? $res[-1] : $seg[-1];        # o ultimo (tela final)
        $s =~ s/\e\[[0-9;?]*[A-Za-z]//g;           # remove ANSI restante
        if ($ENV{MODO} eq "panel") {               # recorta so a caixa RESULTADO
            $s =~ /(\+=+ RESULTADO =+\+.*?\+=+\+)/s and $s = $1;
        }
        $s =~ s/\A\s*\n//;                          # tira linhas em branco do topo
        $s =~ s/\s+\z/\n/;                          # normaliza o fim
        print $s;
    ' tmp/raw_sessao.txt > "$OUT/$saida"
    echo "  ok: $OUT/$saida"
}

# Sequencia: ordem=3; insere 30 20 40 55 10 35 50; (4) imprime arvore; (0) sai.
# Dashboard completo: mostra a "cara" da interface (menu + painel).
capturar "sessao_demo.txt" \
    $'3\n1\n30\n1\n20\n1\n40\n1\n55\n1\n10\n1\n35\n1\n50\n4\n0\n' full

# Mesma carga, terminando em (5) Estatisticas de I/O — so o painel.
capturar "sessao_stats.txt" \
    $'3\n1\n30\n1\n20\n1\n40\n1\n55\n1\n10\n1\n35\n1\n50\n5\n0\n' panel

# Arvore pequena (5 chaves), terminando em (6) Dump bruto do .dat — so o painel.
capturar "sessao_dump.txt" \
    $'3\n1\n30\n1\n20\n1\n40\n1\n55\n1\n10\n6\n0\n' panel

rm -f tmp/raw_sessao.txt
echo "Sessoes capturadas em $OUT/."
