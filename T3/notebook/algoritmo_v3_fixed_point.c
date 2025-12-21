/*
* @(#)main.c    1.0 02/11/2025
*
* Copyright 2025 by Raquel Maciel e Ulisses Bonfim
* Universidade IFCE - Engenharia de Computação (DTEL)
* All rights reserved.
*
* Este software é parte de um trabalho acadêmico para a disciplina de
* Sistemas Embarcados. Uso livre para fins educacionais.
*/

/*
* ============================================================================
* REGRESSÃO LINEAR COM DESCIDA DE GRADIENTE
* ============================================================================
* 
* DESCRIÇÃO GERAL:
*   Implementa regressão linear (y ≈ a*x + b) usando o algoritmo de descida
*   de gradiente (gradient descent) com centralização de dados.
* 
* CARACTERÍSTICAS:
*   - Leitura de datasets CSV
*   - Treinamento com descida de gradiente
*   - Exportação do histórico para visualização
*   - Armazenamento estático (~8 KB)
* 
* ENTRADA:
*   Arquivo CSV no formato:
*     x,y
*     1.000000,3.190011
*     2.000000,5.093333
*     ...
*   Onde x = variável independente, y = variável dependente
* 
* SAÍDA:
*   - Terminal: Parâmetros finais (a, b) e estatísticas
*   - Arquivo CSV: Histórico completo do treinamento (época, a, b, MSE)
* 
* COMO USAR:
*   Compilar: gcc -o main main.c -lm
*   Executar: ./main
*   
* AUTORES:
*   Raquel Maciel
*   Ulisses Bonfim
* 
* DATA: Novembro 2024
* 
* CONTEXTO:
*   Trabalho de Sistemas Embarcados
*   IFCE - Campus Fortaleza
* 
* PLATAFORMA ALVO:
*   Linux/Windows/MacOS com GCC
*   Requer biblioteca matemática padrão (-lm)
* 
* ============================================================================
*/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

/* ========================================================================== */
/* CONFIGURAÇÕES E CONSTANTES                                                 */
/* ========================================================================== */

/* Número de amostras */
#define QUANTIDADE_AMOSTRAS 750

/* Número de iterações de treinamento */
#define EPOCAS_TREINAMENTO 30000

/* Nome da melhoria para debugar */
#define SCALE 1000
#define NOME_MELHORIA "v3_fixed_point"

/* 
Taxas de aprendizado (learning rates):
- Taxa menor para inclinação (a) pois x pode ter valores grandes
- Taxa para intercepto (b) pode ser igual ou diferente
*/
#define TAXA_APRENDIZADO_INCLINACAO  1e-5
#define TAXA_APRENDIZADO_INTERCEPTO  1e-5

/* Intervalo para exibir progresso (a cada N épocas) */
#define INTERVALO_DE_LOG 5000


/* ========================================================================== */
/* ARMAZENAMENTO ESTÁTICO (~8 KB)                                             */
/* ========================================================================== */

/*
  Vetores globais para armazenar os dados:
  - g_x: valores de entrada (eixo horizontal)
  - g_y: valores de saída/target (eixo vertical)
  - g_x_centralizado: valores de x após centralização (x - média)
  - g_n: quantidade efetiva de amostras carregadas
  */
 
// Armazenamento em inteiros para evitar FPU
static int32_t g_x_fp[QUANTIDADE_AMOSTRAS];
static int32_t g_y_fp[QUANTIDADE_AMOSTRAS];
static int32_t g_xc_fp[QUANTIDADE_AMOSTRAS];
static float g_x[QUANTIDADE_AMOSTRAS];
static float g_y[QUANTIDADE_AMOSTRAS];
static float g_x_centralizado[QUANTIDADE_AMOSTRAS];
static int   g_n = 0;


/* ========================================================================== */
/* FUNÇÕES DE CARREGAMENTO DE DADOS                                           */
/* ========================================================================== */

/**
 * Carrega dados de um arquivo CSV para os arrays globais.
 * 
 * Formato esperado:
 *   x,y
 *   1.0,3.2
 *   2.0,5.1
 *   ...
 * 
 * @param caminho_csv Caminho do arquivo CSV a ser lido
 * @return Número de amostras carregadas, ou 0 em caso de erro
 * @note Esta função modifica (escreve) nas variáveis globais g_x, g_y e g_n.
 */
