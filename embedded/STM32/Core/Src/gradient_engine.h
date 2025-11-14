#ifndef GRADIENT_ENGINE_H
#define GRADIENT_ENGINE_H

#include <stdint.h>

//
// Número de blocos comprimidos na flash
// (Para 1000 amostras, com blocos de 128, usamos 8 blocos.
//
#define NUM_BLOCOS 8

//
// Variáveis globais calculadas durante o processo
// usadas depois no main.c
//
extern float media_x;
extern float media_y;

extern float a_cent;
extern float b_cent;

extern int total_amostras;

//
// Passadas do algoritmo de gradiente descendente
// (idêntico ao seu código original, mas lendo blocos da flash)
//
void passada_medias(int blocos);
void passada_gradiente(int blocos, int epocas);
float passada_mse(int blocos);

#endif
