import csv
import random

for i in range(4):
    # Definição dos nomes dos arquivos
    arquivo_origem = f'original/dataset_original{i}.csv'
    arquivo_destino = f'shuffle/dataset_randomico{i}.csv'

    # ---------------------------------------------------------
    # PASSO 1: Gerar os dados e criar o primeiro CSV
    # ---------------------------------------------------------
    dados_originais = []

    print(f"Gerando {arquivo_origem}...")

    for x in range(1, 1001):
        # Gera ruído aleatório entre -0.5 e 0.5
        ruido = random.uniform(-0.5, 0.5)
        
        # Calcula y = 2*x + 1 + ruído
        y = 2 * x + 1 + ruido
        
        # Adiciona à lista (formatando para garantir legibilidade, opcional)
        dados_originais.append([x, y])

    # Escreve o primeiro arquivo CSV
    with open(arquivo_origem, mode='w', newline='', encoding='utf-8') as f:
        writer = csv.writer(f)
        writer.writerow(['x', 'y'])  # Cabeçalho
        writer.writerows(dados_originais)

    print(f"Sucesso! {arquivo_origem} criado com 1000 linhas.")

    # ---------------------------------------------------------
    # PASSO 2: Ler o primeiro CSV e criar o segundo com 750 pontos
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