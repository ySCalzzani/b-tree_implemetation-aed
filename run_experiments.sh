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
# [+] Experimento de I/O REAL (opção 2): page cache vs. dispositivo.
# Os tempos da bateria acima são servidos pelo page cache do SO — o .dat de
# N=10^6 (11-36 MB) cabe inteiro na RAM —, então NÃO refletem o disco físico.
# Aqui re-medimos N=10^6 sob um cgroup com MemoryMax baixo (systemd-run --user,
# cgroup v2, sem root), que limita o page cache do PROCESSO e força as
# leituras/escritas a baterem no dispositivo real (NVMe). Compara o modo
# 'cacheado' (sem limite) com 'restrito' (com cgroup). As métricas lógicas
# (reads/writes/bytes/altura) são idênticas entre os modos — só o tempo muda,
# e só o padrão de baixa localidade (Rand) é punido (Seq tem localidade alta).
# -------------------------------------------------------------------------
# MemoryHigh (limite SOFT) estrangula o processo e força eviction do page cache
# perto de 8 MB -> I/O real no dispositivo, SEM OOM-kill (ao contrário de
# MemoryMax, que mata sob pressão de páginas sujas). MemoryMax=64M é só um teto
# rígido de segurança, bem acima do RSS anônimo (~5 MB) do binário.
MEM_HIGH="8M"   # < tamanho de qualquer .dat de N=10^6 (11-36 MB) => força I/O real
IO_FILE="data/resultados_io_real.csv"
if command -v systemd-run >/dev/null 2>&1 && \
   [ "$(stat -fc %T /sys/fs/cgroup 2>/dev/null)" = "cgroup2fs" ]; then
    echo "[+] Experimento de I/O real (cgroup MemoryHigh=$MEM_HIGH): cacheado vs. restrito..."
    echo "M,numKeys,tipo,reads,writes,avg_reads,avg_writes,tempo_total,tempo_cpu,tempo_io,bytes,altura,modo" > "$IO_FILE"
    for m in "${ORDENS[@]}"; do
        for tipo in 1 0; do
            echo "$(./main.exe --experiment "$m" 1000000 "$tipo"),cacheado" >> "$IO_FILE"
            echo "$(systemd-run --user --scope -q -p MemoryHigh="$MEM_HIGH" -p MemoryMax=64M \
                    -p MemorySwapMax=0 ./main.exe --experiment "$m" 1000000 "$tipo" 2>/dev/null),restrito" >> "$IO_FILE"
        done
    done
    echo "I/O real gravado em '$IO_FILE'."
else
    echo "AVISO: systemd-run/cgroup v2 indisponível — pulando experimento de I/O real (opção 2)."
fi

# -------------------------------------------------------------------------
# [5/5] Sanidade: alerta se alguma linha tem bytes=0 (assinatura de falha de
# I/O silenciosa — ex.: diretório tmp/ ausente).
# -------------------------------------------------------------------------
if awk -F, 'NR>1 && $11==0 {bad=1} END{exit !bad}' "$OUTPUT_FILE"; then
    echo "AVISO: há linhas com bytes=0 em $OUTPUT_FILE — possível falha de I/O!"
fi

echo "--------------------------------------------------------"
echo "[5/5] Experimentos concluídos!"