/*
  gradient_descent_linear_regression.c
  ---------------------------------------------------------------
  Regressão linear (y ≈ a*x + b) treinada com Descida de Gradiente,
  validada com a solução fechada (mínimos quadrados).
  - ~8 KB de dados: 2 × 1000 floats
  - Leitura de CSV no formato:
        x,y
        1.000000,3.190011
        2.000000,5.093333
        ...

  Conceitos e símbolos:
    - a  : inclinação (slope) da reta
    - b  : intercepto da reta (valor de y quando x=0)
    - taxa_de_aprendizado_inclinacao  : passo de atualização para a
    - taxa_de_aprendizado_intercepto  : passo de atualização para b
    - erro_medio_quadratico (MSE)     : média do quadrado do erro
    - x_centralizado = x - media_x    : ajuda a “desacoplar” a de b
    - b_original = b_centralizado - a_centralizado * media_x

  Observação: Usamos “centralização” de x (subtrair a média) para que
  o intercepto aprenda corretamente com passos de atualização simples.
  Isso não é “otimização micro”; é apenas tornar o problema bem condicionado.
*/

#include <stdio.h>

/* --------------------- Parâmetros do experimento --------------------- */

#define QUANTIDADE_AMOSTRAS 1000
#define EPOCAS_TREINAMENTO  30000

/* Passos de atualização separados:
   - a (inclinação) precisa de passo pequeno (x pode ser grande)
   - b (intercepto) pode usar passo maior
*/
#define TAXA_APRENDIZADO_INCLINACAO  1e-5f
#define TAXA_APRENDIZADO_INTERCEPTO  1e-2f

#define INTERVALO_DE_LOG 5000

/* --------------------- Armazenamento estático (~8KB) ------------------ */

// vetor de entradas (x) — valores do eixo horizontal.
// exemplo: 1.0, 2.0, 3.0, ..., 1000.0
static float g_x[QUANTIDADE_AMOSTRAS];            /* valores de entrada (x)        */

// vetor de saídas (y) — valores medidos do dataset
// exemplo: y = 2x + 1 + ruído
static float g_y[QUANTIDADE_AMOSTRAS];            /* valores alvo (y)              */

// vetor auxiliar: x centralizado (x - média(x))
// equilibra o treinamento e evitar oscilações
static float g_x_centralizado[QUANTIDADE_AMOSTRAS];/* x - media_x                   */

// quantidade real de amostras
static int   g_n = 0;                              /* número de amostras lidas      */

/* --------------------- Leitura de CSV (simples) ----------------------- */

static int carregar_csv(const char *caminho_csv) {
    FILE *fp = fopen(caminho_csv, "r");
    if (!fp) {
        fprintf(stderr, "Erro: não foi possível abrir '%s'\n", caminho_csv);
        return 0;
    }

    char cabecalho[128];
    if (!fgets(cabecalho, sizeof(cabecalho), fp)) {
        fprintf(stderr, "Erro: arquivo vazio ou corrompido.\n");
        fclose(fp);
        return 0;
    }

    for (int i = 0; i < QUANTIDADE_AMOSTRAS; i++) {
        double xd, yd;
        if (fscanf(fp, "%lf,%lf", &xd, &yd) != 2) {
            fprintf(stderr, "Erro ao ler a linha %d do CSV.\n", i + 2);
            fclose(fp);
            return 0;
        }
        g_x[i] = (float)xd;
        g_y[i] = (float)yd;
    }

    fclose(fp);
    g_n = QUANTIDADE_AMOSTRAS;
    return g_n;
}

/* --------------------- Utilidades numéricas simples ------------------- */

static float media(const float *v, int n) {
    double soma = 0.0;
    for (int i = 0; i < n; i++) soma += (double)v[i];
    return (float)(soma / (double)n);
}

static float erro_medio_quadratico(float a, float b, const float *x, const float *y, int n) {
    double soma = 0.0;
    for (int i = 0; i < n; i++) {
        double y_pred = (double)a * (double)x[i] + (double)b; // ← previsão: a*x + b
        double e = y_pred - (double)y[i];                     // ← erro da amostra
        soma += e * e;                                        // ← erro ao quadrado
    }
    return (float)(soma / (double)n);                         // ← MSE: média dos quadrados
}

/* --------------------- Referência: mínimos quadrados ------------------ */
/* Resolve a* e b* analiticamente (para validação) sobre (g_x, g_y).     */

static void minimos_quadrados(float *a_ref, float *b_ref) {
    double sx = 0.0, sy = 0.0, sxx = 0.0, sxy = 0.0;
    for (int i = 0; i < g_n; i++) {
        double x = (double)g_x[i];
        double y = (double)g_y[i];
        sx  += x;
        sy  += y;
        sxx += x * x;
        sxy += x * y;
    }
    double denom = (double)g_n * sxx - sx * sx;
    double a = ((double)g_n * sxy - sx * sy) / denom;
    double b = (sy - a * sx) / (double)g_n;
    *a_ref = (float)a;
    *b_ref = (float)b;
}

