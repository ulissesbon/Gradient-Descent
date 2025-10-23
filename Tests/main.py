import numpy as np
from sklearn.linear_model import LinearRegression
from sklearn.metrics import mean_squared_error

print("Carregando dados de 'dataset.csv'...")
try:
    dados = np.loadtxt("dataset.csv", delimiter=",", skiprows=1)
    
    X_dados = dados[:, 0]
    Y_dados = dados[:, 1]
    
    print(f"Dados carregados com sucesso ({len(X_dados)} pontos).\n")

except FileNotFoundError:
    print("Erro: Arquivo 'dataset.csv' não encontrado.")
    print("Por favor, execute o script 'gerar_csv.py' primeiro.")
    exit()
except Exception as e:
    print(f"Erro ao ler o arquivo: {e}")
    exit()

peso_real = 2.0
intercepto_real = 1.0

X_reshaped = X_dados.reshape(-1, 1)
Y = Y_dados

modelo = LinearRegression()
modelo.fit(X_reshaped, Y)

peso_calculado = modelo.coef_[0]
intercepto_calculado = modelo.intercept_

y_predito = modelo.predict(X_reshaped)
erro_mse = mean_squared_error(Y, y_predito)


print("\n\n=== AJUSTE OLS (com Scikit-learn) ===\n")
print(f"Peso:       {peso_calculado:.6f} (esperado: {peso_real:.2f})")
print(f"Intercepto: {intercepto_calculado:.6f} (esperado: {intercepto_real:.2f})")
print(f"Erro (MSE): {erro_mse:.6f}\n")


print("Testando predições:")
teste_valores = np.array([100.0, 500.0, 1000.0])
teste_valores_reshaped = teste_valores.reshape(-1, 1)

predicoes = modelo.predict(teste_valores_reshaped)

for i in range(len(teste_valores)):
    x_val = teste_valores[i]
    p = predicoes[i]
    r = peso_real * x_val + intercepto_real
    print(f"x = {x_val:.1f} -> y predito = {p:.2f} (real seria: {r:.2f})")