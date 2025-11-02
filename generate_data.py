import numpy as np

qtd_arquivos = 1
qtd_linhas = 10
a = [2, 60, 100, 300]
b = [1, 70, 1000, 0]
r = [x * 0.5 for x in b]  # Ruído proporcional a 'b'

X_data = np.arange(1, qtd_linhas + 1, dtype=np.float64)

# Gerar datasets
for j in range(qtd_arquivos):
    Y_data = np.array([X_data[i] * a[j] + b[j] + np.random.normal(0, r[j]) for i in range(len(X_data))], dtype=np.float64)

    # Empilha X e Y como colunas
    dados = np.column_stack((X_data, Y_data))

    # Salva no formato CSV
    np.savetxt(f"data/dataset{j}.csv", dados, delimiter=",", header="x,y", comments='', fmt='%.1f')

    print(f"Arquivo 'dataset{j}.csv' com {len(dados)} linhas de dados gerado com sucesso.")
