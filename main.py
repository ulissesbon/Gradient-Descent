"""
Treinamento de Regressão Linear com Descida de Gradiente
=========================================================
Treina modelos de regressão linear usando SGDRegressor (Scikit-learn)
com centralização e normalização dos dados.

Este script replica o comportamento do código em C, permitindo
comparação direta entre as implementações.
"""

import numpy as np
from sklearn.linear_model import SGDRegressor
from sklearn.preprocessing import StandardScaler
from pathlib import Path


# ============================================================================
# CONFIGURAÇÕES DE TREINAMENTO
# ============================================================================

class ConfiguracaoTreinamento:
    """Classe para armazenar hiperparâmetros de treinamento."""
    
    # Quantidade de datasets a processar
    QUANTIDADE_ARQUIVOS = 4
    
    # Hiperparâmetros do gradiente descendente
    NUMERO_EPOCAS = 20000
    TAXA_APRENDIZADO = 1e-5
    
    # Diretório dos datasets
    DIRETORIO_DADOS = "data"
    
    # Precisão para arredondamento de X (evita erros numéricos)
    CASAS_DECIMAIS_PRECISAO = 6


# ============================================================================
# FUNÇÕES DE CARREGAMENTO E PRÉ-PROCESSAMENTO
# ============================================================================

def carregar_dataset(caminho_arquivo):
    """
    Carrega um dataset de um arquivo CSV.
    
    Args:
        caminho_arquivo: Caminho do arquivo CSV
        
    Returns:
        tuple: (valores_x, valores_y)
    """
    # Carregar dados pulando o cabeçalho
    dados = np.loadtxt(caminho_arquivo, delimiter=",", skiprows=1)
    
    # Separar features (X) e targets (Y)
    valores_x = dados[:, 0]
    valores_y = dados[:, 1]
    
    return valores_x, valores_y


def preprocessar_features(valores_x, casas_decimais):
    """
    Preprocessa os valores de X para evitar problemas de precisão numérica.
    
    Args:
        valores_x: Array com valores de entrada
        casas_decimais: Número de casas decimais para arredondamento
        
    Returns:
        ndarray: Array processado no formato correto (n_amostras, 1)
    """
    # Reshape para formato de coluna
    x_reshape = valores_x.reshape(-1, 1)
    
    # Arredondar para evitar problemas de precisão
    x_arredondado = np.round(x_reshape, decimals=casas_decimais)
    
    return x_arredondado


def centralizar_e_normalizar(valores_x):
    """
    Centraliza (subtrai média) e normaliza (divide por desvio padrão) os dados.
    
    Esta transformação melhora significativamente a convergência do SGD:
    - Centralização: Remove o acoplamento entre a e b
    - Normalização: Equilibra a escala dos gradientes
    
    Args:
        valores_x: Array com valores originais
        
    Returns:
        tuple: (valores_centralizados, scaler_usado)
    """
    scaler = StandardScaler(with_mean=True, with_std=True)
    valores_centralizados = scaler.fit_transform(valores_x)
    
    return valores_centralizados, scaler


# ============================================================================
# FUNÇÕES DE TREINAMENTO
# ============================================================================

def criar_modelo_sgd(taxa_aprendizado, numero_epocas):
    """
    Cria e configura o modelo SGDRegressor.
    
    Configurações importantes:
    - alpha=0: Sem regularização (queremos ajuste puro)
    - penalty=None: Desabilita penalização
    - learning_rate='constant': Taxa de aprendizado fixa
    - shuffle=False: Mantém ordem determinística
    - tol=None: Desabilita critério de parada por convergência
    
    Args:
        taxa_aprendizado: Passo de atualização dos pesos
        numero_epocas: Número de iterações de treinamento
        
    Returns:
        SGDRegressor: Modelo configurado
    """
    modelo = SGDRegressor(
        alpha=0.0,                          # Sem regularização L2
        penalty=None,                       # Desabilita penalização
        learning_rate='constant',           # Taxa constante
        eta0=taxa_aprendizado,              # Valor da taxa de aprendizado
        shuffle=False,                      # Não embaralhar dados
        max_iter=numero_epocas,             # Número máximo de épocas
        tol=None,                           # Sem critério de parada automático
        fit_intercept=True,                 # Aprender o intercepto (b)
        random_state=None                   # Sem seed (comportamento padrão)
    )
    
    return modelo


def treinar_modelo(valores_x_centralizados, valores_y, config):
    """
    Treina o modelo de regressão linear usando descida de gradiente.
    
    Args:
        valores_x_centralizados: Features após centralização/normalização
        valores_y: Valores alvo (não transformados)
        config: Configurações de treinamento
        
    Returns:
        SGDRegressor: Modelo treinado
    """
    # Criar modelo
    modelo = criar_modelo_sgd(config.TAXA_APRENDIZADO, config.NUMERO_EPOCAS)
    
    # Treinar modelo (ajusta a e b no espaço centralizado)
    modelo.fit(valores_x_centralizados, valores_y)
    
    return modelo


def converter_parametros_escala_original(modelo, scaler):
    """
    Converte os parâmetros aprendidos de volta para a escala original.
    
    No espaço centralizado:
        y = a_c * x_c + b_c
        onde x_c = (x - media_x) / desvio_x
    
    Na escala original:
        y = a * x + b
        onde:
            a = a_c / desvio_x
            b = b_c - a * media_x
    
    Args:
        modelo: Modelo treinado no espaço centralizado
        scaler: StandardScaler usado na transformação
        
    Returns:
        tuple: (coeficiente_angular, coeficiente_linear)
    """
    # Parâmetros no espaço centralizado
    a_centralizado = modelo.coef_[0]
    b_centralizado = modelo.intercept_[0]
    
    # Converter para escala original
    coeficiente_angular = a_centralizado / scaler.scale_[0]
    coeficiente_linear = b_centralizado - coeficiente_angular * scaler.mean_[0]
    
    return coeficiente_angular, coeficiente_linear


