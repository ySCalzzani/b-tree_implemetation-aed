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

echo "M,numKeys,tipo,reads,writes,avg_reads,avg_writes,tempo_total,tempo_cpu,tempo_io,bytes" > $OUTPUT_FILE
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
echo "[4/4] Experimentos concluídos!"