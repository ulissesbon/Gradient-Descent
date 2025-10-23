#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define N 1000  // dataset fixo (sem alocação dinâmica)

typedef struct {
    double peso;       // w
    double intercepto; // b
} Modelo;

double random_double(double min, double max) {
    return min + ((double)rand() / RAND_MAX) * (max - min);
}

void gerar_dados(double x[], double y[], int n,
                 double peso_real, double intercepto_real, double ruido_max) {
    printf("Gerando %d pontos de dados...\n", n);
    printf("Função real: y = %.2fx + %.2f (com ruído até ±%.2f)\n\n",
           peso_real, intercepto_real, ruido_max);
    for (int i = 0; i < n; i++) {
        x[i] = (double)(i + 1); // 1..N
        double ruido = random_double(-ruido_max, ruido_max);
        y[i] = peso_real * x[i] + intercepto_real + ruido;
    }
}

double prever(const Modelo *m, double x) { return m->peso * x + m->intercepto; }

double mse(const Modelo *m, const double x[], const double y[], int n) {
    double e = 0.0;
    for (int i = 0; i < n; i++) {
        double d = prever(m, x[i]) - y[i];
        e += d * d;
    }
    return e / n;
}

/* Ajuste linear por solução fechada (OLS) */
void ajustar_ols(Modelo *m, const double x[], const double y[], int n) {
    double sumx = 0.0, sumy = 0.0;
    for (int i = 0; i < n; i++) { sumx += x[i]; sumy += y[i]; }
    double mean_x = sumx / n;
    double mean_y = sumy / n;

    double Sxx = 0.0, Sxy = 0.0;
    for (int i = 0; i < n; i++) {
        double xc = x[i] - mean_x;
        double yc = y[i] - mean_y;
        Sxx += xc * xc;
        Sxy += xc * yc;
    }

    m->peso = Sxy / Sxx;                 // w
    m->intercepto = mean_y - m->peso * mean_x; // b
}

int main(void) {
    srand((unsigned)time(NULL));

    const double peso_real = 2.0;
    const double intercepto_real = 1.0;
    const double ruido_max = 0.5;

    // Arrays estáticos (sem alocação dinâmica)
    double x[N], y[N];

    gerar_dados(x, y, N, peso_real, intercepto_real, ruido_max);

    Modelo modelo;
    ajustar_ols(&modelo, x, y, N); // sem gradiente, estável e exato no MSE

    printf("=== AJUSTE OLS (sem malloc, sem gradiente) ===\n\n");
    printf("Peso: %.6f (esperado: %.2f)\n", modelo.peso, peso_real);
    printf("Intercepto: %.6f (esperado: %.2f)\n", modelo.intercepto, intercepto_real);
    printf("Erro (MSE): %.6f\n\n", mse(&modelo, x, y, N));

    printf("Testando predições:\n");
    double teste[] = {100.0, 500.0, 1000.0};
    for (int i = 0; i < 3; i++) {
        double p = prever(&modelo, teste[i]);
        double r = peso_real * teste[i] + intercepto_real;
        printf("x = %.1f -> y predito = %.2f (real seria: %.2f)\n", teste[i], p, r);
    }

    return 0;
}