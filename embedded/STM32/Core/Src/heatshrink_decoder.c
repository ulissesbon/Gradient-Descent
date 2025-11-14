#include "heatshrink_decoder.h"

void heatshrink_decoder_init(heatshrink_decoder *hsd,
                             uint8_t *input, size_t input_size,
                             uint8_t *output, size_t output_size)
{
    hsd->input_buf = input;
    hsd->input_size = input_size;
    hsd->input_idx = 0;

    hsd->output_buf = output;
    hsd->output_size = output_size;
    hsd->output_idx = 0;

    hsd->window_sz2 = 8;
    hsd->lookahead_sz2 = 4;

    hsd->state = 0;
    hsd->bit_accumulator = 0;
    hsd->bit_count = 0;
}

static int get_bit(heatshrink_decoder *hsd, uint8_t *bit)
{
    if (hsd->bit_count == 0) {
        if (hsd->input_idx >= hsd->input_size) return 0;
        hsd->bit_accumulator = hsd->input_buf[hsd->input_idx++];
        hsd->bit_count = 8;
    }
    *bit = (hsd->bit_accumulator >> 7) & 1;
    hsd->bit_accumulator <<= 1;
    hsd->bit_count--;
    return 1;
}

static int get_bits(heatshrink_decoder *hsd, uint8_t count, uint16_t *out)
{
    uint16_t r = 0;
    uint8_t b;
    for (uint8_t i = 0; i < count; i++) {
        if (!get_bit(hsd, &b)) return 0;
        r = (r << 1) | b;
    }
    *out = r;
    return 1;
}

int heatshrink_decode(heatshrink_decoder *hsd)
{
    uint8_t is_literal;
    uint16_t length, offset;

    while (hsd->output_idx < hsd->output_size) {
        if (!get_bit(hsd, &is_literal)) return hsd->output_idx;

        if (is_literal) {
            if (hsd->input_idx >= hsd->input_size) return hsd->output_idx;
            hsd->output_buf[hsd->output_idx++] =
                hsd->input_buf[hsd->input_idx++];
        } else {
            if (!get_bits(hsd, hsd->lookahead_sz2, &length)) return hsd->output_idx;
            if (!get_bits(hsd, hsd->window_sz2, &offset)) return hsd->output_idx;

            length += 1;
            for (uint16_t i = 0; i < length; i++) {
                if (offset > hsd->output_idx) break;
                hsd->output_buf[hsd->output_idx] =
                    hsd->output_buf[hsd->output_idx - offset];
                hsd->output_idx++;
                if (hsd->output_idx >= hsd->output_size) break;
            }
        }
    }
    return hsd->output_idx;
}
