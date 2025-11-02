"""
Visualizador de Treinamento em C
=================================
Lê o histórico de treinamento gerado pelo código C e cria animações.
"""

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation


def carregar_dados_dataset(caminho_csv):
    """Carrega os dados originais do dataset."""
    dados = np.loadtxt(caminho_csv, delimiter=",", skiprows=1)
    X = dados[:, 0]
    Y = dados[:, 1]
    return X, Y


def carregar_historico_treinamento(caminho_historico):
    """
    Carrega o histórico de treinamento do arquivo CSV gerado pelo C.
    
    Formato esperado: epoca,a,b,mse
    
    Returns:
        dict com arrays: epocas, valores_a, valores_b, valores_mse
    """
    dados = np.loadtxt(caminho_historico, delimiter=",", skiprows=1)
    
    return {
        'epocas': dados[:, 0].astype(int),
        'valores_a': dados[:, 1],
        'valores_b': dados[:, 2],
        'valores_mse': dados[:, 3]
    }


def configurar_visualizacao(X, Y, historico):
    """Configura os três subplots para animação."""
    
    epocas = historico['epocas']
    valores_a = historico['valores_a']
    valores_b = historico['valores_b']
    valores_mse = historico['valores_mse']
    
    # Criar figura
    fig, (ax1, ax2, ax3) = plt.subplots(1, 3, figsize=(16, 5))
    
    # ---- SUBPLOT 1: Ajuste da reta ----
    ax1.scatter(X, Y, color='blue', alpha=0.5, s=20, label='Dados reais')
    linha_ajuste, = ax1.plot([], [], 'r-', linewidth=2.5, label='Reta treinada (C)')
    ax1.set_xlim(X.min() - 1, X.max() + 1)
    ax1.set_ylim(Y.min() - 10, Y.max() + 10)
    ax1.set_xlabel('X', fontsize=11)
    ax1.set_ylabel('Y', fontsize=11)
    ax1.set_title('Ajuste da Reta (Código C)', fontsize=12, fontweight='bold')
    ax1.legend(loc='upper left')
    ax1.grid(True, alpha=0.3)
    
    # Texto para mostrar época e parâmetros
    texto_params = ax1.text(0.02, 0.98, '', transform=ax1.transAxes,
                           verticalalignment='top', fontsize=9,
                           bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.8))
    
    # ---- SUBPLOT 2: Evolução do MSE ----
    ax2.set_xlim(0, epocas[-1])
    ax2.set_ylim(0, valores_mse[0] * 1.1)
    ax2.set_xlabel('Época', fontsize=11)
    ax2.set_ylabel('MSE (Erro Quadrático Médio)', fontsize=11)
    ax2.set_title('Convergência do Erro', fontsize=12, fontweight='bold')
    linha_mse, = ax2.plot([], [], 'g-', linewidth=2)
    ponto_mse, = ax2.plot([], [], 'go', markersize=8)
    ax2.grid(True, alpha=0.3)
    
    # ---- SUBPLOT 3: Trajetória no espaço de parâmetros ----
    linha_trajetoria, = ax3.plot([], [], 'r.-', linewidth=1.5, markersize=5,
                                  label='Trajetória (a, b)', alpha=0.7)
    ponto_atual, = ax3.plot([], [], 'ro', markersize=10, label='Posição atual')
    ax3.set_xlabel('Coeficiente Angular (a)', fontsize=11)
    ax3.set_ylabel('Coeficiente Linear (b)', fontsize=11)
    ax3.set_title('Espaço de Parâmetros', fontsize=12, fontweight='bold')
    ax3.legend()
    ax3.grid(True, alpha=0.3)
    
    # Adicionar campo de gradiente (opcional)
    if len(valores_a) > 10:
        passo = max(1, len(valores_a) // 20)
        a_sample = valores_a[::passo]
        b_sample = valores_b[::passo]
        
        # Calcular "direção" aproximada
        da = np.diff(a_sample)
        db = np.diff(b_sample)
        
        ax3.quiver(a_sample[:-1], b_sample[:-1], da, db,
                   color='gray', alpha=0.3, scale_units='xy', scale=1,
                   width=0.003)
    
    elementos = {
        'linha_ajuste': linha_ajuste,
        'texto_params': texto_params,
        'linha_mse': linha_mse,
        'ponto_mse': ponto_mse,
        'linha_trajetoria': linha_trajetoria,
        'ponto_atual': ponto_atual
    }
    
    return fig, ax1, ax2, ax3, elementos


def atualizar_frame(frame_idx, X, Y, historico, elementos):
    """Atualiza cada frame da animação."""
    
    epocas = historico['epocas']
    valores_a = historico['valores_a']
    valores_b = historico['valores_b']
    valores_mse = historico['valores_mse']
    
    # Parâmetros atuais
    a_atual = valores_a[frame_idx]
    b_atual = valores_b[frame_idx]
    epoca_atual = epocas[frame_idx]
    mse_atual = valores_mse[frame_idx]
    
    # Atualizar reta de ajuste
    Y_pred = a_atual * X + b_atual
    elementos['linha_ajuste'].set_data(X, Y_pred)
    
    # Atualizar texto com parâmetros
    texto = (f'Época: {epoca_atual}\n'
             f'a = {a_atual:.6f}\n'
             f'b = {b_atual:.6f}\n'
             f'MSE = {mse_atual:.4f}')
    elementos['texto_params'].set_text(texto)
    
    # Atualizar gráfico de MSE
    epocas_ate_agora = epocas[:frame_idx + 1]
    mse_ate_agora = valores_mse[:frame_idx + 1]
    elementos['linha_mse'].set_data(epocas_ate_agora, mse_ate_agora)
    elementos['ponto_mse'].set_data([epoca_atual], [mse_atual])
    
    # Atualizar trajetória
    a_ate_agora = valores_a[:frame_idx + 1]
    b_ate_agora = valores_b[:frame_idx + 1]
    elementos['linha_trajetoria'].set_data(a_ate_agora, b_ate_agora)
    elementos['ponto_atual'].set_data([a_atual], [b_atual])
    
    return (elementos['linha_ajuste'], elementos['texto_params'],
            elementos['linha_mse'], elementos['ponto_mse'],
            elementos['linha_trajetoria'], elementos['ponto_atual'])


def main():
    """Função principal."""
    
    # Configurações
    DATASET = "data/dataset3.csv"
    HISTORICO = "historico_treinamento.csv"
    SALVAR_GIF = True
    ARQUIVO_GIF = "animacao_treinamento.gif"
    
    print("=" * 70)
    print("VISUALIZADOR DE TREINAMENTO EM C")
    print("=" * 70)
    
    # Carregar dados
    print("\n1. Carregando dataset...")
    X, Y = carregar_dados_dataset(DATASET)
    print(f"   ✓ {len(X)} amostras carregadas")
    
    print("\n2. Carregando histórico de treinamento...")
    historico = carregar_historico_treinamento(HISTORICO)
    print(f"   ✓ {len(historico['epocas'])} épocas registradas")
    print(f"   - Parâmetros finais: a={historico['valores_a'][-1]:.6f}, "
          f"b={historico['valores_b'][-1]:.6f}")
    print(f"   - MSE inicial: {historico['valores_mse'][0]:.4f}")
    print(f"   - MSE final: {historico['valores_mse'][-1]:.4f}")
    
    # Configurar visualização
    print("\n3. Configurando visualização...")
    fig, ax1, ax2, ax3, elementos = configurar_visualizacao(X, Y, historico)
    
    # Criar animação (mostrar 1 a cada 50 épocas para ser mais rápido)
    passo = max(1, len(historico['epocas']) // 200)
    frames_indices = range(0, len(historico['epocas']), passo)
    
    print(f"4. Gerando animação ({len(frames_indices)} frames)...")
    animacao = FuncAnimation(
        fig,
        lambda frame: atualizar_frame(frame, X, Y, historico, elementos),
        frames=frames_indices,
        interval=100,  # 100ms entre frames
        blit=True,
        repeat=True
    )
    
    plt.tight_layout()
    duracao_desejada = 5 # segundos
    fps_desejado = len(frames_indices) / duracao_desejada

    # Salvar GIF se solicitado
    if SALVAR_GIF:
        print(f"\n5. Salvando animação como GIF...")
        print(f"   (Isso pode levar alguns minutos...)")
        try:
            animacao.save(ARQUIVO_GIF, writer='pillow', fps=fps_desejado, dpi=100)
            print(f"   ✓ GIF salvo em: {ARQUIVO_GIF}")
        except Exception as e:
            print(f"   ⚠️  Erro ao salvar GIF: {e}")
            print(f"   Dica: Instale pillow com 'pip install pillow'")
    
    print("\n✓ Animação pronta!\n")
    plt.show()


if __name__ == "__main__":
    main()