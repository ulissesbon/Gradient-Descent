import pandas as pd
import matplotlib.pyplot as plt

def gerar_analise():
    df = pd.read_csv('results.csv')
    
    # Agrupar por versão para estatísticas de tempo e precisão
    stats = df.groupby('versao').agg({
        'tempo': ['mean', 'std'],
        'a': 'mean',
        'b': 'mean',
        'mse': 'mean'
    }).reset_index()
    stats.columns = ['Versao', 'Tempo_Medio', 'Tempo_Desvio', 'A_Medio', 'B_Medio', 'MSE_Medio']
    
    # REFERÊNCIA: v0_Original_O0
    ref_row = stats[stats['Versao'] == 'v0_Original_O0']
    if ref_row.empty:
        print("Erro: v0_Original_O0 não encontrada nos dados.")
        return
        
    t_ref = ref_row['Tempo_Medio'].values[0]
    
    # Calcular Melhoria Relativa em % (Valores positivos = mais rápido que a referência)
    stats['Melhoria_%'] = ((t_ref - stats['Tempo_Medio']) / t_ref) * 100

    print("\n--- RELATÓRIO DE PERFORMANCE (Ref: v0_Original_O0) ---")
    print(stats[['Versao', 'Tempo_Medio', 'Melhoria_%', 'A_Medio', 'B_Medio']])

    # Gráfico de Barras
    plt.figure(figsize=(12, 7))
    # Ordenar para uma narrativa lógica no slide
    ordem = ['v0_Original_O0', 'v0_Original_O2', 'v1_Manual_Inline', 'v2_Manual_Unrolling', 'v3_Manual_FixedPoint']
    stats['Versao'] = pd.Categorical(stats['Versao'], categories=ordem, ordered=True)
    stats = stats.sort_values('Versao')

    colors = ['gray', 'blue', 'green', 'orange', 'red']
    bars = plt.bar(stats['Versao'], stats['Tempo_Medio'], yerr=stats['Tempo_Desvio'], color=colors, capsize=10)
    
    plt.axhline(y=t_ref, color='r', linestyle='--', label='Referência (v0 -O0)')
    plt.ylabel('Tempo de Execução (s)')
    plt.title('Impacto das Otimizações vs Compilador (-O2)')
    plt.xticks(rotation=15)
    plt.legend()

    for bar, melhora in zip(bars, stats['Melhoria_%']):
        label = "REFERÊNCIA" if abs(melhora) < 0.001 else f"{melhora:+.1f}%"
        plt.text(bar.get_x() + bar.get_width()/2, bar.get_height(), label, ha='center', va='bottom', fontweight='bold')

    plt.tight_layout()
    plt.savefig('comparativo_final.png')

if __name__ == "__main__":
    gerar_analise()