import pandas as pd
import matplotlib.pyplot as plt

def gerar_grafico_final():
    df = pd.read_csv('results_brutos.csv')
    stats = df.groupby('versao')['tempo'].mean().reset_index()
    
    # Define v0_original como base 100%
    t_ref = stats.loc[stats['versao'] == 'v0_original', 'tempo'].values[0]
    stats['Speedup'] = t_ref / stats['tempo']
    
    plt.figure(figsize=(10, 6))
    colors = ['#ff9999','#66b3ff','#99ff99','#ffcc99']
    bars = plt.bar(stats['versao'], stats['tempo'], color=colors)
    
    plt.title('Comparativo de Performance (Compilado com -O2)')
    plt.ylabel('Tempo de Execução (s)')
    
    for bar, speedup in zip(bars, stats['Speedup']):
        plt.text(bar.get_x() + bar.get_width()/2, bar.get_height(), 
                 f'{speedup:.2f}x mais rápido', ha='center', va='bottom')
    
    plt.savefig('comparativo_final_O2.png')
    print("Gráfico comparativo_final_O2.png gerado com sucesso.")

if __name__ == "__main__":
    gerar_grafico_final()