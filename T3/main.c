#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h> // Necessário para clock()

// =========================================================
// CONFIGURAÇÕES
// =========================================================
#define CSV_PATH "data/dataset0.csv"
#define MAX_SAMPLES 5000

#define GRADIENT_EPOCHS 1000       // Ajuste conforme a velocidade do seu PC
#define GRADIENT_LR_A   1e-5f
#define GRADIENT_LR_B   1e-5f
#define GRADIENT_SUBSET_PERCENT 80

// =========================================================
// ESTRUTURAS E DADOS
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

float dataset_get_x(uint16_t i) { return g_dataset[i].x; }
float dataset_get_y(uint16_t i) { return g_dataset[i].y; }

// =========================================================
// UTILS (RANDOM)
// =========================================================
static uint16_t g_lfsr_state = 0xACE1u;

void random_seed(uint32_t seed) {
    if (seed == 0) seed = 1;
    g_lfsr_state = (uint16_t)(seed & 0xFFFF);
}

uint16_t random_uint16(void) {
    uint16_t bit = ((g_lfsr_state >> 0) ^ (g_lfsr_state >> 2) ^
                    (g_lfsr_state >> 3) ^ (g_lfsr_state >> 5)) & 1u;
    g_lfsr_state = (g_lfsr_state >> 1) | (bit << 15);
    return g_lfsr_state;
}

uint16_t random_range(uint16_t max) {
    if (max == 0) return 0;
    uint16_t limit = (0xFFFF / max) * max;
    uint16_t r;
    do { r = random_uint16(); } while (r >= limit);
    return r % max;
}

void shuffle_indices(uint16_t *indices, uint16_t count) {
    for (uint16_t i = count - 1; i > 0; i--) {
        uint16_t j = random_range(i + 1);
        uint16_t temp = indices[i];
        indices[i] = indices[j];
        indices[j] = temp;
    }
}

// =========================================================
// MOTOR DE GRADIENTE (COM MEDIÇÃO INTERNA)
// =========================================================
static float g_final_a = 0.0f;
static float g_final_b = 0.0f;
static float g_final_mse = 0.0f;
static uint16_t g_shuffled_indices[MAX_SAMPLES];

// Variável global para armazenar o tempo puro de cálculo
double g_time_pure_math_sec = 0.0;

void gradient_run(void) {
    uint16_t total_samples = g_dataset_count;
    if (total_samples == 0) return;

    uint16_t samples_per_epoch = (total_samples * GRADIENT_SUBSET_PERCENT) / 100;
    if (samples_per_epoch == 0) samples_per_epoch = 1;

    random_seed((uint32_t)time(NULL));

    // Pré-cálculo de médias (não contamos isso como tempo de treino repetitivo)
    float sum_x = 0.0f, sum_y = 0.0f;
    for (uint16_t i = 0; i < total_samples; i++) {
        sum_x += dataset_get_x(i);
        sum_y += dataset_get_y(i);
    }
    float mean_x = sum_x / total_samples;
    float mean_y = sum_y / total_samples;

    float a = 0.0f;
    float b = mean_y;

    // Variável acumuladora de clocks
    clock_t total_math_clocks = 0; 

    // --- LOOP DE ÉPOCAS ---
    for (uint16_t epoch = 0; epoch < GRADIENT_EPOCHS; epoch++) {
        
        // 1. Preparação (NÃO MEDIR)
        // Reinicia e embaralha índices (overhead do algoritmo estocástico)
        for (uint16_t i = 0; i < total_samples; i++) {
            g_shuffled_indices[i] = i;
        }
        shuffle_indices(g_shuffled_indices, total_samples);

        // ==========================================================
        // INÍCIO DA MEDIÇÃO (Apenas processamento numérico)
        // ==========================================================
        clock_t math_start = clock(); 

        float grad_a = 0.0f;
        float grad_b = 0.0f;

        for (uint16_t i = 0; i < samples_per_epoch; i++) {
            uint16_t idx = g_shuffled_indices[i];

            float x = dataset_get_x(idx);
            float y = dataset_get_y(idx);

            float x_cent = x - mean_x;
            float y_pred = a * x_cent + b;
            float err = y_pred - y;

            grad_a += err * x_cent;
            grad_b += err;
        }

        grad_a = (2.0f * grad_a) / samples_per_epoch;
        grad_b = (2.0f * grad_b) / samples_per_epoch;

        a -= GRADIENT_LR_A * grad_a;
        b -= GRADIENT_LR_B * grad_b;

        clock_t math_end = clock();
        // ==========================================================
        // FIM DA MEDIÇÃO
        // ==========================================================

        // Acumula a diferença
        total_math_clocks += (math_end - math_start);
    }

    // Calcula o tempo total em segundos
    g_time_pure_math_sec = ((double)total_math_clocks) / CLOCKS_PER_SEC;

    // Finalização (cálculo de MSE final também não entra na conta de treino)
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
// LEITURA CSV
// =========================================================
int load_csv(const char* filename) {
    FILE *file = fopen(filename, "r");
    if (!file) return 0;
    
    char line[100];
    int count = 0;
    long pos = ftell(file);
    if (fgets(line, sizeof(line), file)) {
        float d1, d2;
        if (sscanf(line, "%f,%f", &d1, &d2) != 2 && sscanf(line, "%f;%f", &d1, &d2) != 2) {
            // header ignorado
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
    printf("==================================================\n");
    printf(" BENCHMARK (Tempo Puramente Matematico)\n");
    printf("==================================================\n");

    int loaded = load_csv(CSV_PATH);
    if (loaded == 0) {
        printf("Erro: Arquivo %s nao encontrado.\n", CSV_PATH);
        return 1;
    }

    printf("Dataset: %d amostras\n", loaded);
    printf("Epocas : %d\n", GRADIENT_EPOCHS);
    printf("Calculando...\n");

    // Executa o treino
    gradient_run();

    printf("\n==================================================\n");
    printf("RESULTADOS:\n");
    printf("--------------------------------------------------\n");
    printf("Tempo de CALCULO PURO : %.6f s\n", g_time_pure_math_sec);
    printf("Tempo em ms           : %.3f ms\n", g_time_pure_math_sec * 1000.0);
    printf("--------------------------------------------------\n");
    printf("a (Slope)     : %.6f\n", g_final_a);
    printf("b (Intercept) : %.6f\n", g_final_b);
    printf("MSE Final     : %.6f\n", g_final_mse);
    printf("==================================================\n");

    return 0;
}