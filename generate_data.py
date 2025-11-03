"""
Gerador de Datasets para Regressão Linear
==========================================
Gera datasets sintéticos com diferentes parâmetros de ruído e inclinação
para testar algoritmos de regressão linear.

Formato de saída: CSV com colunas 'x,y'
"""

import numpy as np
from pathlib import Path


# ============================================================================
# CONFIGURAÇÕES DOS DATASETS
# ============================================================================

class ConfiguracaoDataset:
    """Classe para armazenar configurações de geração de datasets."""
    
    # Quantidade de arquivos a serem gerados
    QUANTIDADE_ARQUIVOS = 4
    
    # Número de amostras por dataset
    QUANTIDADE_AMOSTRAS = 1000
    
    # Parâmetros da função linear y = ax + b
    # Cada posição corresponde a um dataset diferente
    COEFICIENTES_ANGULARES = [2, 60, 100, 300]      # Inclinação (a)
    COEFICIENTES_LINEARES = [1, 70, 1000, 0]        # Intercepto (b)
    
    # Desvio padrão do ruído como proporção do intercepto
    PROPORCAO_RUIDO = 0.5
    
    # Diretório de saída
    DIRETORIO_SAIDA = "data"


# ============================================================================
# FUNÇÕES DE GERAÇÃO
# ============================================================================

def calcular_desvios_padrao_ruido(coeficientes_lineares, proporcao):
    """
    Calcula o desvio padrão do ruído para cada dataset.
    
    O ruído é proporcional ao intercepto (b), tornando datasets com
    valores maiores naturalmente mais ruidosos.
    
    Args:
        coeficientes_lineares: Lista com valores de b para cada dataset
        proporcao: Proporção do ruído em relação a b
        
    Returns:
        list: Desvios padrão do ruído para cada dataset
    """
    return [b * proporcao for b in coeficientes_lineares]


def gerar_valores_x(quantidade_amostras):
    """
    Gera valores uniformemente espaçados para o eixo X.
    
    Args:
        quantidade_amostras: Número de pontos a serem gerados
        
    Returns:
        ndarray: Array com valores de 1 até quantidade_amostras
    """
    return np.arange(1, quantidade_amostras + 1, dtype=np.float64)


def gerar_valores_y(valores_x, coeficiente_angular, coeficiente_linear, desvio_padrao_ruido):
    """
    Gera valores de Y seguindo o modelo linear com ruído gaussiano.
    
    Modelo: y = a*x + b + ruído
    onde ruído ~ N(0, desvio_padrao_ruido)
    
    Args:
        valores_x: Array com valores de entrada
        coeficiente_angular: Inclinação da reta (a)
        coeficiente_linear: Intercepto da reta (b)
        desvio_padrao_ruido: Desvio padrão do ruído gaussiano
        
    Returns:
        ndarray: Array com valores de Y gerados
    """
    # Componente determinística: y = ax + b
    y_deterministico = valores_x * coeficiente_angular + coeficiente_linear
    
    # Componente estocástico: ruído gaussiano
    ruido = np.random.normal(loc=0, scale=desvio_padrao_ruido, size=len(valores_x))
    
    # Combinação final
    valores_y = y_deterministico + ruido
    
    return valores_y.astype(np.float64)


def criar_diretorio_se_necessario(caminho_diretorio):
    """
    Cria o diretório de saída se ele não existir.
    
    Args:
        caminho_diretorio: Caminho do diretório a ser criado
    """
    Path(caminho_diretorio).mkdir(parents=True, exist_ok=True)


