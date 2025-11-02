# Regressão Linear com Descida de Gradiente (C puro)

Este projeto implementa, em **C puro**, a regressão linear 1D (`y ≈ a·x + b`) usando **Descida de Gradiente** (treino iterativo) e valida com a **solução fechada de mínimos quadrados** (referência). O foco é **clareza**: nomes descritivos, comentários e zero siglas obscuras.

---

## O que é a regressão linear?

É ajustar uma **reta** aos dados para **prever** `y` a partir de `x`:

\[
y \approx a \cdot x + b
\]

- `a` é a **inclinação** (o quanto `y` muda quando `x` aumenta 1).
- `b` é o **intercepto** (valor de `y` quando `x = 0`).

Uma imagem típica (linha ajustando pontos):

- **Exemplo de reta de melhor ajuste** (imagem educacional):  
  https://stats.libretexts.org/Bookshelves/Introductory_Statistics/Statistics%3A_Open_for_Everyone_%28Peter%29/13%3A_Simple_Linear_Regression/13.06%3A_Visualizing_Linear_Regression :contentReference[oaicite:0]{index=0}

(Para imagens livres de uso que ilustram “linear regression”, você também pode explorar coleções como Pixabay.) :contentReference[oaicite:1]{index=1}

---

## O que é “Descida de Gradiente”?

É um método **iterativo** para **minimizar o erro**. Imagine o erro (por exemplo, o **erro médio quadrático**) como um “**terreno em formato de tigela**”; a cada passo, andamos na direção de maior **descida** desse terreno até chegar no fundo (mínimo).

- Explicação didática com texto/diagramas: **MIT – Intro to ML: Gradient Descent Notes**. :contentReference[oaicite:2]{index=2}
- Conceito introdutório e definição simples: **GeeksforGeeks – What is Gradient Descent**. :contentReference[oaicite:3]{index=3}
- Intuição da “tigela” do MSE (superfície convexa): discussão com ilustração. :contentReference[oaicite:4]{index=4}

> Dica: procure por figuras “gradient descent bowl” (a “tigela” do erro) ou vetores/ícones “gradient descent” para slides. Repositórios de imagens livres: Pixabay/Freepik (verifique licenças). :contentReference[oaicite:5]{index=5}

---

## Como este código funciona (passo a passo)

1. **Lê o arquivo `dataset.csv`** no formato:

"""
x,y
1.000000,3.190011
2.000000,5.093333
...
"""

2. **Centraliza os valores de x**:  
`x_centralizado = x - media(x)`  
Isso “desacopla” a inclinação do intercepto e deixa a aprendizagem estável.
3. **Treina iterativamente (Descida de Gradiente)**:  
Atualiza `a` e `b` a cada época para reduzir o **erro médio quadrático (MSE)**.
4. **Converte de volta para a escala original** (apenas no relatório):  
Se treinamos com `x_centralizado = x - media_x`, então:  
`a_final = a_centralizado`  
`b_final = b_centralizado - a_centralizado * media_x`
5. **Valida** comparando com a **solução fechada** (mínimos quadrados).  
Os valores ficam muito próximos (diferenças vêm do ruído do dataset).

---

## Por que centralizar `x`?

Sem centralização, a atualização de `a` (que multiplica `x`) domina a de `b`, porque `x` pode ter valores altos. Ao centralizar, o algoritmo **consegue aprender `b` rapidamente** (ele fica próximo de `média(y)` quando `a` acerta).

---

## Estrutura do código (arquivos)

- `gradient_descent_linear_regression.c` — tudo em um único arquivo: leitura do CSV, treino por descida de gradiente, equações normais (referência) e relatório final.
- `dataset.csv` — seu conjunto de dados no formato `x,y` (a primeira linha é o cabeçalho).

---

## Como compilar e executar

```bash
# 1) coloque dataset.csv na mesma pasta
# 2) compile (sem otimização, de propósito)
gcc -O0 -std=c99 gradient_descent_linear_regression.c -o regress

# 3) execute
./regress

Saída típica (resumo ao final):
=== RESULTADOS FINAIS ===
Descida de Gradiente:  a = 1.999956  b = 1.129028  | MSE = 0.073895
Mínimos Quadrados:     a* = 1.999956 b* = 1.128980
Modelo gerador (ideal): a = 2.000000  b = 1.000000



Formato do dataset.csv

Primeira linha: x,y

Linhas seguintes: dois números em ponto flutuante separados por vírgula, por exemplo:

1.000000,3.190011
2.000000,5.093333
...

Perguntas frequentes (bem diretas)

Q: “Por que b fica errado quando não centralizo x?”
A: Porque a atualização de a usa x (multiplicador), então domina a de b. Centralizar equilibra as duas.

Q: “Isso é ‘otimização’?”
A: Não no sentido de micro-otimização de código. É só deixar o problema bem condicionado para o método iterativo funcionar de forma estável.

Q: “Posso medir um antes/depois?”
A: Sim. Rode uma versão sem centralização (baseline) e compare convergência, épocas e MSE final.

Leituras e imagens úteis

Visualização de regressão linear (educacional): LibreTexts. 
Statistics LibreTexts
https://stats.libretexts.org/Bookshelves/Introductory_Statistics/Statistics%3A_Open_for_Everyone_%28Peter%29/13%3A_Simple_Linear_Regression/13.06%3A_Visualizing_Linear_Regression

Conceito de Descida de Gradiente (texto atualizado): GeeksforGeeks. 
GeeksforGeeks
https://www.geeksforgeeks.org/data-science/what-is-gradient-descent/

Notas didáticas (algoritmo e variações): MIT Intro to ML. 
introml.mit.edu
https://introml.mit.edu/notes/gradient_descent.html

“Tigela” do erro (MSE): discussão com figura (bowl-shaped surface). 
Cross Validated
https://stats.stackexchange.com/questions/597520/why-mean-squared-error-surface-takes-bowl-shape
