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
    - x_centralizado = x - media_x    : ajuda a “desacoplar” a de b
    - b_original = b_centralizado - a_centralizado * media_x

  Observação: Usamos “centralização” de x (subtrair a média) para que
  o intercepto aprenda corretamente com passos de atualização simples.
  Isso não é “otimização micro”; é apenas tornar o problema bem condicionado.
*/

#include <stdio.h>

/* --------------------- Parâmetros do experimento --------------------- */

#define QUANTIDADE_ARQUIVOS 1
#define QUANTIDADE_AMOSTRAS 10
#define EPOCAS_TREINAMENTO  20
#define ARQUIVO_HISTORICO "historico_treinamento.csv"

/* Passos de atualização separados:
   - a (inclinação) precisa de passo pequeno (x pode ser grande)
   - b (intercepto) pode usar passo maior
*/
#define TAXA_APRENDIZADO_INCLINACAO  1e-1f
#define TAXA_APRENDIZADO_INTERCEPTO  1e-1f

#define INTERVALO_DE_LOG 5000

/* --------------------- Armazenamento estático (~8KB) ------------------ */

// vetor de entradas (x) — valores do eixo horizontal.
// exemplo: 1.0, 2.0, 3.0, ..., 1000.0
static double g_x[QUANTIDADE_AMOSTRAS];

// vetor de saídas (y) — valores medidos do dataset
// exemplo: y = 2x + 1 + ruído
static double g_y[QUANTIDADE_AMOSTRAS];

// vetor auxiliar: x centralizado (x - média(x))
// equilibra o treinamento e evitar oscilações
static double g_x_centralizado[QUANTIDADE_AMOSTRAS];

// quantidade real de amostras
static int   g_n = 0;

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
        g_x[i] = (double)xd;
        g_y[i] = (double)yd;
    }

    fclose(fp);
    g_n = QUANTIDADE_AMOSTRAS;
    return g_n;
}

/* --------------------- Utilidades numéricas simples ------------------- */

static double media(const double *v, int n) {
    double soma = 0.0;
    for (int i = 0; i < n; i++) soma += (double)v[i];
    return (double)(soma / (double)n);
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

static void uma_epoca_descida_de_gradiente(double *a_centralizado,
                                           double *b_centralizado)
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

    *a_centralizado -= (TAXA_APRENDIZADO_INCLINACAO * (double)gradiente_a);
    *b_centralizado -= (TAXA_APRENDIZADO_INTERCEPTO * (double)gradiente_b);
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

static void treinar_descida_de_gradiente(double *a_original,
                                         double *b_original,
                                         const char *arquivo_historico)
{
    FILE *fp_hist = fopen(arquivo_historico, "w");
    if (fp_hist) {
        fprintf(fp_hist, "epoca,a,b,mse\n");
    }

    double media_x = media(g_x, g_n);
    double media_y = media(g_y, g_n);
    
    for (int i = 0; i < g_n; i++) {
        g_x_centralizado[i] = g_x[i] - media_x;
    }

    double a_c = 0.0f;          /* inclinação no espaço centrado */
    double b_c = media_y;       /* intercepto começa na média de y */

    for (int epoca = 0; epoca < EPOCAS_TREINAMENTO; epoca++) {
        uma_epoca_descida_de_gradiente(&a_c, &b_c);

        // Calcular MSE
        double mse = 0.0;
        double a_temp = a_c;
        double b_temp = b_c - a_c * media_x;
        
        for (int i = 0; i < g_n; i++) {
            double y_pred = a_temp * g_x[i] + b_temp;
            double erro = y_pred - g_y[i];
            mse += erro * erro;
        }
        mse /= g_n;

        // Salvar no CSV
        if (fp_hist) {
            fprintf(fp_hist, "%d,%.10f,%.10f,%.10f\n", 
                    epoca, a_temp, b_temp, mse);
        }
    }

    if (fp_hist) fclose(fp_hist);
    
    *a_original = a_c;
    *b_original = b_c - a_c * media_x;
}

/* --------------------- Programa principal ----------------------------- */

int main(void) {
    for(volatile int i = 0; i < QUANTIDADE_ARQUIVOS; i++) {
        char caminho[256];
        sprintf(caminho, "data/dataset%d.csv", i);
        if (!carregar_csv(caminho)) return 1;
        printf("\n(%d) - Descida de Gradiente (C) - Dataset%d:\n", i, i);
        
        // parâmetros ajustados pelo algoritmo
        // a = inclinação da reta, b = intercepto
        double a_treinado = 0.0f, b_treinado = 0.0f;
        treinar_descida_de_gradiente(&a_treinado, &b_treinado, ARQUIVO_HISTORICO);
        
        printf("a = %f  b = %f\n", a_treinado, b_treinado);
    }
    
    return 0;
}