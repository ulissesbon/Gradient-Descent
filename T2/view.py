import pandas as pd
import re
import matplotlib.pyplot as plt
import seaborn as sns

# 1. Ler e parsear o arquivo
data = []
# Ajustei o regex para ser flexível com espaços
pattern = r'\((.*?)\)\s+Dataset\s+(\d+):\s+a=([\d\.]+)[,\s]+b=([\d\.]+)[,\s]+mse=([\d\.]+)'

try:
    with open('resultados_finais.txt', 'r') as f:
        for line in f:
            match = re.search(pattern, line)
            if match:
                raw_strategy = match.group(1)
                # Renomear para clareza imediata
                if 'randomico' in raw_strategy:
                    clean_strategy = 'Randômico (no Original)'
                elif 'original' in raw_strategy:
                    clean_strategy = 'Original'
                else:
                    clean_strategy = raw_strategy

                data.append({
                    'Estrategia': clean_strategy,
                    'Dataset': int(match.group(2)),
                    'a': float(match.group(3)),
                    'b': float(match.group(4)),
                    'mse': float(match.group(5))
                })
except FileNotFoundError:
    print("Arquivo 'resultados_finais.txt' não encontrado.")

# Criar DataFrame
df = pd.DataFrame(data)

if not df.empty:
    # 2. Criar Tabela Comparativa Limpa (Sem Diff, Sem Melhor)
    comparacao = df.pivot(index='Dataset', columns='Estrategia', values='mse')
    
    # Ordenar colunas para garantir que Original venha antes ou depois conforme preferir
    # Aqui deixamos automático (alfabético), mas você pode forçar a ordem se quiser
    
    print("=== Tabela Final de MSE (Apenas Original vs Randômico) ===")
    print(comparacao)
    
    # 3. Visualização Profissional
    sns.set_theme(style="whitegrid", context="talk")
    plt.figure(figsize=(10, 6))

    # Definir cores profissionais: Azul (Original) vs Laranja/Vermelho (Randômico)
    cores = {'Original': '#1f77b4', 'Randômico (no Original)': '#ff7f0e'}

    # Gráfico
    chart = sns.barplot(
        data=df, 
        x='Dataset', 
        y='mse', 
        hue='Estrategia', 
        palette=cores
    )

    plt.title('Comparação de MSE: Estratégia Original vs Randômica', fontsize=14, pad=20)
    plt.ylabel('Mean Squared Error (MSE)')
    plt.xlabel('Dataset ID')
    
    # Ajuste de Zoom no eixo Y para ver a diferença sutil
    min_mse = df['mse'].min()
    ylim_bottom = min_mse * 0.90
    plt.ylim(bottom=ylim_bottom)
    
    # Legenda limpa
    plt.legend(title='', bbox_to_anchor=(1.02, 1), loc='upper left')

    plt.tight_layout()
    plt.savefig('comparacao_focada.png')
    print("\nGráfico salvo como 'comparacao_focada.png'")
    plt.show()
else:
    print("Nenhum dado foi carregado. Verifique o arquivo de texto.")