static int carregar_csv(const char *caminho_csv) {
    /* Abrir arquivo para leitura */
    FILE *fp = fopen(caminho_csv, "r");
    if (!fp) {
        fprintf(stderr, " Erro: não foi possível abrir '%s'\n", caminho_csv);
        return 0;
    }

    /* Ler e descartar a linha de cabeçalho */
    char cabecalho[128];
    if (!fgets(cabecalho, sizeof(cabecalho), fp)) {
        fprintf(stderr, " Erro: arquivo vazio ou corrompido.\n");
        fclose(fp);
        return 0;
    }

    /* Ler todas as linhas de dados */
    for (int i = 0; i < QUANTIDADE_AMOSTRAS; i++) {
        float xd, yd;
        
        /* Tentar ler um par (x, y) */
        if (fscanf(fp, "%f,%f", &xd, &yd) != 2) {
            fprintf(stderr, " Erro ao ler a linha %d do CSV.\n", i + 2);
            fclose(fp);
            return 0;
        }
        
        /* Armazenar nos arrays globais */
        g_x[i] = xd;
        g_y[i] = yd;
    }

    fclose(fp);
    g_n = QUANTIDADE_AMOSTRAS;
    
    return g_n;
}


/* ========================================================================== */
/* FUNÇÕES MATEMÁTICAS AUXILIARES                                             */
/* ========================================================================== */

/**
 * Calcula a média aritmética de um vetor.
 * 
 * @param v Ponteiro para o vetor
 * @param n Número de elementos
 * @return Média dos valores
 */
static float calcular_media(const float *v, int n) {
    float soma = 0.0;
    
    for (int i = 0; i < n; i++) {
        soma += v[i];
    }
    
    return soma / (float)n;
}

/**
 * Calcula o Erro Quadrático Médio (Mean Squared Error).
 * 
 * MSE = (1/N) * Σ(y_pred - y_real)²
 * 
 * @param a Coeficiente angular (inclinação)
 * @param b Coeficiente linear (intercepto)
 * @return Valor do MSE
 * @note Esta função lê as variáveis globais g_x, g_y e g_n.
 */
static float calcular_mse(float a, float b) {
    float soma_erros_quadrados = 0.0;
    
    for (int i = 0; i < g_n; i++) {
        float y_predito = a * g_x[i] + b;
        float erro = y_predito - g_y[i];
        soma_erros_quadrados += erro * erro;
    }
    
    return soma_erros_quadrados / (float)g_n;
}


/* ========================================================================== */
/* ALGORITMO DE DESCIDA DE GRADIENTE                                          */
/* ========================================================================== */

/**
 * Executa uma época (iteração) da descida de gradiente.
 * 
 * Modelo no espaço centralizado:
 *   y ≈ a_c * (x - média_x) + b_c
 * 
 * Gradientes (derivadas parciais da função de custo):
 *   ∂J/∂a_c = (2/N) * Σ [erro * (x - média_x)]
 *   ∂J/∂b_c = (2/N) * Σ [erro]
 * 
 * Atualização dos parâmetros:
 *   a_c ← a_c - taxa_a * (∂J/∂a_c)
 *   b_c ← b_c - taxa_b * (∂J/∂b_c)
 * 
 * @param a_centralizado Ponteiro para o coeficiente angular (centralizado)
 * @param b_centralizado Ponteiro para o coeficiente linear (centralizado)
 * @note Esta função lê as variáveis globais g_x_centralizado, g_y e g_n.
 */

static inline void executar_epoca_gradiente(float *a_c, float *b_c) {
    float grad_a = 0.0f, grad_b = 0.0f;
    float a = *a_c; 
    float b = *b_c;

    /* OTIMIZAÇÃO: Loop Unrolling fator 5 */
    for (int i = 0; i < g_n; i += 5) {
        float e0 = (a * g_x_centralizado[i]) + b - g_y[i];
        grad_a += e0 * g_x_centralizado[i]; grad_b += e0;
        
        float e1 = (a * g_x_centralizado[i+1]) + b - g_y[i+1];
        grad_a += e1 * g_x_centralizado[i+1]; grad_b += e1;

        float e2 = (a * g_x_centralizado[i+2]) + b - g_y[i+2];
        grad_a += e2 * g_x_centralizado[i+2]; grad_b += e2;

        float e3 = (a * g_x_centralizado[i+3]) + b - g_y[i+3];
        grad_a += e3 * g_x_centralizado[i+3]; grad_b += e3;

        float e4 = (a * g_x_centralizado[i+4]) + b - g_y[i+4];
        grad_a += e4 * g_x_centralizado[i+4]; grad_b += e4;
    }
    *a_c -= TAXA_APRENDIZADO_INCLINACAO * (2.0f * grad_a / (float)g_n);
    *b_c -= TAXA_APRENDIZADO_INTERCEPTO * (2.0f * grad_b / (float)g_n);
}

/* ========================================================================== */
/* FUNÇÃO DE TREINAMENTO COMPLETO                                             */
/* ========================================================================== */

