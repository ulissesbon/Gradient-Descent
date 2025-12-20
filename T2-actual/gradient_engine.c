/**
 * @file gradient_engine.c
 * @brief Implementação de Regressão Linear via Gradiente Descendente para Sistemas Embarcados.
 *
 * @section desc_sec Apresentação Geral
 * Esta aplicação implementa um motor de aprendizado de máquina supervisionado simples (Regressão Linear)
 * otimizado para microcontroladores. O algoritmo ajusta uma reta (y = ax + b) a um conjunto de dados
 * armazenados na memória Flash, minimizando o Erro Quadrático Médio (MSE).
 *
 * O diferencial desta implementação é o uso de Centralização de Dados, que mitiga problemas de 
 * precisão numérica comuns em variáveis de ponto flutuante (float) de precisão simples, garantindo
 * convergência estável mesmo em arquiteturas limitadas.
 *
 * @section copyright_sec Permissões de Uso
 * Copyright (c) 2025. Todos os direitos reservados.
 * Este código é disponibilizado para fins educacionais. A redistribuição e uso em formas de código 
 * fonte e binário, com ou sem modificação, são permitidos desde que mantido este aviso de copyright.
 *
 * @section usage_sec Como Usar
 * 1. Garanta que o dataset foi gravado na Flash externa/interna usando o módulo `dataset_flash`.
 * 2. Chame a função `gradient_run()` no loop principal ou em uma tarefa dedicada.
 * 3. Aguarde o retorno da função (processamento bloqueante).
 * 4. Recupere os resultados usando `gradient_a()`, `gradient_b()` e `gradient_mse()`.
 *
 * @section io_sec Entrada e Saída
 * - **Entrada:** Pares de coordenadas (x, y) do tipo `float`, lidos sequencialmente da memória Flash.
 * - **Saída:** Coeficientes da reta `a` (inclinação) e `b` (intercepto), e o erro final `mse`.
 *
 * @author Ulisses Gonçalves Bonfim e Raquel Maciel Coelho de Sousa
 * @date 19 de Novembro de 2025
 * @context Trabalho da disciplina de Sistemas Embarcados
 * @platform STM32 NUCLEO-F030R8
 */

#include "gradient_engine.h"
#include "dataset_flash.h"
#include <math.h>

/* ==========================================================================
 * Váriaveis Globais Estáticas (Privadas)
 * ========================================================================== */

/** * @brief Coeficiente angular final (inclinação da reta).
 * Armazena o resultado 'a' após a execução do treino.
 */
static float g_a;

/** * @brief Coeficiente linear final (intercepto).
 * Armazena o resultado 'b' após a execução do treino e descetralização.
 */
static float g_b;

/** * @brief Erro Quadrático Médio (Mean Squared Error).
 * Indica a qualidade do ajuste da reta aos dados (quanto menor, melhor).
 */
static float g_mse;

/* ==========================================================================
 * Hiperparâmetros e Constantes
 * ========================================================================== */

/** @brief Número total de amostras a serem lidas da Flash. */
static const int N = DATASET_MAX_SAMPLES;

/** @brief Quantidade de passadas completas pelo dataset (épocas). */
#define EPOCHS 30

/** @brief Taxa de aprendizado para o coeficiente 'a' (passo do gradiente). */
#define LR_A 1e-5

/** @brief Taxa de aprendizado para o coeficiente 'b'. */
#define LR_B 1e-5

/* ==========================================================================
 * Funções do Motor de Gradiente
 * ========================================================================== */

/**
 * @brief Executa o algoritmo de treinamento (Descida de Gradiente em Lote).
 *
 * Esta função realiza todo o processo de treinamento:
 * 1. Leitura dos dados da Flash para cálculo das médias (pré-processamento).
 * 2. Centralização dos dados para estabilidade numérica.
 * 3. Execução das iterações (épocas) de ajuste dos pesos.
 * 4. Conversão dos coeficientes de volta para o espaço original.
 * 5. Cálculo da métrica de erro final (MSE).
 *
 * @note O algoritmo utiliza Centralização de Dados: \f$ x_{cent} = x - \bar{x} \f$.
 * Isso evita estouro de precisão (overflow/underflow) e acelera a convergência.
 *
 * @warning Esta função é computacionalmente intensiva e bloqueante. O tempo de execução
 * depende linearmente de N e EPOCHS.
 *
 * @param void Não recebe parâmetros (usa dados globais/flash).
 * @return void Não retorna valor (atualiza variáveis estáticas globais).
 *
 * @sideeffect Atualiza as variáveis estáticas `g_a`, `g_b` e `g_mse`.
 */
