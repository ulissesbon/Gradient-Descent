
#include "gradient_engine.h"
#include "dataset_flash.h"
#include <math.h>

static float g_a, g_b, g_mse;
static const int N = DATASET_MAX_SAMPLES; // Quantidade de amostras
#define EPOCHS 30	// Quantidade de épocas
#define LR_A 1e-5	// Tamanho do passo
#define LR_B 1e-5

void gradient_run(void)
{
    // Calcular médias
    float sum_x = 0.0f, sum_y = 0.0f;

    for(int i = 0; i < N; i++)
    {
        float x = flash_dataset_read_float(i*2);
        float y = flash_dataset_read_float(i*2 + 1);
        sum_x += x;
        sum_y += y;
    }

    float mean_x = sum_x / N;
    float mean_y = sum_y / N;

    // Inicializar regressão centralizada
    float a = 0.0f;
    float b = mean_y;

    // Epocas
    for(int e = 0; e < EPOCHS; e++)
    {
        float gradA = 0.0f;
        float gradB = 0.0f;

        for(int i = 0; i < N; i++)
        {
            float x = flash_dataset_read_float(i*2);
            float y = flash_dataset_read_float(i*2 + 1);

            float x_cent = x - mean_x;
            float y_pred = a * x_cent + b;
            float err = y_pred - y;

            gradA += err * x_cent;
            gradB += err;
        }

        gradA = (2.0f * gradA) / N;
        gradB = (2.0f * gradB) / N;

        a -= LR_A * gradA;
        b -= LR_B * gradB;
    }

    // Converter de volta ao espaço não-centralizado
    g_a = a;
    g_b = b - a * mean_x;

    // MSE final
    float sum = 0.0f;
    for(int i = 0; i < N; i++)
    {
        float x = flash_dataset_read_float(i*2);
        float y = flash_dataset_read_float(i*2 + 1);

        float y_pred = g_a * x + g_b;
        float err = y_pred - y;
        sum += err*err;
    }

    g_mse = sum / N;
}

float gradient_a(void){ return g_a; }
float gradient_b(void){ return g_b; }
float gradient_mse(void){ return g_mse; }
