/*
  gd_csv_nao_otimizado.c
  ------------------------------------------------------------------
  Regressão Linear 1D (y ≈ a*x + b) com Descida de Gradiente (GD),
  lendo dados de um arquivo CSV no formato:
    x,y
    1.000000,3.190011
    2.000000,5.093333
    ...

  Objetivo: versão didática e NAO OTIMIZADA (para "antes/depois").
  - Sem recursão
  - Sem alocação dinâmica (arrays estáticos)
  - ~8 KB de dados: x[1000] + y[1000] em float
  - Laços aninhados claros: épocas x amostras
  - Fórmula do GD explícita (θ ← θ − α ∂J/∂θ)

  Compile SEM otimização:
    gcc -O0 -std=c99 gd_csv_nao_otimizado.c -o gd_csv_nao_opt
*/

#include <stdio.h>

#define DATASET_PATH "dataset.csv"  /* altere o nome/ caminho se precisar */
#define CAPACITY 1000               /* capacidade máxima de linhas lidas  */

#define EPOCHS 50000                /* muitas épocas, propositalmente     */
#define LEARNING_RATE 1e-7f         /* eta pequeno (x pode ir até ~1000)  */
#define LOG_INTERVAL 5000           /* imprime a cada 5k épocas           */

/* ------------------------------------------------------------------
   Armazenamento estático (~8KB): 2 * 1000 * 4 bytes = 8000 bytes
   ------------------------------------------------------------------ */
static float g_x[CAPACITY];
static float g_y[CAPACITY];

/* número real de amostras lidas do CSV (<= CAPACITY) */
static int g_n = 0;

/* ------------------------------------------------------------------
   Leitura de CSV simples e NÃO otimizada:
   - Usa fgets + sscanf
   - Ignora a primeira linha (cabeçalho "x,y")
   - Para em EOF ou quando CAPACITY é atingida
   ------------------------------------------------------------------ */
static int load_csv(const char *path)
{
    FILE *fp = fopen(path, "r");
    if (!fp) {
        printf("ERRO: nao consegui abrir '%s'\n", path);
        return 0;
    }

    char line[256];
    int line_no = 0;
    int n = 0;

    while (fgets(line, sizeof(line), fp) != NULL) {
        if (line_no == 0) {  /* pula o cabecalho "x,y" */
            line_no++;
            continue;
        }

        float xv = 0.0f, yv = 0.0f;
        /* leitura sem otimização; confia no formato "x,y" */
        if (sscanf(line, "%f,%f", &xv, &yv) == 2) {
            if (n < CAPACITY) {
                g_x[n] = xv;
                g_y[n] = yv;
                n++;
            } else {
                /* Paramos ao atingir a capacidade; sem realloc. */
                break;
            }
        }
        line_no++;
    }

    fclose(fp);
    g_n = n;
    return n;
}

/* --------------------------
   Cálculo do custo (MSE)
   -------------------------- */
static float mse(float a, float b)
{
    double soma = 0.0;           /* double pra acumular com menos erro */
    int i = 0;
    while (i < g_n) {
        double y_hat = (double)a * (double)g_x[i] + (double)b;
        double e = y_hat - (double)g_y[i];
        soma = soma + e * e;
        i = i + 1;
    }
    return (float)(soma / (double)g_n);
}

/* -----------------------------------------------
   Solução fechada (referência) — mínimos quadrados
   ----------------------------------------------- */
static void normal_equation(float *a_ref, float *b_ref)
{
    double sx = 0.0, sy = 0.0, sxx = 0.0, sxy = 0.0;
    int i = 0;
    while (i < g_n) {
        double x = (double)g_x[i];
        double y = (double)g_y[i];
        sx  = sx  + x;
        sy  = sy  + y;
        sxx = sxx + x * x;
        sxy = sxy + x * y;
        i   = i + 1;
    }
    double denom = (double)g_n * sxx - sx * sx;
    double a = ((double)g_n * sxy - sx * sy) / denom;
    double b = (sy - a * sx) / (double)g_n;
    *a_ref = (float)a;
    *b_ref = (float)b;
}

