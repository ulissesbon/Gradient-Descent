#!/bin/bash
# run.sh

CONFIGS=(
    "algoritmo_v0_original.c:v0_Original_O2:-O2"
    "algoritmo_v0_original.c:v0_Original_O0:-O0"
)

EXECUCOES=10
# Adicionamos colunas de memória no cabeçalho

    




echo "versao,a,b,mse,tempo,flash,ram" > results.csv

for cfg in "${CONFIGS[@]}"; do
    IFS=":" read -r ARQUIVO NOME FLAG <<< "$cfg"
    echo ">>> Processando: $NOME"
    
    # Injetamos o OPTIMIZATION_NAME via GCC
    gcc $FLAG "$ARQUIVO" -lm -DOPTIMIZATION_NAME="\"$NOME\"" -o temp_main
    
    # Captura memória uma vez por versão
    FLASH=$(size temp_main | awk 'NR==2 {print $1}')
    RAM=$(size temp_main | awk 'NR==2 {print $2 + $3}')

    for i in $(seq 1 $EXECUCOES); do
        # Rodamos e anexamos a memória manualmente na linha que o C gerar
        # Como o seu C escreve no arquivo, vamos apenas rodar e depois injetar a memória no CSV
        # Captura a saída do C (que agora é só uma linha de texto)
        RESULTADO=$(./temp_main)
        # Escreve tudo junto no CSV de uma só vez
        echo "$RESULTADO,$FLASH,$RAM" >> results.csv
    done
done

python3 calculate.py