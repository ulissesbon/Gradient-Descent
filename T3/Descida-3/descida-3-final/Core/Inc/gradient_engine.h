#ifndef GRADIENT_ENGINE_H
#define GRADIENT_ENGINE_H

#include <stdint.h>

// Configurações de treinamento
#define GRADIENT_EPOCHS 30
#define GRADIENT_LR_A   1e-5f
#define GRADIENT_LR_B   1e-5f

// Configuração de subconjunto para cada época (80% dos pontos disponíveis)
#define GRADIENT_SUBSET_PERCENT 80

// Funções principais
void gradient_run(void);
float gradient_a(void);
float gradient_b(void);
float gradient_mse(void);

#endif
