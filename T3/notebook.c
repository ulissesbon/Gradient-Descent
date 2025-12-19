#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h> // <--- NECESSÁRIO PARA MEDIÇÃO DE TEMPO

// =========================================================
// CONFIGURAÇÕES GERAIS
// =========================================================
#define CSV_PATH "data/dataset0.csv"
#define MAX_SAMPLES 5000

// Aumentei um pouco as épocas para garantir que o tempo seja mensurável
// se o PC for muito rápido. Ajuste conforme necessário.
#define GRADIENT_EPOCHS 1000 
#define GRADIENT_LR_A   1e-6f  // Ajustado LR para mais épocas
#define GRADIENT_LR_B   1e-6f

// =========================================================
// ESTRUTURA DE DADOS
// =========================================================
typedef struct {
    float x;
    float y;
} Point;

static Point g_dataset[MAX_SAMPLES];
static uint16_t g_dataset_count = 0;

void dataset_add_point(float x, float y) {
    if (g_dataset_count < MAX_SAMPLES) {
        g_dataset[g_dataset_count].x = x;
        g_dataset[g_dataset_count].y = y;
        g_dataset_count++;
    }
}

// =========================================================
// MOTOR DE GRADIENTE (BATCH)
// =========================================================
static float g_final_a = 0.0f;
static float g_final_b = 0.0f;
static float g_final_mse = 0.0f;

void gradient_run(void) {
    uint16_t total_samples = g_dataset_count;
    if (total_samples == 0) return;

    float sum_x = 0.0f, sum_y = 0.0f;
    for (uint16_t i = 0; i < total_samples; i++) {
        sum_x += g_dataset[i].x;
        sum_y += g_dataset[i].y;
    }
    float mean_x = sum_x / total_samples;
    float mean_y = sum_y / total_samples;

    float a = 0.0f;
    float b = mean_y;

    // Comentei os prints internos para não sujar a medição de tempo da CPU
    // printf("\n--- Iniciando Treinamento ---\n"); 

    for (uint16_t epoch = 0; epoch < GRADIENT_EPOCHS; epoch++) {
        float grad_a = 0.0f;
        float grad_b = 0.0f;

        for (uint16_t i = 0; i < total_samples; i++) {
            float x_cent = g_dataset[i].x - mean_x;
            float y_pred = a * x_cent + b;
            float err = y_pred - g_dataset[i].y;
            grad_a += err * x_cent;
            grad_b += err;
        }

        grad_a = (2.0f * grad_a) / total_samples;
        grad_b = (2.0f * grad_b) / total_samples;

        a -= GRADIENT_LR_A * grad_a;
        b -= GRADIENT_LR_B * grad_b;
    }

    g_final_a = a;
    g_final_b = b - a * mean_x;

    float sum_sq_err = 0.0f;
    for (uint16_t i = 0; i < total_samples; i++) {
        float y_pred = g_final_a * g_dataset[i].x + g_final_b;
        float err = y_pred - g_dataset[i].y;
        sum_sq_err += err * err;
    }
    g_final_mse = sum_sq_err / total_samples;
}

// =========================================================
// LOAD CSV
// =========================================================
int load_csv(const char* filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("ERRO: Nao foi possivel abrir '%s'\n", filename);
        return 0;
    }
    char line[100];
    int count = 0;
    long pos = ftell(file);
    if (fgets(line, sizeof(line), file)) {
        float d1, d2;
        if (sscanf(line, "%f,%f", &d1, &d2) != 2 && sscanf(line, "%f;%f", &d1, &d2) != 2) {
            // header
        } else {
            fseek(file, pos, SEEK_SET);
        }
    }
    float x, y;
    while (fgets(line, sizeof(line), file) && count < MAX_SAMPLES) {
        if (sscanf(line, "%f,%f", &x, &y) == 2 || sscanf(line, "%f;%f", &x, &y) == 2) {
            dataset_add_point(x, y);
            count++;
        }
    }
    fclose(file);
    return count;
}

// =========================================================
// MAIN
// =========================================================
int main() {
    printf("========================================\n");
    printf("   BENCHMARK REGRESSAO LINEAR (C)       \n");
    printf("========================================\n");

    int loaded = load_csv(CSV_PATH);
    if (loaded == 0) return 1;
    printf("Dados carregados: %d amostras\n", loaded);
    printf("Configuracao: %d epocas\n", GRADIENT_EPOCHS);

    // --- BLOCO DE MEDIÇÃO DE TEMPO ---
    printf("Calculando...\n");
    
    clock_t start_time = clock(); // <--- Inicia contagem de clocks da CPU
    
    gradient_run();               // <--- Executa a função pesada
    
    clock_t end_time = clock();   // <--- Para contagem
    // ---------------------------------

    // Cálculo: (Final - Inicial) / Clocks por Segundo
    double time_taken = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;

    printf("\n========================================\n");
    printf("RESULTADOS DE PERFORMANCE:\n");
    printf("Tempo de CPU  : %.6f segundos\n", time_taken);
    printf("Tempo (ms)    : %.3f ms\n", time_taken * 1000.0);
    printf("========================================\n");
    
    printf("Slope (a)     : %.6f\n", g_final_a);
    printf("Intercept (b) : %.6f\n", g_final_b);
    printf("MSE Final     : %.6f\n", g_final_mse);

    return 0;
}