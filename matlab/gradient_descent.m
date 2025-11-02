clear; clc; close all;

%% 1️⃣ Gerar dados simulados (reta real com ruído)
dados = csvread('data/dataset0.csv', 1, 0); % pula 1 linha (cabeçalho)
x = dados(:,1);
y = dados(:,2);


%% 2️⃣ Função de custo (erro quadrático médio)
J = @(a,b) mean((y - (a*x + b)).^2);

%% 3️⃣ Geração de grade para visualizar a topologia 3D
a_vals = linspace(-200, 600, 100);
b_vals = linspace(-100, 300, 100);
[A,B] = meshgrid(a_vals, b_vals);

J_vals = zeros(size(A));
for i = 1:numel(A)
    J_vals(i) = J(A(i), B(i));
end

%% 4️⃣ Derivadas (gradientes)
grad_a = @(a,b) -2/length(x) * sum((y - (a*x + b)) .* x);
grad_b = @(a,b) -2/length(x) * sum(y - (a*x + b));

%% 5️⃣ Exibir a superfície 3D do erro
figure('Position',[100 100 900 600])
surf(A, B, J_vals, 'EdgeColor', 'none')
xlabel('a (inclinação)'), ylabel('b (intercepto)'), zlabel('Erro J(a,b)')
title('Superfície do Erro Quadrático Médio')
colormap('turbo')
hold on

figure;
contour(A,B,J_vals,30); hold on;
plot(trajetoria(:,1),trajetoria(:,2),'r.-','LineWidth',2);
quiver(a,b,-da,-db,0.2,'k','LineWidth',1);
xlabel('a'); ylabel('b'); title('Curvas de Nível e Direção do Gradiente');


%% 6️⃣ Ponto inicial e parâmetros do gradiente
a = 0; b = 0; alpha = 0.05; % taxa de aprendizado
n_iter = 30;

trajetoria = zeros(n_iter,3);

for i = 1:n_iter
    % Calcular gradiente
    da = grad_a(a,b);
    db = grad_b(a,b);

    % Atualizar parâmetros
    a = a - alpha * da;
    b = b - alpha * db;

    % Guardar trajetória
    trajetoria(i,:) = [a, b, J(a,b)];

    % Mostrar seta de gradiente no gráfico
    quiver3(a, b, J(a,b), -da*0.05, -db*0.05, 0, 'Color','k','LineWidth',1);
    plot3(a, b, J(a,b), 'ro', 'MarkerFaceColor','r');
    pause(0.2)

end


%% 7️⃣ Mostrar trajetória do ponto descendo o vale
plot3(trajetoria(:,1), trajetoria(:,2), trajetoria(:,3), 'r.-', 'LineWidth',2, 'MarkerSize',12)
scatter3(trajetoria(1,1), trajetoria(1,2), trajetoria(1,3), 60, 'filled', 'g') % início
scatter3(trajetoria(end,1), trajetoria(end,2), trajetoria(end,3), 60, 'filled', 'r') % final
legend('Superfície do Erro','Vetor Gradiente','Trajetória','Início','Mínimo','Location','northoutside')
view(0,90)

grid on