void gradient_run(void)
{
    // ---------------------------------------------------------
    // PASSO 1: Pré-processamento (Cálculo das Médias)
    // ---------------------------------------------------------
    float sum_x = 0.0f, sum_y = 0.0f;

    // Percorre todo o dataset armazenado na Flash para somar X e Y
    for(int i = 0; i < N; i++)
    {
        // Acesso à memória Flash é mais lento que RAM, mas permite datasets maiores
        float x = flash_dataset_read_float(i*2);     // Índices pares representam X
        float y = flash_dataset_read_float(i*2 + 1); // Índices ímpares representam Y
        sum_x += x;
        sum_y += y;
    }

    // Calcula o ponto médio (centróide) dos dados
    float mean_x = sum_x / N;
    float mean_y = sum_y / N;

    // ---------------------------------------------------------
    // PASSO 2: Inicialização dos Pesos (Modelo Centralizado)
    // ---------------------------------------------------------
    // Iniciamos 'a' como 0 (reta horizontal) e 'b' na altura média dos dados.
    // Equação temporária: y_pred = a * (x - mean_x) + b
    float a = 0.0f;
    float b = mean_y;

    // ---------------------------------------------------------
    // PASSO 3: Loop de Treinamento (Épocas)
    // ---------------------------------------------------------
    for(int e = 0; e < EPOCHS; e++)
    {
        // Acumuladores para o gradiente (derivada do erro)
        float gradA = 0.0f;
        float gradB = 0.0f;

        // Batch Gradient Descent: Processa todos os pontos antes de atualizar os pesos
        for(int i = 0; i < N; i++)
        {
            float x = flash_dataset_read_float(i*2);
            float y = flash_dataset_read_float(i*2 + 1);

            // TÉCNICA DE CENTRALIZAÇÃO: Trabalhamos com o desvio da média
            float x_cent = x - mean_x;
            
            // Predição do modelo atual
            float y_pred = a * x_cent + b;
            
            // Cálculo do erro (resíduo)
            float err = y_pred - y;

            // Acumula as derivadas parciais da função de custo MSE
            // Gradiente em relação a A: erro * entrada
            gradA += err * x_cent;
            // Gradiente em relação a B: apenas o erro
            gradB += err;
        }

        // Finaliza o cálculo da derivada média
        // O fator 2.0 vem da derivada de (y_pred - y)^2
        gradA = (2.0f * gradA) / N;
        gradB = (2.0f * gradB) / N;

        // Passo de Otimização: Atualiza os pesos na direção oposta ao gradiente
        a -= LR_A * gradA;
        b -= LR_B * gradB;
    }
    
    // ---------------------------------------------------------
    // PASSO 4: Descentralização (Recuperação dos Coeficientes Reais)
    // ---------------------------------------------------------
    // O modelo treinado foi: y = a(x - mean_x) + b_treinado
    // Expandindo: y = ax - a*mean_x + b_treinado
    // Logo, o 'b' real é (b_treinado - a*mean_x)
    
    g_a = a;                 // A inclinação não muda com a translação
    g_b = b - a * mean_x;    // Ajuste do offset

    // ---------------------------------------------------------
    // PASSO 5: Validação (Cálculo do MSE Final)
    // ---------------------------------------------------------
    float sum = 0.0f;
    for(int i = 0; i < N; i++)
    {
        float x = flash_dataset_read_float(i*2);
        float y = flash_dataset_read_float(i*2 + 1);

        // Previsão usando a equação final: y = ax + b
        float y_pred = g_a * x + g_b;
        
        float err = y_pred - y;
        sum += err*err; // Acumula o quadrado do erro
    }

    g_mse = sum / N; // Média final
}

/**
 * @brief Getter para o coeficiente angular 'a'.
 * @return float Valor da inclinação da reta treinada.
 */
float gradient_a(void){ return g_a; }

/**
 * @brief Getter para o coeficiente linear 'b'.
 * @return float Valor do intercepto da reta treinada.
 */
float gradient_b(void){ return g_b; }

/**
 * @brief Getter para o Erro Quadrático Médio (MSE).
 * @return float Valor do erro médio final após o treinamento.
 */
float gradient_mse(void){ return g_mse; }