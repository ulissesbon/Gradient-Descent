#!/bin/sh
python3 gerar.py 
rm resultados_finais.txt
gcc -o main main.c
./main
python3 view.py
