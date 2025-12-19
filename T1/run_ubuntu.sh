#!/bin/sh
python3 generate_data.py 
rm resultados_finais.txt
gcc -o main autoral.c
./main
python3 padrao_ouro.py
# python3 view.py
python3 comparison.py