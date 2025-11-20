import pandas as pd
import re
import matplotlib.pyplot as plt
import seaborn as sns
import os

# === CONFIGURAÇÕES ===
RESULTS_FILE = 'resultados_finais.txt'
OUTPUT_IMG = 'comparacao_mse_profissional.png'

# Estilo limpo e executivo
sns.set_theme(style="whitegrid", context="talk")

def parse_results(filepath):
    """Lê o arquivo de texto e extrai os dados via Regex."""
    data = []
    # Regex robusta: aceita espaços ou vírgulas como separadores
    pattern = r'\((.*?)\)\s+Dataset\s+(\d+):\s+a=([\d\.]+)[,\s]+b=([\d\.]+)[,\s]+mse=([\d\.]+)'

    if not os.path.exists(filepath):
        print(f"ERRO: Arquivo '{filepath}' não encontrado.")
        return pd.DataFrame()

    with open(filepath, 'r') as f:
        for line in f:
            match = re.search(pattern, line)
            if match:
                data.append({
                    'Estrategia': match.group(1),
                    'Dataset': int(match.group(2)),
                    'mse': float(match.group(5)) # Focamos apenas no MSE
                })
    return pd.DataFrame(data)

def main():
    df = parse_results(RESULTS_FILE)
    
    if df.empty:
        print("Nenhum dado encontrado.")
        return

    # Configurar a figura
    plt.figure(figsize=(12, 7))
    
    # Cores Profissionais: C (Azul), Python (Verde), Emb (Vermelho)
    custom_palette = {'C Lang': '#1f77b4', 'Python': '#2ca02c', 'Emb': '#d62728'}

    # Criar o gráfico de barras agrupadas
    ax = sns.barplot(
        data=df, 
        x='Dataset', 
        y='mse', 
        hue='Estrategia', 
        palette=custom_palette,
        edgecolor='black',  # Borda preta para definição
        linewidth=1
    )

    # === MELHORIA VISUAL: ZOOM NO EIXO Y ===
    # Como os erros são parecidos, cortamos o eixo Y para destacar a diferença
    min_mse = df['mse'].min()
    max_mse = df['mse'].max()
    
    # Define o chão do gráfico como 95% do menor valor encontrado
    ylim_bottom = min_mse * 0.95 
    ylim_top = max_mse * 1.02
    
    ax.set_ylim(ylim_bottom, ylim_top)

    # Adicionar os valores exatos no topo das barras
    for container in ax.containers:
        ax.bar_label(container, fmt='%.4f', padding=5, fontsize=11, fontweight='bold')

    # Títulos e Legendas
    plt.title('Comparativo de Precisão (MSE) por Estratégia', fontweight='bold', pad=20)
    plt.ylabel('Mean Squared Error (Menor é Melhor)')
    plt.xlabel('Dataset de Teste')
    plt.legend(title='Implementação', bbox_to_anchor=(1.02, 1), loc='upper left', borderaxespad=0)

    # Ajuste final e salvamento
    plt.tight_layout()
    print(f"Gerando gráfico: {OUTPUT_IMG}")
    plt.savefig(OUTPUT_IMG, dpi=300)
    plt.show()
    
    # Printar tabela simples no terminal para conferência
    print("\n=== Tabela de MSE ===")
    pivot_table = df.pivot(index='Dataset', columns='Estrategia', values='mse')
    print(pivot_table)

if __name__ == "__main__":
    main()