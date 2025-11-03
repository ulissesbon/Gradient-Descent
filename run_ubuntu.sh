#!/bin/sh
python generate_data.py 
rm resultados_finais.txt
gcc -o main main.c
./main
python main.py
python view.py