# ============================================================================
# FUNÇÕES DE EXIBIÇÃO
# ============================================================================

def exibir_informacoes_dataset(indice, scaler, coef_angular, coef_linear):
    """
    Exibe informações sobre o dataset e os parâmetros aprendidos.
    
    Args:
        indice: Índice do dataset
        scaler: StandardScaler usado
        coef_angular: Coeficiente angular aprendido (a)
        coef_linear: Coeficiente linear aprendido (b)
    """
    print("\n" + "=" * 70)
    print(f"Dataset {indice} - Descida de Gradiente (Python)")
    print("=" * 70)
    print(f"  Estatísticas dos dados:")
    print(f"    • Média de X: {scaler.mean_[0]:.6f}")
    print(f"    • Desvio padrão de X: {scaler.scale_[0]:.6f}")
    print(f"\n  Parâmetros do modelo linear (y = a*x + b):")
    print(f"    • Coeficiente angular (a): {coef_angular:.6f}")
    print(f"    • Coeficiente linear (b):  {coef_linear:.6f}")
    print("=" * 70)

def salvar_parametros_finais(indice_dataset, coef_angular, coef_linear, valores_x, valores_y):
    """
    Salva os parâmetros finais em arquivo para comparação.
    
    Args:
        indice_dataset: Número do dataset
        coef_angular: Coeficiente angular (a)
        coef_linear: Coeficiente linear (b)
        valores_x: Features originais
        valores_y: Targets originais
    """
    # Calcular MSE
    y_pred = coef_angular * valores_x + coef_linear
    mse = np.mean((y_pred - valores_y) ** 2)
    
    # Salvar em arquivo (modo append)
    with open("resultados_finais.txt", "a") as f:
        f.write(f"Dataset {indice_dataset}: a={coef_angular:.10f}, "
                f"b={coef_linear:.10f}, mse={mse:.10f}\n")


# ============================================================================
# FUNÇÃO DE PROCESSAMENTO DE DATASET
# ============================================================================

def processar_dataset(indice, config):
    """
    Processa um único dataset: carrega, treina e exibe resultados.
    
    Args:
        indice: Índice do dataset a processar
        config: Configurações de treinamento
        
    Returns:
        dict: Resultados do treinamento
    """
    # Construir caminho do arquivo
    caminho_arquivo = f"{config.DIRETORIO_DADOS}/dataset{indice}.csv"
    
    # Verificar se arquivo existe
    if not Path(caminho_arquivo).exists():
        print(f"\n⚠️  Arquivo não encontrado: {caminho_arquivo}")
        return None
    
    # 1. Carregar dados
    valores_x, valores_y = carregar_dataset(caminho_arquivo)
    
    # 2. Preprocessar features
    x_preprocessado = preprocessar_features(valores_x, config.CASAS_DECIMAIS_PRECISAO)
    
    # 3. Centralizar e normalizar
    x_centralizado, scaler = centralizar_e_normalizar(x_preprocessado)
    
    # 4. Treinar modelo
    modelo = treinar_modelo(x_centralizado, valores_y, config)
    
    # 5. Converter parâmetros para escala original
    coef_angular, coef_linear = converter_parametros_escala_original(modelo, scaler)
    
    # 6. Salvar resultados finais
    salvar_parametros_finais(indice, coef_angular, coef_linear, valores_x, valores_y)

    # 7. Exibir resultados
    exibir_informacoes_dataset(indice, scaler, coef_angular, coef_linear)
    
    # Retornar resultados
    resultados = {
        'indice': indice,
        'coeficiente_angular': coef_angular,
        'coeficiente_linear': coef_linear,
        'media_x': scaler.mean_[0],
        'desvio_x': scaler.scale_[0]
    }
    
    return resultados


# ============================================================================
# FUNÇÃO PRINCIPAL
# ============================================================================

def main():
    """Função principal que coordena o treinamento de todos os datasets."""
    
    print("=" * 70)
    print("REGRESSÃO LINEAR COM DESCIDA DE GRADIENTE EM PYTHON")
    print("=" * 70)
    
    # Carregar configurações
    config = ConfiguracaoTreinamento()
    
    print(f"\n⚙️  Configurações:")
    print(f"  • Taxa de aprendizado: {config.TAXA_APRENDIZADO}")
    print(f"  • Número de épocas: {config.NUMERO_EPOCAS}")
    print(f"  • Datasets a processar: {config.QUANTIDADE_ARQUIVOS}")
    
    # Processar cada dataset
    todos_resultados = []
    
    for indice in range(config.QUANTIDADE_ARQUIVOS):
        resultado = processar_dataset(indice, config)
        
        if resultado is not None:
            todos_resultados.append(resultado)
    
    # Resumo final
    print("\n" + "=" * 70)
    print("RESUMO FINAL")
    print("=" * 70)
    print(f"✅ {len(todos_resultados)} dataset(s) processado(s) com sucesso!")
    
    for res in todos_resultados:
        print(f"\n  Dataset {res['indice']}: "
              f"a = {res['coeficiente_angular']:.6f}, "
              f"b = {res['coeficiente_linear']:.6f}")
    
    print("\n" + "=" * 70 + "\n")


if __name__ == "__main__":
    main()