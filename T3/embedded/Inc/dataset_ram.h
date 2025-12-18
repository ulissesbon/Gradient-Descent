#ifndef DATASET_RAM_H
#define DATASET_RAM_H

#include <stdint.h>

// Configurações do dataset
#define DATASET_TOTAL_SAMPLES  1000
#define DATASET_SUBSET_PERCENT 55    // 55% dos pontos
#define DATASET_RAM_SAMPLES    ((DATASET_TOTAL_SAMPLES * DATASET_SUBSET_PERCENT) / 100)

// Funções de gerenciamento
void dataset_ram_reset(void);
void dataset_ram_add_point(float x, float y);
float dataset_ram_get_x(uint16_t index);
float dataset_ram_get_y(uint16_t index);
uint16_t dataset_ram_get_count(void);

#endif
