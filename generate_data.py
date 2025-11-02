import numpy as np

a = [200, 600, 100, 300]
b = [100, 700, 1000, 0]
r = 0.5

X_data = np.arange(1, 1001, dtype=np.float64)

# Gerar datasets
for j in range(4):
    Y_data = np.array([X_data[i] * a[j] + b[j] + np.random.normal(0, r) for i in range(len(X_data))], dtype=np.float64)

    # Empilha X e Y como colunas
    dados = np.column_stack((X_data, Y_data))

    # Salva no formato CSV
    np.savetxt(f"data/dataset{j}.csv", dados, delimiter=",", header="x,y", comments='', fmt='%.1f')

    print(f"Arquivo 'dataset{j}.csv' com {len(dados)} linhas de dados gerado com sucesso.")
