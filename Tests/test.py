import numpy as np
from sklearn.linear_model import SGDRegressor
from sklearn.preprocessing import StandardScaler

# === Dataset ===
dados = np.loadtxt("dataset.csv", delimiter=",", skiprows=1)
x = dados[:, 0].reshape(-1, 1)
y = dados[:, 1]

# Centralizar e normalizar (melhor para o SGDRegressor)
scaler = StandardScaler(with_mean=True, with_std=True)
x_c = scaler.fit_transform(x)

# === Modelo ===
modelo = SGDRegressor(
    max_iter=30000,
    learning_rate='constant',
    eta0=1e-5,         # pode tentar 1e-6 se ainda oscilar
    penalty=None,
    shuffle=False,
    tol=None,
    fit_intercept=True
)

modelo.fit(x_c, y)

# Converter de volta para escala original
a_c = modelo.coef_[0]
b_c = modelo.intercept_[0]
a = a_c / scaler.scale_[0]
b = b_c - a * scaler.mean_[0]

print(f"a={a:.6f}, b={b:.6f}")