/**
 * Treina o modelo de regressão linear usando descida de gradiente.
 * 
 * Passos do algoritmo:
 *   1. Calcular média de X e Y
 *   2. Centralizar X (subtrair média)
 *   3. Inicializar parâmetros (a=0, b=média_y)
 *   4. Executar épocas de gradiente descendente
 *   5. Converter parâmetros para escala original
 * 
 * Conversão para escala original:
 *   a_original = a_centralizado
 *   b_original = b_centralizado - a_centralizado * média_x
 * 
 * @param a_original Ponteiro para armazenar coeficiente angular final
 * @param b_original Ponteiro para armazenar coeficiente linear final
 * @note Esta função lê g_x, g_y, g_n e modifica (escreve) em g_x_centralizado.
 * @note Esta função chama outras funções que também acessam globais 
 * (calcular_media, executar_epoca_gradiente, calcular_mse).
 */
static void treinar_modelo(float *a_final, float *b_final) {
    // Conversão inicial: float -> fixed point
    for(int i=0; i<g_n; i++) {
        g_x_fp[i] = (int32_t)(g_x[i] * SCALE);
        g_y_fp[i] = (int32_t)(g_y[i] * SCALE);
    }
    
    int32_t a = 0;
    int32_t b = (int32_t)(calcular_media(g_y, g_n) * SCALE);
    int32_t media_x = (int32_t)(calcular_media(g_x, g_n) * SCALE);
    
    for(int i=0; i<g_n; i++) g_xc_fp[i] = g_x_fp[i] - media_x;
    
    for (int epoca = 0; epoca < EPOCAS_TREINAMENTO; epoca++) {
        int64_t grad_a = 0, grad_b = 0; // 64 bits para evitar overflow no acúmulo
        for (int i = 0; i < g_n; i++) {
            // y_pred = (a * x) / SCALE + b
            int32_t y_pred = (int32_t)(((int64_t)a * g_xc_fp[i]) / SCALE) + b;
            int32_t erro = y_pred - g_y_fp[i];
            grad_a += (int64_t)erro * g_xc_fp[i] / SCALE;
            grad_b += erro;
        }
        // Atualização usando a taxa de aprendizado (1e-5 -> / 100000)
        a -= (int32_t)((2 * grad_a / g_n) / 100000);
        b -= (int32_t)((2 * grad_b / g_n) / 100000);
    }
    *a_final = a;
    *b_final = b - (int32_t)(((int64_t)a * media_x) / SCALE);

    *a_final /= SCALE; 
    *b_final /= SCALE;
}


/**
 * Salva os parâmetros finais em arquivo para comparação.
 * 
 * @param a Coeficiente angular final
 * @param b Coeficiente linear final
 * @param mse Erro quadrático médio final
 * @param tempo_cpu Tempo de CPU gasto no treinamento
 */
static void salvar_parametros_finais(float a, float b, float mse, double tempo_cpu) {
    FILE *fp = fopen("results.txt", "a");
    if (fp) {
        fprintf(fp, "(Notebook) %s: a=%.6f, b=%.6f, mse=%.6f, tempo_cpu=%.6f\n", 
                NOME_MELHORIA, a, b, mse, tempo_cpu);
        fclose(fp);
    }
}

/* ========================================================================== */
/* FUNÇÃO PRINCIPAL                                                           */
/* ========================================================================== */

/**
 * Função principal do programa.
 * 
 * Processa todos os datasets configurados:
 *   1. Carrega dados do CSV
 *   2. Treina modelo com descida de gradiente
 *   3. Exibe resultados finais
 */
int main(void) {
    printf("\n");
    printf("=======================================================================\n");
    printf("  REGRESSÃO LINEAR COM DESCIDA DE GRADIENTE EM C\n");
    printf("=======================================================================\n");
    printf("  Autores: Raquel Maciel e Ulisses Bonfim\n");
    printf("=======================================================================\n");
    printf("\n");

    /* Construir caminho do arquivo */
    char caminho_dataset[256];
    snprintf(caminho_dataset, sizeof(caminho_dataset), 
                "../data/shuffle/dataset_randomico0.csv");

    /* Carregar dados do CSV */
    if (!carregar_csv(caminho_dataset)) {
        fprintf(stderr, " Falha ao carregar dataset\n");
        return 1;
    }

    /* Variáveis para medição de tempo */
    clock_t inicio, fim;
    double tempo_cpu;

    /* Treinar modelo - MEDIÇÃO APENAS DO ALGORITMO */
    float a_treinado = 0.0;
    float b_treinado = 0.0;
    
    inicio = clock(); 
    treinar_modelo(&a_treinado, &b_treinado);
    fim = clock();
    
    tempo_cpu = ((double) (fim - inicio)) / CLOCKS_PER_SEC;
    printf("\n  TEMPO DE EXECUÇÃO DO ALGORITMO: %f segundos\n", tempo_cpu);

    // Calcular MSE final
    float mse_final = calcular_mse(a_treinado, b_treinado);

    // Salvar resultados
    salvar_parametros_finais(a_treinado, b_treinado, mse_final, tempo_cpu);

    return 0;
}