#include "gradient_engine.h"
#include "heatshrink_decoder.h"
#include "dataset_flash.h"
#include <string.h>

#define BLOCO_MAX 128
#define RAW_BYTES (BLOCO_MAX * 8)

static uint8_t buffer_comp[1024];
static uint8_t buffer_raw[RAW_BYTES];

static float *xbuf = (float *)buffer_raw;
static float *ybuf = (float *)&buffer_raw[BLOCO_MAX * sizeof(float)];

int total_amostras = 1000;

float media_x = 0.0f;
float media_y = 0.0f;

float a_cent = 0.0f;
float b_cent = 0.0f;

// -----------------------------------------------------

static int carregar_bloco_flash(int bloco_idx,
                                uint8_t *out_comp)
{
    uint32_t *meta = (uint32_t *)DATASET_FLASH_ADDR;
    uint32_t offset = meta[bloco_idx];

    uint32_t *meta_size = (uint32_t *)(DATASET_FLASH_ADDR + 1024);
    uint32_t size = meta_size[bloco_idx];

    memcpy(out_comp, (uint8_t *)(DATASET_FLASH_ADDR + offset), size);

    return size;
}

// -----------------------------------------------------

void passada_medias(int blocos)
{
    float sx = 0, sy = 0;
    int n = 0;

    for (int b = 0; b < blocos; b++) {

        int comp_size = carregar_bloco_flash(b, buffer_comp);

        heatshrink_decoder hsd;
        heatshrink_decoder_init(&hsd,
                                buffer_comp, comp_size,
                                buffer_raw, RAW_BYTES);

        int out_bytes = heatshrink_decode(&hsd);
        int amostras = out_bytes / 8;

        for (int i = 0; i < amostras; i++) {
            sx += xbuf[i];
            sy += ybuf[i];
        }
        n += amostras;
    }

    media_x = sx / n;
    media_y = sy / n;
}

// -----------------------------------------------------

void passada_gradiente(int blocos, int epocas)
{
    a_cent = 0;
    b_cent = media_y;

    for (int e = 0; e < epocas; e++) {

        float acc_a = 0.0f;
        float acc_b = 0.0f;

        for (int b = 0; b < blocos; b++) {

            int comp_size = carregar_bloco_flash(b, buffer_comp);

            heatshrink_decoder hsd;
            heatshrink_decoder_init(&hsd,
                                    buffer_comp, comp_size,
                                    buffer_raw, RAW_BYTES);

            int out_bytes = heatshrink_decode(&hsd);
            int amostras = out_bytes / 8;

            for (int i = 0; i < amostras; i++) {
                float xc = xbuf[i] - media_x;
                float pred = a_cent * xc + b_cent;
                float err = pred - ybuf[i];

                acc_a += err * xc;
                acc_b += err;
            }
        }

        acc_a = (2.0f * acc_a) / total_amostras;
        acc_b = (2.0f * acc_b) / total_amostras;

        a_cent -= 1e-5f * acc_a;
        b_cent -= 1e-5f * acc_b;
    }
}

// -----------------------------------------------------

float passada_mse(int blocos)
{
    float soma = 0.0f;

    for (int b = 0; b < blocos; b++) {

        int comp_size = carregar_bloco_flash(b, buffer_comp);

        heatshrink_decoder hsd;
        heatshrink_decoder_init(&hsd,
                                buffer_comp, comp_size,
                                buffer_raw, RAW_BYTES);

        int out_bytes = heatshrink_decode(&hsd);
        int amostras = out_bytes / 8;

        for (int i = 0; i < amostras; i++) {
            float y_pred = a_cent * xbuf[i]
                + (b_cent - a_cent * media_x);

            float err = y_pred - ybuf[i];
            soma += err * err;
        }
    }

    return soma / total_amostras;
}
