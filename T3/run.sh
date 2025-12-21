#!/bin/bash

EXECUCOES=10
VERSOES=("v0_original") 

# Limpeza inicial absoluta
rm -f results_brutos.csv results.txt

echo "versao,dataset,tempo,code_sz,data_sz" > results_brutos.csv

for v in "${VERSOES[@]}"; do
    echo "--- Analisando Versão: $v ---"
    
    # Compilação
    gcc -O2 "algoritmo_$v.c" -lm -o temp_main
    
    # Medição de Memória : text (Flash), data+bss (RAM)
    CODE_SZ=$(size temp_main | awk 'NR==2 {print $1}')
    DATA_SZ=$(size temp_main | awk 'NR==2 {print $2 + $3}')

    # Limpa o results.txt antes de começar as 10 execuções desta versão
    rm -f results.txt

    for i in $(seq 1 $EXECUCOES); do
        echo "  Execução $i/$EXECUCOES..."
        ./temp_main > /dev/null
    done
    
    # Captura apenas as linhas que contém "tempo_cpu" do arquivo que o C gerou
    # O sed remove caracteres extras para garantir que o Python leia apenas números
    grep "tempo_cpu" results.txt | while read -r line; do
        DS=$(echo "$line" | sed -E 's/.*Dataset ([0-9]+).*/\1/')
        TIME=$(echo "$line" | sed -E 's/.*tempo_cpu=([0-9.]+).*/\1/')
        echo "$v,$DS,$TIME,$CODE_SZ,$DATA_SZ" >> results_brutos.csv
    done
done

python3 calculate.py