import numpy as np
from sklearn.linear_model import SGDRegressor
from sklearn.preprocessing import StandardScaler


# === Dataset ===
for i in range(4):
    dados = np.loadtxt(f"data/dataset{i}.csv", delimiter=",", skiprows=1)
    x = dados[:, 0].reshape(-1, 1)
    x = [x.round(decimals=6) for x in x]  # Evitar problemas de precisão
    y = dados[:, 1]
    # print(f"dados {x[1]}")

    # Centralizar e normalizar (melhor para o SGDRegressor)
    scaler = StandardScaler(with_mean=True, with_std=True)
    x_c = scaler.fit_transform(x)

    # === Modelo ===
    modelo = SGDRegressor(
        alpha=0.0,
        penalty=None,
        learning_rate='constant',
        eta0=1e-5,         # pode tentar 1e-6 se ainda oscilar
        shuffle=False,
        max_iter=30000,
        tol=None,
        fit_intercept=True
    )

    modelo.fit(x_c, y)

    # Converter de volta para escala original
    a_c = modelo.coef_[0]
    b_c = modelo.intercept_[0]
    a = a_c / scaler.scale_[0]
    b = b_c - a * scaler.mean_[0]

    print(f"\n({i}) - Descida de Gradiente (Python) - Dataset{i}:")
    print(f"x mean = {scaler.mean_[0]}, y mean = {b_c}")
    print(f"a={a}, b={b}")