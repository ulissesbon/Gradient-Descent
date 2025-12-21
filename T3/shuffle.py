import csv
import random

for i in range(4):
    # Definição dos nomes dos arquivos
    arquivo_origem = f'original/dataset_original{i}.csv'
    arquivo_destino = f'shuffle/dataset_randomico{i}.csv'

    # ---------------------------------------------------------
    # Ler o CSV original e criar o segundo com 750 pontos
    # ---------------------------------------------------------
    print(f"\nLendo {arquivo_origem} e gerando {arquivo_destino}...")

    dados_lidos = []

    # Abre o arquivo recém-criado para leitura (conforme solicitado)
    with open(arquivo_origem, mode='r', encoding='utf-8') as f:
        reader = csv.reader(f)
        header = next(reader)  # Pula o cabeçalho
        
        for row in reader:
            dados_lidos.append(row)

    # Seleciona 750 itens aleatórios da lista lida.
    # O random.sample já pega os itens e os embaralha (não virão na ordem original)
    amostra_randomica = random.sample(dados_lidos, 750)

    # Escreve o segundo arquivo CSV
    with open(arquivo_destino, mode='w', newline='', encoding='utf-8') as f:
        writer = csv.writer(f)
        writer.writerow(['x', 'y'])  # Cabeçalho
        writer.writerows(amostra_randomica)

    print(f"Sucesso! {arquivo_destino} criado com 750 linhas fora de ordem.")