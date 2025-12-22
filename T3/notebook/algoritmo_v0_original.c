#include <stdio.h>
#include <string.h>
#include <time.h>

#define DATASET_MAX_SAMPLES 1000
#define GRADIENT_EPOCHS 30000
#define GRADIENT_LR_A 1e-5f
#define GRADIENT_LR_B 1e-5f

#ifndef OPTIMIZATION_NAME
    #define OPTIMIZATION_NAME "Unknown_Optimization"
#endif

// Dados globais (População)
static const int N = DATASET_MAX_SAMPLES;
static float g_x[DATASET_MAX_SAMPLES];
static float g_y[DATASET_MAX_SAMPLES];
static float g_a;
static float g_b;
static float g_mse;

// Funções de suporte ao ambiente (IGUAIS ÀS SUAS ORIGINAIS)
static int carregar_csv(const char *caminho_csv) {
    FILE *fp = fopen(caminho_csv, "r");
    if (!fp) return 0;
    char cabecalho[128];
    if (!fgets(cabecalho, sizeof(cabecalho), fp)) return 0;
    for (int i = 0; i < DATASET_MAX_SAMPLES; i++) {
        if (fscanf(fp, "%f,%f", &g_x[i], &g_y[i]) != 2) break;
    }
    fclose(fp);
    return 1;
}

/* ========================================================================
   ESTA É A FUNÇÃO COM TODA A LÓGICA DENTRO (KERNEL)
   ======================================================================== */
static void gradient_run() {
    
    // 1. CÁLCULO DE MÉDIAS (Necessário para estabilidade numérica/centralização)
    float sum_x = 0.0f, sum_y = 0.0f;
    for (int i = 0; i < N; i++) {
        sum_x += g_x[i];
        sum_y += g_y[i];
    }
    float mean_x = sum_x / (float)N;
    float mean_y = sum_y / (float)N;

    // 2. INICIALIZAÇÃO
    float a = 0.0f;
    float b = mean_y; // Início inteligente para convergir mais rápido

    // 3. LOOP PRINCIPAL DO GRADIENTE (O núcleo da medição)
    for (int e = 0; e < GRADIENT_EPOCHS; e++) {
        float gradA = 0.0f;
        float gradB = 0.0f;

        for (int i = 0; i < N; i++) {
            // Lógica interna: y_pred = a * (x - mean_x) + b
            float x_cent = g_x[i] - mean_x;
            float y_pred = a * x_cent + b;
            float err = y_pred - g_y[i];

            gradA += err * x_cent;
            gradB += err;
        }

        gradA = (2.0f * gradA) / (float)N;
        gradB = (2.0f * gradB) / (float)N;

        // Atualização dos parâmetros
        a -= GRADIENT_LR_A * gradA;
        b -= GRADIENT_LR_B * gradB;
    }

    // 4. DESCENTRALIZAÇÃO (Voltando para a escala original y = ax + b)
    g_a = a;
    g_b = b - (a * mean_x);

    // 5. CÁLCULO DO MSE (Dentro da lógica final)
    float sum = 0.0f;
    for (int i = 0; i < N; i++) 
    {
        float y_pred = g_a * g_x[i] + g_b;
        float err = y_pred - g_y[i];
        sum += err * err;
    }
    g_mse = sum / (float)N;
}

// No seu algoritmo_v0_original.c

static void salvar_parametros_finais(double tempo_cpu) {
    // APENAS ISTO, sem texto extra, para o Bash capturar
    printf("%s,%.6f,%.6f,%.6f,%.6f", OPTIMIZATION_NAME, g_a, g_b, g_mse, tempo_cpu);
}

int main(void) {
    // Remova todos os printf("=======...") do main
    if (!carregar_csv("../data/original/dataset_original0.csv")) return 1;

    clock_t inicio = clock();
    gradient_run(); // Sua função de treino
    clock_t fim = clock();
    
    double tempo_cpu = ((double) (fim - inicio)) / CLOCKS_PER_SEC;
    // Chama a função que imprime a linha do CSV
    salvar_parametros_finais(tempo_cpu);

    return 0;
}