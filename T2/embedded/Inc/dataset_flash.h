#ifndef DATASET_FLASH_H
#define DATASET_FLASH_H

#include "stm32f0xx_hal.h" // Biblioteca de funções da placa
#include <stdint.h>

#define DATASET_FLASH_BASE   0x0800C000U // Cálculo do endereço de memória que não sobreescreva
#define DATASET_MAX_SAMPLES  1000		 // Quantidade de amostras

void flash_dataset_erase(void);
void flash_dataset_write_float(uint32_t index, float value);
float flash_dataset_read_float(uint32_t index);

#endif
