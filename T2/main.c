#include <stdio.h>
#include <string.h>

/* ========================================================================== */
/* CONFIGURAÇÕES                                                              */
/* ========================================================================== */

#define QUANTIDADE_AMOSTRAS 1000
#define QUANTIDADE_AMOSTRAS_RANDOM 750
#define EPOCAS_TREINAMENTO 3000
#define QUANTIDADE_ARQUIVOS 4

#define TAXA_APRENDIZADO_INCLINACAO  1e-5
#define TAXA_APRENDIZADO_INTERCEPTO  1e-5

/* Buffers para dataset ORIGINAL */
static float g_x_original[QUANTIDADE_AMOSTRAS];
static float g_y_original[QUANTIDADE_AMOSTRAS];
static float g_x_orig_centralizado[QUANTIDADE_AMOSTRAS];
static int   g_n_original = 0;

/* Buffers para dataset RANDOM */
static float g_x_random[QUANTIDADE_AMOSTRAS_RANDOM];
static float g_y_random[QUANTIDADE_AMOSTRAS_RANDOM];
static float g_x_random_centralizado[QUANTIDADE_AMOSTRAS_RANDOM];
static int   g_n_random = 0;


/**
 * Salva os parâmetros finais em arquivo para comparação.
 * 
 * @param indice_dataset Número do dataset
 * @param a Coeficiente angular final
 * @param b Coeficiente linear final
 * @param mse Erro quadrático médio final
 */
static void salvar_parametros_finais(char* context, int i, float a, float b, float mse) {
    FILE *fp = fopen("resultados_finais.txt", "a");  // modo append
    if (fp) {
        fprintf(fp, "(%s) Dataset %d: a=%.10f, b=%.10f, mse=%.10f\n", 
                context, i,  a, b, mse);
        fclose(fp);
    }
}


/* ========================================================================== */
/* FUNÇÕES AUXILIARES                                                         */
/* ========================================================================== */

static int carregar_csv_original(const char *caminho) {
    FILE *fp = fopen(caminho, "r");
    if (!fp) return 0;

    char cab[128];
    fgets(cab, sizeof(cab), fp);

    for (int i = 0; i < QUANTIDADE_AMOSTRAS; i++) {
        if (fscanf(fp, "%f,%f", &g_x_original[i], &g_y_original[i]) != 2) {
            fclose(fp);
            return 0;
        }
    }
    fclose(fp);
    g_n_original = QUANTIDADE_AMOSTRAS;
    return g_n_original;
}

static int carregar_csv_random(const char *caminho) {
    FILE *fp = fopen(caminho, "r");
    if (!fp) return 0;

    char cab[128];
    fgets(cab, sizeof(cab), fp);

    for (int i = 0; i < QUANTIDADE_AMOSTRAS_RANDOM; i++) {
        if (fscanf(fp, "%f,%f", &g_x_random[i], &g_y_random[i]) != 2) {
            fclose(fp);
            return 0;
        }
    }
    fclose(fp);
    g_n_random = QUANTIDADE_AMOSTRAS_RANDOM;
    return g_n_random;
}

static float calcular_media(const float *v, int n) {
    float s = 0;
    for (int i = 0; i < n; i++) s += v[i];
    return s / n;
}

/* MSE no dataset ORIGINAL */
static float calcular_mse_original(float a, float b) {
    float erro = 0;
    for (int i = 0; i < g_n_original; i++) {
        float pred = a * g_x_original[i] + b;
        float e = pred - g_y_original[i];
        erro += e * e;
    }
    return erro / g_n_original;
}

/* MSE no dataset RANDOM */
static float calcular_mse_random(float a, float b) {
    float erro = 0;
    for (int i = 0; i < g_n_random; i++) {
        float pred = a * g_x_random[i] + b;
        float e = pred - g_y_random[i];
        erro += e * e;
    }
    return erro / g_n_random;
}

/* Gradiente descendente (parametrizável para qualquer dataset) */
static void executar_epoca_gradiente(float *a, float *b,
                                     const float *x_cent, const float *y,
                                     int n) {
    float ga = 0, gb = 0;

    for (int i = 0; i < n; i++) {
        float pred = (*a) * x_cent[i] + (*b);
        float e = pred - y[i];
        ga += e * x_cent[i];
        gb += e;
    }

    ga = (2.0 * ga) / n;
    gb = (2.0 * gb) / n;

    *a -= TAXA_APRENDIZADO_INCLINACAO * ga;
    *b -= TAXA_APRENDIZADO_INTERCEPTO * gb;
}

static void treinar_modelo(const float *x, const float *y,
                           float *x_cent, int n,
                           float *a_out, float *b_out) {

    float media_x = calcular_media(x, n);
    float media_y = calcular_media(y, n);

    for (int i = 0; i < n; i++)
        x_cent[i] = x[i] - media_x;

    float a = 0.0;
    float b = media_y;

    for (int ep = 0; ep < EPOCAS_TREINAMENTO; ep++)
        executar_epoca_gradiente(&a, &b, x_cent, y, n);

    *a_out = a;
    *b_out = b - a * media_x;
}

/* ========================================================================== */
/* MAIN                                                                       */
/* ========================================================================== */

int main() {
    for(int i = 0;  i < QUANTIDADE_ARQUIVOS; i++) {
        char caminho_original[256];
        char caminho_shuffle[256];
        snprintf(caminho_original, sizeof(caminho_original), "original/dataset_original%d.csv", i);
        snprintf(caminho_shuffle, sizeof(caminho_shuffle), "shuffle/dataset_randomico%d.csv", i);

        float a_orig, b_orig;
        float a_rand, b_rand;
        
        printf("=====================================\n");
        printf(" TREINANDO NO DATASET ORIGINAL\n");
        printf("=====================================\n");
        
        carregar_csv_original(caminho_original);
        treinar_modelo(g_x_original, g_y_original, g_x_orig_centralizado,
                    g_n_original, &a_orig, &b_orig);

        float mse_orig = calcular_mse_original(a_orig, b_orig);
        
        // Salvar resultados
        salvar_parametros_finais("original", i, a_orig, b_orig, mse_orig);

        printf("Modelo Original: a=%.6f b=%.6f\n", a_orig, b_orig);
        printf("MSE Original = %.6f\n\n", mse_orig);


        printf("=====================================\n");
        printf(" TREINANDO NO DATASET RANDOMICO\n");
        printf("=====================================\n");
        
        carregar_csv_random(caminho_shuffle);
        treinar_modelo(g_x_random, g_y_random, g_x_random_centralizado,
            g_n_random, &a_rand, &b_rand);
            
        float mse_rand = calcular_mse_random(a_rand, b_rand);
        
        // Salvar resultados
        // salvar_parametros_finais("randomico", i, a_rand, b_rand, mse_rand);

        printf("Modelo Randomico: a=%.6f b=%.6f\n", a_rand, b_rand);
        printf("MSE Randomico = %.6f\n\n", mse_rand);
        
        
        printf("=====================================\n");
        printf(" AVALIANDO MODELO RANDOMICO NO ORIGINAL\n");
        printf("=====================================\n");
        
        /* Aqui está o teste final que você pediu */
        float mse_rand_no_original = calcular_mse_original(a_rand, b_rand);
        
        // Salvar resultados
        salvar_parametros_finais("randomico no original", i, a_rand, b_rand, mse_rand_no_original);


        printf("MSE(modelo_random  →  dados_originais) = %.6f\n",
            mse_rand_no_original);
            
        printf("\n======== FIM ========\n");
    }
    return 0;
}
