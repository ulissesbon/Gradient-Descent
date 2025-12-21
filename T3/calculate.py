import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

def processar_resultados():
    try:
        df = pd.read_csv('results_brutos.csv')
    except Exception as e:
        print(f"Erro ao ler CSV: {e}")
        return

    # Agrupar por Versão e Dataset
    stats = df.groupby(['versao', 'dataset']).agg({
        'tempo': ['mean', 'std'],
        'code_sz': 'first',
        'data_sz': 'first'
    }).reset_index()

    # Flatten nas colunas
    stats.columns = ['Versão', 'Dataset', 'Tempo_Media', 'Tempo_Desvio', 'Flash_Bytes', 'RAM_Bytes']
    
    print("\n--- RESUMO DE DESEMPENHO ---")
    print(stats.to_string(index=False))
    
    # Salvar tabela para o slide
    stats.to_csv('relatorio_final.csv', index=False)

    # Gerar Gráfico de Comparação de Tempo
    plt.figure(figsize=(10, 6))
    for versao in stats['Versão'].unique():
        subset = stats[stats['Versão'] == versao]
        plt.errorbar(subset['Dataset'], subset['Tempo_Media'], yerr=subset['Tempo_Desvio'], 
                     fmt='-o', capsize=5, label=f'Tempo - {versao}')

    plt.title('Tempo de Computação por Dataset (Média ± DP)')
    plt.xlabel('ID do Dataset')
    plt.ylabel('Tempo (s)')
    plt.legend()
    plt.grid(True, linestyle='--')
    plt.savefig('comparativo_tempo.png')
    print("\n[SUCESSO] Gráfico 'comparativo_tempo.png' gerado.")

if __name__ == "__main__":
    processar_resultados()