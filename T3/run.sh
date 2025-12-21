#!/bin/sh

# resetar/apagar arquivos gerados
rm results.txt
rm original/*.csv
rm shuffle/*.csv

# gerar dados
python3 generate_data.py 

# embaralhar sample de 75% e gerar arquivos shuffle
python3 shuffle.py 

# rodar notebook e gravar results.txt
gcc -o main notebook.c
./main

# # iniciar embarcado no modo gravar (na ide do stm32)
# # enviar dados para placa guardar na flash
# python sender_receiver.py # no modo gravar

# # iniciar embarcado no modo executar (na ide do stm32)
# # receber respostas da placa (tempo e cálculos) e guardar nos results.txt
# python sender_receiver.py # no modo monitorar

# # calcular médias e desvios padrões e gerar melhoria_{n}.png sendo n o número da melhoria
# python calculate.py