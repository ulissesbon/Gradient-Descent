#include <stdio.h>
#include <math.h>
#include <stdlib.h> 

#define N 1000

typedef struct {
    double peso;       // w
    double intercepto; // b
} Modelo;

double prever(const Modelo *m, double x) { return m->peso * x + m->intercepto; }

double mse(const Modelo *m, const double x[], const double y[], int n) {
    double e = 0.0;
    for (int i = 0; i < n; i++) {
        double d = prever(m, x[i]) - y[i];
        e += d * d;
    }
    return e / n;
}

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

    m->peso = Sxy / Sxx; // w
    m->intercepto = mean_y - m->peso * mean_x; // b
}

int main(void) {

    double X[N];
    double Y[N];
    
    FILE *fp = fopen("dataset.csv", "r");
    if (fp == NULL) {
        fprintf(stderr, "Erro: Não foi possível abrir o arquivo 'dataset.csv'\n");
        return 1; 
    }

    char buffer[100];
    if (fgets(buffer, sizeof(buffer), fp) == NULL) {
        fprintf(stderr, "Erro: Arquivo 'dataset.csv' está vazio ou corrompido.\n");
        fclose(fp);
        return 1;
    }

    for (int i = 0; i < N; i++) {
        if (fscanf(fp, "%lf,%lf", &X[i], &Y[i]) != 2) {
            fprintf(stderr, "Erro ao ler a linha %d do arquivo de dados.\n", i + 2);
            fclose(fp);
            return 1;
        }
    }
    
    fclose(fp);
    printf("Dados carregados de 'dataset.csv' com sucesso.\n");

    const double peso_real = 2.0;
    const double intercepto_real = 1.0;
    const double ruido_max = 0.5;

    Modelo modelo;
    ajustar_ols(&modelo, X, Y, N);

    printf("\n\n=== AJUSTE OLS ===\n\n");
    printf("Peso: %.6f (esperado: %.2f)\n", modelo.peso, peso_real);
    printf("Intercepto: %.6f (esperado: %.2f)\n", modelo.intercepto, intercepto_real);
    printf("Erro (MSE): %.6f\n\n", mse(&modelo, X, Y, N));

    printf("Testando predições:\n");
    double teste[] = {100.0, 500.0, 1000.0};
    for (int i = 0; i < 3; i++) {
        double p = prever(&modelo, teste[i]);
        double r = peso_real * teste[i] + intercepto_real;
        printf("x = %.1f -> y predito = %.2f (real seria: %.2f)\n", teste[i], p, r);
    }

    return 0;
}