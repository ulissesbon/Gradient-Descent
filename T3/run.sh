#!/bin/bash

# Configurações
EXECUCOES=10
VERSOES=("v0_original") # Adicione "v1_unrolling", etc, conforme criar os arquivos

# Resetar ambiente
rm -f results.txt results_brutos.csv
rm -rf original shuffle
mkdir -p original shuffle

# 1. Gerar e Embaralhar Dados
python3 generate_data.py 
python3 shuffle.py 

echo "versao,dataset,tempo,code_sz,data_sz" > results_brutos.csv

for v in "${VERSOES[@]}"; do
    echo "--- Analisando Versão: $v ---"
    
    # 2. Compilar e Medir Memória (Estática)
    # Assume que você renomeou notebook.c para algoritmo_v0_original.c
    gcc -O2 "algoritmo_$v.c" -lm -o temp_main
    
    # Captura tamanhos: text (Flash), data+bss (RAM)
    CODE_SZ=$(size temp_main | awk 'NR==2 {print $1}')
    DATA_SZ=$(size temp_main | awk 'NR==2 {print $2 + $3}')

    # 3. Executar múltiplas vezes para Estatística
    for i in $(seq 1 $EXECUCOES); do
        echo "  Execução $i/$EXECUCOES..."
        # O programa C deve imprimir apenas os tempos no stdout ou podemos ler do results.txt
        ./temp_main > /dev/null
    done
    
    # 4. Extrair dados do results.txt gerado pelo C para o CSV bruto
    # Formato esperado no results.txt: (Notebook) Dataset X: ..., tempo_cpu=0.123
    grep "tempo_cpu" results.txt | tail -n 4 | while read -r line; do
        DS=$(echo $line | sed -E 's/.*Dataset ([0-9]+).*/\1/')
        TIME=$(echo $line | sed -E 's/.*tempo_cpu=([0-9.]+).*/\1/')
        echo "$v,$DS,$TIME,$CODE_SZ,$DATA_SZ" >> results_brutos.csv
    done
    
    rm -f results.txt # Limpa para a próxima versão
done

# 5. Gerar Relatório Final
python3 calculate.py