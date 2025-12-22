import pandas as pd
import matplotlib.pyplot as plt

def gerar_graficos():
    df = pd.read_csv('results.csv')
    stats = df.groupby('versao').agg({
        'tempo': ['mean', 'std'],
        'flash': 'first',
        'ram': 'first'
    }).reset_index()
    stats.columns = ['Versao', 'Tempo_Media', 'Tempo_STD', 'Flash', 'RAM']

    fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(12, 18))

    # Gráfico 1: Tempo
    ax1.bar(stats['Versao'], stats['Tempo_Media'], yerr=stats['Tempo_STD'], color='teal', capsize=7)
    ax1.set_title('Tempo de Execução (s)')
    ax1.grid(axis='y', linestyle='--')

    # Gráfico 2: Flash (Code Size)
    ax2.bar(stats['Versao'], stats['Flash'], color='darkblue')
    ax2.set_title('Memória Flash (Bytes) - Seção .text')
    ax2.set_ylim(min(stats['Flash'])*0.9, max(stats['Flash'])*1.1)

    # Gráfico 3: RAM (Data + BSS)
    ax3.bar(stats['Versao'], stats['RAM'], color='darkorange')
    ax3.set_title('Memória RAM (Bytes) - Seções .data + .bss')
    ax3.set_ylim(min(stats['RAM'])*0.9, max(stats['RAM'])*1.1)

    plt.tight_layout()
    plt.savefig('analise_completa.png')
    print(stats)

if __name__ == "__main__":
    gerar_graficos()