#!/bin/sh
python gerar_dados.py && gcc -o main main.c && sudo ./main && python main.py  && python visualizador.py