#!/bin/bash

# Configurações
EXECUCOES=10
VERSOES=("v0_original" "v1_inline" "v2_unrolling" "v3_fixed_point") # Adicione novas aqui

# Resetar ambiente
rm -f results_brutos.csv results.txt

echo "versao,tempo,code_sz,data_sz" > results_brutos.csv

for v in "${VERSOES[@]}"; do
    echo ">>> Testando Versão: $v"
    
    # 1. Compilação
    gcc -O2 "algoritmo_$v.c" -lm -o temp_main
    
    # 2. Medição de Memória Estática (Flash e RAM)
    CODE_SZ=$(size temp_main | awk 'NR==2 {print $1}')
    DATA_SZ=$(size temp_main | awk 'NR==2 {print $2 + $3}')

    # 3. Execuções repetitivas
    rm -f results.txt
    for i in $(seq 1 $EXECUCOES); do
        ./temp_main > /dev/null
    done
    
    # 4. Extração do tempo
    grep "tempo_cpu" results.txt | while read -r line; do
        TIME=$(echo "$line" | sed -E 's/.*tempo_cpu=([0-9.]+).*/\1/')
        echo "$v,$TIME,$CODE_SZ,$DATA_SZ" >> results_brutos.csv
    done
done

python3 calculate.py