/* -----------------------------------------------
   Uma época de GD "batch" (gradiente médio)
   Derivadas (para MSE):
     dJ/da = (2/N) * Σ (a*x_i + b - y_i) * x_i
     dJ/db = (2/N) * Σ (a*x_i + b - y_i)
   Atualização (FÓRMULA DO GD):
     a = a - η * dJ/da
     b = b - η * dJ/db
   ----------------------------------------------- */
static void gd_epoch(float *a, float *b)
{
    double grad_a = 0.0;
    double grad_b = 0.0;

    int i = 0;
    while (i < g_n) {
        double y_hat = (double)(*a) * (double)g_x[i] + (double)(*b);
        double e     = y_hat - (double)g_y[i];
        grad_a = grad_a + e * (double)g_x[i];
        grad_b = grad_b + e;
        i = i + 1;
    }

    /* (2/N) calculado toda vez de propósito (não otimizado) */
    double invN = 1.0 / (double)g_n;
    grad_a = 2.0 * invN * grad_a;
    grad_b = 2.0 * invN * grad_b;

    /* AQUI está a fórmula θ ← θ − α ∂J/∂θ */
    *a = *a - (LEARNING_RATE * (float)grad_a);
    *b = *b - (LEARNING_RATE * (float)grad_b);
}

/* --------------------------
   Treinamento por E épocas
   -------------------------- */
static void train(float *a_out, float *b_out)
{
    float a = 0.0f;  /* chutes simples */
    float b = 0.0f;

    int epoch = 0;
    while (epoch < EPOCHS) {
        gd_epoch(&a, &b);

        if ((epoch % LOG_INTERVAL) == 0 || epoch == (EPOCHS - 1)) {
            float j = mse(a, b);
            printf("epoch %5d | a = %.7f  b = %.7f  | MSE = %.7f\n",
                   epoch, a, b, j);
        }
        epoch = epoch + 1;
    }

    *a_out = a;
    *b_out = b;
}

/* ---------------
   Programa principal
   --------------- */
int main(void)
{
    int n = load_csv(DATASET_PATH);
    if (n <= 0) {
        printf("Nenhuma linha valida lida de '%s'.\n", DATASET_PATH);
        return 1;
    }
    printf("Carregadas %d amostras de '%s'.\n", n, DATASET_PATH);

    /* Treina (GD) */
    float a_gd = 0.0f, b_gd = 0.0f;
    train(&a_gd, &b_gd);

    /* Referência por mínimos quadrados (validação) */
    float a_ref = 0.0f, b_ref = 0.0f;
    normal_equation(&a_ref, &b_ref);

    /* Relatório final */
    float j_final = mse(a_gd, b_gd);
    float ea = (a_ref != 0.0f) ? (a_gd - a_ref) / a_ref : 0.0f;
    float eb = (b_ref != 0.0f) ? (b_gd - b_ref) / b_ref : 0.0f;

    /* valores verdadeiros esperados do gerador: y = 2x + 1 (aprox.) */
    const float A_TRUE = 2.0f;
    const float B_TRUE = 1.0f;

    printf("\n==== RESULTADOS (NAO OTIMIZADO) ====\n");
    printf("GD:   a = %.7f, b = %.7f, MSE = %.7f\n", a_gd, b_gd, j_final);
    printf("REF:  a* = %.7f, b* = %.7f  (minimos quadrados)\n", a_ref, b_ref);
    printf("TRUE: aT = %.7f, bT = %.7f  (do gerador: y=2x+1)\n", A_TRUE, B_TRUE);
    if (a_ref != 0.0f) ea = (a_gd - a_ref) / a_ref;
    if (b_ref != 0.0f) eb = (b_gd - b_ref) / b_ref;
    if (ea < 0) ea = -ea; if (eb < 0) eb = -eb;
    printf("Erro relativo (GD vs REF): a = %.3e | b = %.3e\n", ea, eb);

    return 0;
}
