#include "gradient_engine.h"
#include "dataset_ram.h"
#include "random_utils.h"
#include "stm32f0xx_hal.h"
#include <string.h>

static float g_a, g_b, g_mse;
static uint16_t g_shuffled_indices[DATASET_RAM_SAMPLES];

void gradient_run(void)
{
    uint16_t total_samples = dataset_ram_get_count();
    if (total_samples == 0) return;

    uint16_t samples_per_epoch = (total_samples * GRADIENT_SUBSET_PERCENT) / 100;
    if (samples_per_epoch == 0) samples_per_epoch = 1;

    // Semente baseada em tempo (sem warnings)
    random_seed(HAL_GetTick());

    // Calcular médias globais
    float sum_x = 0.0f, sum_y = 0.0f;
    for (uint16_t i = 0; i < total_samples; i++)
    {
        sum_x += dataset_ram_get_x(i);
        sum_y += dataset_ram_get_y(i);
    }
    float mean_x = sum_x / total_samples;
    float mean_y = sum_y / total_samples;

    // Inicializar parâmetros
    float a = 0.0f;
    float b = mean_y;

    // Loop de épocas
    for (uint16_t epoch = 0; epoch < GRADIENT_EPOCHS; epoch++)
    {
        // Criar e embaralhar índices
        for (uint16_t i = 0; i < total_samples; i++)
        {
            g_shuffled_indices[i] = i;
        }
        shuffle_indices(g_shuffled_indices, total_samples);

        // Calcular gradientes
        float grad_a = 0.0f;
        float grad_b = 0.0f;

        for (uint16_t i = 0; i < samples_per_epoch; i++)
        {
            uint16_t idx = g_shuffled_indices[i];

            float x = dataset_ram_get_x(idx);
            float y = dataset_ram_get_y(idx);

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
    }

    // Converter para espaço não-centralizado
    g_a = a;
    g_b = b - a * mean_x;

    // Calcular MSE final
    float sum_sq_err = 0.0f;
    for (uint16_t i = 0; i < total_samples; i++)
    {
        float x = dataset_ram_get_x(i);
        float y = dataset_ram_get_y(i);

        float y_pred = g_a * x + g_b;
        float err = y_pred - y;
        sum_sq_err += err * err;
    }

    g_mse = sum_sq_err / total_samples;
}

float gradient_a(void) { return g_a; }
float gradient_b(void) { return g_b; }
float gradient_mse(void) { return g_mse; }
