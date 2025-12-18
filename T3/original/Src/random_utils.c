#include "random_utils.h"

// Estado do LFSR (Linear Feedback Shift Register) de 16 bits
static uint16_t g_lfsr_state = 0xACE1u;  // Semente inicial não-zero

void random_seed(uint32_t seed)
{
    if (seed == 0) seed = 1;  // LFSR não pode ser zero
    g_lfsr_state = (uint16_t)(seed & 0xFFFF);
}

uint16_t random_uint16(void)
{
    // LFSR de 16 bits com taps em 16, 15, 13, 4
    // Período máximo: 65535
    uint16_t bit = ((g_lfsr_state >> 0) ^ (g_lfsr_state >> 2) ^
                    (g_lfsr_state >> 3) ^ (g_lfsr_state >> 5)) & 1u;
    g_lfsr_state = (g_lfsr_state >> 1) | (bit << 15);
    return g_lfsr_state;
}

uint16_t random_range(uint16_t max)
{
    if (max == 0) return 0;

    // Método simples: módulo com rejeição para uniformidade
    uint16_t limit = (0xFFFF / max) * max;
    uint16_t r;

    do {
        r = random_uint16();
    } while (r >= limit);

    return r % max;
}

void shuffle_indices(uint16_t *indices, uint16_t count)
{
    // Algoritmo Fisher-Yates para embaralhamento in-place
    for (uint16_t i = count - 1; i > 0; i--)
    {
        uint16_t j = random_range(i + 1);

        // Troca indices[i] com indices[j]
        uint16_t temp = indices[i];
        indices[i] = indices[j];
        indices[j] = temp;
    }
}
