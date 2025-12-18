#ifndef RANDOM_UTILS_H
#define RANDOM_UTILS_H

#include <stdint.h>

// Inicializa o gerador com uma semente
void random_seed(uint32_t seed);

// Retorna um número aleatório de 16 bits
uint16_t random_uint16(void);

// Retorna um número aleatório no intervalo [0, max)
uint16_t random_range(uint16_t max);

// Embaralha um array de índices usando Fisher-Yates
void shuffle_indices(uint16_t *indices, uint16_t count);

#endif