def salvar_dataset_csv(valores_x, valores_y, caminho_arquivo):
    """
    Salva o dataset em formato CSV.
    
    Formato:
        x,y
        1.0,3.2
        2.0,5.1
        ...
    
    Args:
        valores_x: Array com valores de X
        valores_y: Array com valores de Y
        caminho_arquivo: Caminho completo do arquivo de saída
    """
    # Empilhar X e Y como colunas
    dados = np.column_stack((valores_x, valores_y))
    
    # Salvar em CSV com cabeçalho
    np.savetxt(
        caminho_arquivo,
        dados,
        delimiter=",",
        header="x,y",
        comments='',  # Remove o caractere '#' do cabeçalho
        fmt='%.1f'    # Formato com 1 casa decimal
    )


def gerar_dataset(indice, valores_x, config):
    """
    Gera um único dataset com os parâmetros especificados.
    
    Args:
        indice: Índice do dataset (0 a N-1)
        valores_x: Array com valores de X (compartilhado entre datasets)
        config: Instância de ConfiguracaoDataset
        
    Returns:
        tuple: (valores_x, valores_y, parametros)
    """
    # Obter parâmetros para este dataset
    a = config.COEFICIENTES_ANGULARES[indice]
    b = config.COEFICIENTES_LINEARES[indice]
    
    # Calcular desvio padrão do ruído
    desvios_ruido = calcular_desvios_padrao_ruido(
        config.COEFICIENTES_LINEARES,
        config.PROPORCAO_RUIDO
    )
    desvio_ruido = desvios_ruido[indice]
    
    # Gerar valores de Y
    valores_y = gerar_valores_y(valores_x, a, b, desvio_ruido)
    
    parametros = {
        'a': a,
        'b': b,
        'desvio_ruido': desvio_ruido
    }
    
    return valores_x, valores_y, parametros


def exibir_resumo_dataset(indice, parametros, quantidade_amostras):
    """
    Exibe informações resumidas sobre o dataset gerado.
    
    Args:
        indice: Índice do dataset
        parametros: Dicionário com parâmetros do dataset
        quantidade_amostras: Número de amostras geradas
    """
    print(f"\n{'='*60}")
    print(f"Dataset {indice} gerado com sucesso!")
    print(f"{'='*60}")
    print(f"  Arquivo: dataset{indice}.csv")
    print(f"  Amostras: {quantidade_amostras}")
    print(f"  Modelo: y = {parametros['a']}*x + {parametros['b']} + ruído")
    print(f"  Desvio padrão do ruído: {parametros['desvio_ruido']:.2f}")
    print(f"{'='*60}")


# ============================================================================
# FUNÇÃO PRINCIPAL
# ============================================================================

def main():
    """Função principal que coordena a geração de todos os datasets."""
    
    print("=" * 70)
    print("GERADOR DE DATASETS PARA REGRESSÃO LINEAR")
    print("=" * 70)
    
    # Carregar configurações
    config = ConfiguracaoDataset()
    
    # Criar diretório de saída
    criar_diretorio_se_necessario(config.DIRETORIO_SAIDA)
    print(f"\n Diretório de saída: {config.DIRETORIO_SAIDA}/")
    
    # Gerar valores de X (compartilhados por todos os datasets)
    valores_x = gerar_valores_x(config.QUANTIDADE_AMOSTRAS)
    print(f" Gerando {config.QUANTIDADE_ARQUIVOS} dataset(s) com "
          f"{config.QUANTIDADE_AMOSTRAS} amostras cada...")
    
    # Gerar cada dataset
    for indice in range(config.QUANTIDADE_ARQUIVOS):
        # Gerar dados
        x, y, parametros = gerar_dataset(indice, valores_x, config)
        
        # Construir caminho do arquivo
        caminho_arquivo = f"{config.DIRETORIO_SAIDA}/dataset{indice}.csv"
        
        # Salvar em CSV
        salvar_dataset_csv(x, y, caminho_arquivo)
        
        # Exibir resumo
        exibir_resumo_dataset(indice, parametros, len(x))
    
    print("\n Todos os datasets foram gerados com sucesso!\n")


if __name__ == "__main__":
    # Configurar seed para reprodutibilidade (opcional)
    # np.random.seed(42)
    
    main()