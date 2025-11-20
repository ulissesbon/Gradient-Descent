#!/bin/sh
python3 generate_data.py 
rm resultados_finais.txt
gcc -o main main.c
./main
python3 main.py
# python3 view.py
python3 send.py
python3 comparison.py