/* --------------------- Uma época de descida de gradiente -------------- */
/*
  Modelo treinado no espaço “centrado”:
    y ≈ a_centralizado * (x - media_x) + b_centralizado

  Gradientes (derivados do erro médio quadrático):
    dJ/da_centralizado = (2/N) * Σ (erro * (x - media_x))
    dJ/db_centralizado = (2/N) * Σ erro

  Atualização (regra do “desce a ladeira”):
    a_centralizado ← a_centralizado - taxa_inclinacao  * dJ/da_centralizado
    b_centralizado ← b_centralizado - taxa_intercepto  * dJ/db_centralizado
*/

static void uma_epoca_descida_de_gradiente(float *a_centralizado,
                                           float *b_centralizado)
{
    double gradiente_a = 0.0;
    double gradiente_b = 0.0;

    for (int i = 0; i < g_n; i++)
    {
        double u = (double)g_x_centralizado[i];         // u = x - média(x)
        double y_pred = (double)(*a_centralizado) * u + (double)(*b_centralizado);
        double erro = y_pred - (double)g_y[i];

        gradiente_a += erro * u;   // dJ/da_c ∝ soma(erro * u)
        gradiente_b += erro;       // dJ/db_c ∝ soma(erro)
    }
    // se gradiente_a > 0, diminuímos a_c; se < 0, aumentamos a_c, o mesmo vale para b_c
    gradiente_a = 2.0 * gradiente_a / (double)g_n;
    gradiente_b = 2.0 * gradiente_b / (double)g_n;

    *a_centralizado -= (TAXA_APRENDIZADO_INCLINACAO * (float)gradiente_a);
    *b_centralizado -= (TAXA_APRENDIZADO_INTERCEPTO * (float)gradiente_b);
}

/* --------------------- Treinamento completo --------------------------- */
/*
  Passos:
    1) centraliza x: g_x_centralizado[i] = g_x[i] - media_x
    2) inicia a e b (b começa em media_y para acelerar)
    3) executa EPOCAS_TREINAMENTO épocas
    4) converte de volta para a escala original:
         a_original = a_centralizado
         b_original = b_centralizado - a_centralizado * media_x
*/

static void treinar_descida_de_gradiente(float *a_original,
                                         float *b_original)
{
    float media_x = media(g_x, g_n);
    float media_y = media(g_y, g_n);

    for (int i = 0; i < g_n; i++) {
        g_x_centralizado[i] = g_x[i] - media_x;
    }

    float a_c = 0.0f;          /* inclinação no espaço centrado */
    float b_c = media_y;       /* intercepto começa na média de y */

    for (int epoca = 0; epoca < EPOCAS_TREINAMENTO; epoca++) {
        uma_epoca_descida_de_gradiente(&a_c, &b_c);

        if ((epoca % INTERVALO_DE_LOG) == 0 || epoca == (EPOCAS_TREINAMENTO - 1)) {
            /* Métrica no espaço centrado (opcional) */
            float mse_centrado = erro_medio_quadratico(a_c, b_c, g_x_centralizado, g_y, g_n);

            /* Converte para a escala original para inspecionar a e b “finais” */
            float a_temp = a_c;
            float b_temp = b_c - a_c * media_x;
            float mse_original = erro_medio_quadratico(a_temp, b_temp, g_x, g_y, g_n);

            printf("época %5d | (centrado) a_c=%.6f b_c=%.6f | MSEc=%.6f | "
                   "(original) a=%.6f b=%.6f | MSE=%.6f\n",
                   epoca, a_c, b_c, mse_centrado, a_temp, b_temp, mse_original);
        }
    }

    *a_original = a_c;
    *b_original = b_c - a_c * media_x;
}

/* --------------------- Programa principal ----------------------------- */

int main(void) {
    const char *caminho_csv = "../dataset.csv";

    if (!carregar_csv(caminho_csv)) return 1;
    printf("Arquivo '%s' lido com sucesso (%d amostras)\n", caminho_csv, g_n);

    // parâmetros ajustados pelo algoritmo
    // a = inclinação da reta, b = intercepto
    float a_treinado = 0.0f, b_treinado = 0.0f;
    treinar_descida_de_gradiente(&a_treinado, &b_treinado);

    float a_referencia = 0.0f, b_referencia = 0.0f;
    minimos_quadrados(&a_referencia, &b_referencia);

    float mse_final = erro_medio_quadratico(a_treinado, b_treinado, g_x, g_y, g_n);

    printf("\n=== RESULTADOS FINAIS ===\n");
    printf("Descida de Gradiente:  a = %.6f  b = %.6f  | MSE = %.6f\n", a_treinado, b_treinado, mse_final);
    printf("Mínimos Quadrados:     a* = %.6f b* = %.6f\n", a_referencia, b_referencia);
    printf("Modelo gerador (ideal): a = 2.000000  b = 1.000000  (y = 2x + 1)\n");

    return 0;
}
