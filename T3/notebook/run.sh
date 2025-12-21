#!/bin/bash
# run.sh

# A estrutura é ARQUIVO:NOME_NO_RELATORIO:FLAG
CONFIGS=(
    "algoritmo_v0_original.c:v0_Original_O2:-O2"
    "algoritmo_v0_original.c:v0_Original_O0:-O0"
    "algoritmo_v1_inline.c:v1_Manual_Inline:-O0"
    "algoritmo_v2_unrolling.c:v2_Manual_Unrolling:-O0"
    "algoritmo_v3_fixedpoint.c:v3_Manual_FixedPoint:-O0"
)

EXECUCOES=10
echo "versao,a,b,mse,tempo" > results.csv # Reset do arquivo central

for cfg in "${CONFIGS[@]}"; do
    IFS=":" read -r ARQUIVO NOME FLAG <<< "$cfg"
    
    # Compila passando o NOME_MELHORIA via terminal para o C usar
    gcc $FLAG "$ARQUIVO" -lm -DNOME_MELHORIA="\"$NOME\"" -o temp_main
    
    echo "Running $NOME with flag $FLAG"
    size temp_main

    for i in $(seq 1 $EXECUCOES); do
        ./temp_main > /dev/null
    done
done

python3 calculate.py