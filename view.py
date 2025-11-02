import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

#==== 1. Dados simulados ====
np.random.seed(0)
X = np.linspace(0, 10, 50)
ruido = np.random.uniform(-0.5, 0.5, size=X.shape)
Y = 2 * X + 1 + ruido   # função real: y = 2x + 1 + ruído

# dados = np.loadtxt(f"data/dataset0.csv", delimiter=",", skiprows=1)
# X = dados[:, 0].reshape(-1, 1)
# X = [X.round(decimals=6) for X in X]  # Evitar problemas de precisão
# Y = dados[:, 1]
# X = np.array(X).flatten()
# Y = np.array(Y)

# ==== 2. Funções auxiliares ====
def prever(a, b, x):
    return a * x + b

def mse(a, b, x, y):
    return np.mean((prever(a, b, x) - y)**2)

def gradiente(a, b, x, y):
    y_pred = prever(a, b, x)
    erro = y_pred - y
    da = (2/len(x)) * np.sum(erro * x)
    db = (2/len(x)) * np.sum(erro)
    return da, db

# ==== 3. Hiperparâmetros ====
alpha = 0.01     # taxa de aprendizado
epocas = 10

# ==== 4. Inicialização ====
a, b = np.random.randn(2)
historico = [(a, b, mse(a, b, X, Y))]

# ==== 5. Treinamento ====
for _ in range(epocas):
    da, db = gradiente(a, b, X, Y)
    a -= alpha * da
    b -= alpha * db
    historico.append((a, b, mse(a, b, X, Y)))

# ==== 6. Preparar visualização ====
fig, (ax1, ax2, ax3) = plt.subplots(1, 3, figsize=(12, 5))
a_hist, b_hist, J_hist = zip(*historico)

# Configuração do primeiro gráfico (reta ajustando aos dados)
ax1.scatter(X, Y, color='blue', label='Dados reais')
linha, = ax1.plot([], [], 'r-', lw=2, label='Reta estimada')
ax1.set_xlim(min(X)-1, max(X)+1)
ax1.set_ylim(min(Y)-1, max(Y)+1)
ax1.set_title('Ajuste da reta via Descida de Gradiente')
ax1.legend()
ax1.grid(True)

# Configuração do segundo gráfico (erro ao longo das iterações)
ax2.set_xlim(0, epocas)
ax2.set_ylim(0, max(J_hist))
ax2.set_title('Evolução do Erro (MSE)')
linha_erro, = ax2.plot([], [], 'g-', lw=2)
ax2.set_xlabel('Época')
ax2.set_ylabel('Erro (MSE)')
ax2.grid(True)


# Campo de gradiente (opcional)
A_vals = np.linspace(0, 4,20)
B_vals = np.linspace(0, 3,20)
AA, BB = np.meshgrid(A_vals, B_vals)

# AA
# [0 1 2]
# [0 1 2]


# BB
# [0 0 0]
# [1 1 1]


DA, DB = np.zeros_like(AA), np.zeros_like(BB)
for i in range(AA.shape[0]): # rows
    for j in range(AA.shape[1]): # cols
        da, db = gradiente(AA[i,j], BB[i,j], X, Y)
        DA[i,j], DB[i,j] = -da, -db  # direção de descida

# ax3.figure(figsize=(6,5))
ax3.quiver(AA, BB, DA, DB, color='gray', alpha=0.7)
linha_var, = ax3.plot([], [], 'r.-', label='Caminho (a,b)')
ax3.set_xlabel('a')
ax3.set_ylabel('b')
ax3.set_title('Campo de Gradiente e Caminho da Descida')
ax3.legend()
ax3.grid(True)
# ax3.show()a.f

# ==== 7. Função de animação ====
def update(frame):
    a, b = a_hist[frame], b_hist[frame]
    y_pred = prever(a, b, X)
    linha.set_data(X, y_pred)
    linha_erro.set_data(range(frame+1), J_hist[:frame+1])
    linha_var.set_data(a_hist[:frame+1], b_hist[:frame+1])
    return linha, linha_erro, linha_var

anim = FuncAnimation(fig, update, frames=len(a_hist), interval=500, blit=True)
plt.tight_layout()
plt.show()

