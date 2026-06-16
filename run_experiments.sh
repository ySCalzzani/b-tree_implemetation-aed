#!/bin/bash

# -------------------------------------------------------------------------
# run_experiments.sh
# Script para rodar os experimentos do projeto de árvore B.
# -------------------------------------------------------------------------

echo "[1/4] A limpar e compilar o projeto..."
make clean > /dev/null 2>&1
make > /dev/null

if [ $? -ne 0 ]; then
    echo "Erro na compilação!"
    exit 1
fi
echo "Compilação concluída com sucesso!"

echo "[2/4] A configurar o diretório de saída..."
mkdir -p tmp
mkdir -p data
OUTPUT_FILE="data/resultados_experimentos.csv"

echo "M,numKeys,tipo,reads,writes,avg_reads,avg_writes,tempo_total,tempo_cpu,tempo_io,bytes,altura" > $OUTPUT_FILE
echo "Ficheiro de saída '$OUTPUT_FILE' criado com sucesso."

ORDENS=(3 5 10 50 100 500 1000)
CHAVES=(1000 10000 50000 100000 1000000)
TIPOS=(1 0)

echo "[3/4] A iniciar a bateria de testes..."
echo "--------------------------------------------------------"

for m in "${ORDENS[@]}"; do
    for chaves in "${CHAVES[@]}"; do
        for tipo in "${TIPOS[@]}"; do
            
            tipo_nome="Aleatório"
            if [ "$tipo" -eq 1 ]; then
                tipo_nome="Sequencial"
            fi
            
            echo "A rodar teste -> M: $m | Chaves: $chaves | Modo: $tipo_nome..."
            
            ./main.exe --experiment "$m" "$chaves" "$tipo" >> $OUTPUT_FILE
            
        done
    done
done

echo "--------------------------------------------------------"

# -------------------------------------------------------------------------
# [4/5] Comparação de ocupação COM vs SEM reaproveitamento de nós.
# Cada cenário insere N chaves, remove metade e reinsere N/2; mede o tamanho
# final do arquivo com o reuso ligado (1) e desligado (0).
# -------------------------------------------------------------------------
echo "[4/5] A comparar ocupação com e sem reaproveitamento de nós..."
REUSE_FILE="data/reuso_comparacao.csv"
echo "M,numKeys,tipo,reuso,reads,writes,avg_reads,avg_writes,tempo_total,bytes,altura" > $REUSE_FILE

REUSE_ORDENS=(3 10 100)
REUSE_CHAVES=(10000 100000)
for m in "${REUSE_ORDENS[@]}"; do
    for chaves in "${REUSE_CHAVES[@]}"; do
        for reuso in 1 0; do
            ./main.exe --reuse-experiment "$m" "$chaves" 1 "$reuso" >> $REUSE_FILE
        done
    done
done
echo "Comparação de reuso gravada em '$REUSE_FILE'."

# -------------------------------------------------------------------------
# [5/5] Sanidade: alerta se alguma linha tem bytes=0 (assinatura de falha de
# I/O silenciosa — ex.: diretório tmp/ ausente).
# -------------------------------------------------------------------------
if awk -F, 'NR>1 && $11==0 {bad=1} END{exit !bad}' "$OUTPUT_FILE"; then
    echo "AVISO: há linhas com bytes=0 em $OUTPUT_FILE — possível falha de I/O!"
fi

echo "--------------------------------------------------------"
echo "[5/5] Experimentos concluídos!"