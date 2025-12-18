#include "dataset_ram.h"
#include <string.h>

// Buffers na RAM para armazenar o subconjunto
static float g_x_data[DATASET_RAM_SAMPLES];
static float g_y_data[DATASET_RAM_SAMPLES];
static uint16_t g_sample_count = 0;

void dataset_ram_reset(void)
{
    g_sample_count = 0;
    memset(g_x_data, 0, sizeof(g_x_data));
    memset(g_y_data, 0, sizeof(g_y_data));
}

void dataset_ram_add_point(float x, float y)
{
    if (g_sample_count < DATASET_RAM_SAMPLES)
    {
        g_x_data[g_sample_count] = x;
        g_y_data[g_sample_count] = y;
        g_sample_count++;
    }
}

float dataset_ram_get_x(uint16_t index)
{
    if (index < g_sample_count)
        return g_x_data[index];
    return 0.0f;
}

float dataset_ram_get_y(uint16_t index)
{
    if (index < g_sample_count)
        return g_y_data[index];
    return 0.0f;
}

uint16_t dataset_ram_get_count(void)
{
    return g_sample_count;
}
