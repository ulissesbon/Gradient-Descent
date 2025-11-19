#ifndef HEATSHRINK_DECODER_H
#define HEATSHRINK_DECODER_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint8_t *input_buf;
    size_t input_size;
    size_t input_idx;

    uint8_t *output_buf;
    size_t output_size;
    size_t output_idx;

    uint16_t window_sz2;
    uint16_t lookahead_sz2;

    uint8_t state;
    uint16_t bit_accumulator;
    uint8_t bit_count;

} heatshrink_decoder;

void heatshrink_decoder_init(heatshrink_decoder *hsd,
                             uint8_t *input, size_t input_size,
                             uint8_t *output, size_t output_size);

int heatshrink_decode(heatshrink_decoder *hsd);

#endif
