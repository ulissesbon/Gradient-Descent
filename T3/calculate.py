import pandas as pd
import matplotlib.pyplot as plt

def processar_resultados():
    try:
        df = pd.read_csv('results_brutos.csv')
        if df.empty:
            print("Erro: O arquivo results_brutos.csv está vazio.")
            return
            
        # Forçar conversão para numérico para evitar erros de string
        df['tempo'] = pd.to_numeric(df['tempo'], errors='coerce')
        df = df.dropna(subset=['tempo'])

        # Agrupar e calcular 
        stats = df.groupby(['versao', 'dataset']).agg({
            'tempo': ['mean', 'std', 'count'],
            'code_sz': 'first',
            'data_sz': 'first'
        }).reset_index()

        stats.columns = ['Versão', 'Dataset', 'Média', 'Desvio', 'Amostras', 'Flash', 'RAM']
        
        # Preencher NaN no desvio com 0.0 (ocorre se Amostras < 2)
        stats['Desvio'] = stats['Desvio'].fillna(0.0)

        print("\n--- ESTATÍSTICAS DA EXPERIMENTAÇÃO ---")
        print(stats.to_string(index=False))
        
        stats.to_csv('relatorio_final.csv', index=False)

        # Gráfico 
        plt.figure(figsize=(10, 6))
        for versao in stats['Versão'].unique():
            sub = stats[stats['Versão'] == versao]
            plt.errorbar(sub['Dataset'], sub['Média'], yerr=sub['Desvio'], 
                         fmt='-o', capsize=5, label=versao)

        plt.title('Performance do Algoritmo: Média e Desvio Padrão')
        plt.xlabel('Dataset ID')
        plt.ylabel('Tempo (s)')
        plt.legend()
        plt.grid(True)
        plt.savefig('melhoria_grafico.png')
        
    except Exception as e:
        print(f"Erro no processamento: {e}")

if __name__ == "__main__":
    processar_resultados()