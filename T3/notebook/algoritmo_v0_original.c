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

/* ========================================================================== */
/* CONFIGURAÇÕES E CONSTANTES                                                 */
/* ========================================================================== */

/* Quantidade de datasets a processar */
#define QUANTIDADE_ARQUIVOS 4

/* Número de amostras por dataset */
#define QUANTIDADE_AMOSTRAS 750

/* Número de iterações de treinamento */
#define EPOCAS_TREINAMENTO 30000

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
static void executar_epoca_gradiente(float *a_centralizado,
                                     float *b_centralizado)
{
    float gradiente_a = 0.0;
    float gradiente_b = 0.0;

    /* Calcular gradientes acumulando contribuições de cada amostra */
    for (int i = 0; i < g_n; i++) {
        float x_cent = g_x_centralizado[i];  /* x - média(x) */
        float y_predito = (*a_centralizado) * x_cent + (*b_centralizado);
        float erro = y_predito - g_y[i];

        /* Acumular gradientes */
        gradiente_a += erro * x_cent;  /* ∂J/∂a ∝ Σ(erro * x_cent) */
        gradiente_b += erro;           /* ∂J/∂b ∝ Σ(erro) */
    }

    /* Normalizar gradientes pela quantidade de amostras */
    gradiente_a = (2.0 * gradiente_a) / (float)g_n;
    gradiente_b = (2.0 * gradiente_b) / (float)g_n;

    /* Atualizar parâmetros (descida do gradiente) */
    *a_centralizado -= TAXA_APRENDIZADO_INCLINACAO * gradiente_a;
    *b_centralizado -= TAXA_APRENDIZADO_INTERCEPTO * gradiente_b;
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
 *   6. Salvar histórico em CSV
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
static void treinar_modelo(float *a_original,
                          float *b_original)
{
    /* Passo 1: Calcular estatísticas dos dados */
    float media_x = calcular_media(g_x, g_n);
    float media_y = calcular_media(g_y, g_n);
    
    /* Passo 2: Centralizar valores de X */
    for (int i = 0; i < g_n; i++) {
        g_x_centralizado[i] = g_x[i] - media_x;
    }

    /* Passo 3: Inicializar parâmetros */
    float a_centralizado = 0.0;      /* Inclinação começa em zero */
    float b_centralizado = media_y;  /* Intercepto começa na média de Y */

    /* Passo 4: Loop principal de treinamento */
    for (int epoca = 0; epoca < EPOCAS_TREINAMENTO; epoca++) {
        /* Executar uma época de descida de gradiente */
        executar_epoca_gradiente(&a_centralizado, &b_centralizado);

        /* Converter para escala original temporariamente */
        float a_temp = a_centralizado;
        float b_temp = b_centralizado - a_centralizado * media_x;
        
        /* Calcular erro (MSE) na escala original */
        float mse = calcular_mse(a_temp, b_temp);
    }

    /* Passo 5: Converter parâmetros finais para escala original */
    *a_original = a_centralizado;
    *b_original = b_centralizado - a_centralizado * media_x;
}

/**
 * Salva os parâmetros finais em arquivo para comparação.
 * 
 * @param indice_dataset Número do dataset
 * @param a Coeficiente angular final
 * @param b Coeficiente linear final
 * @param mse Erro quadrático médio final
 * @param tempo_cpu Tempo de CPU gasto no treinamento
 */
static void salvar_parametros_finais(int indice_dataset, float a, float b, float mse, double tempo_cpu) {
    FILE *fp = fopen("results.txt", "a");
    if (fp) {
        fprintf(fp, "(Notebook) Dataset %d: a=%.6f, b=%.6f, mse=%.6f, tempo_cpu=%.6f\n", 
                indice_dataset, a, b, mse, tempo_cpu);
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

    /* Processar cada dataset */
    for (int i = 0; i < QUANTIDADE_ARQUIVOS; i++) {
        /* Construir caminho do arquivo */
        char caminho_dataset[256];
        snprintf(caminho_dataset, sizeof(caminho_dataset), 
                 "data/shuffle/dataset_randomico%d.csv", i);

        printf(" Dataset %d: %s\n", i, caminho_dataset);
        printf("-----------------------------------------------------------------------\n");

        /* Carregar dados do CSV */
        if (!carregar_csv(caminho_dataset)) {
            fprintf(stderr, " Falha ao carregar dataset %d\n\n", i);
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
        salvar_parametros_finais(i, a_treinado, b_treinado, mse_final, tempo_cpu);
    }

    return 0;
}