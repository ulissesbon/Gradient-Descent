import serial
import csv
import time

# ============================================================================
# CONFIGURAÇÃO INICIAL
# ============================================================================
# Abre a porta serial. Certifique-se que o STM32 está plugado.
porta = serial.Serial('/dev/ttyACM0', 115200)

# IMPORTANTE: Ao abrir a serial, muitas placas (Arduino/STM32) reiniciam automaticamente.
# Esse sleep de 2s dá tempo para o boot do microcontrolador completar antes de enviarmos dados.
time.sleep(2)

arquivo = 'data/dataset0.csv'

# ============================================================================
# FASE 1: CÁLCULO DAS MÉDIAS (Normalização)
# ============================================================================
# O STM32 precisa saber a média de X e Y para "centralizar" os dados.
# Como ele não tem os dados na memória, enviamos o arquivo inteiro uma vez só para isso.

print("\n=== Passagem 1: Enviando dados para cálculo das médias ===")

with open(arquivo, 'r') as f:
    leitor = csv.reader(f)
    next(leitor, None)  # Pula a linha do cabeçalho (ex: "x,y")
    
    for linha in leitor:
        x, y = linha
        
        # Envia o par X,Y para o STM32 somar nas variáveis acumuladoras
        porta.write(f"{x},{y}\n".encode())
        
        # TIMING CRÍTICO: O STM32 precisa de tempo para ler, converter string->float e somar.
        # Se enviar rápido demais sem controle de fluxo, o buffer da UART estoura.
        time.sleep(0.01) 

# Comando especial que avisa o STM32: "Acabaram os dados da média. Calcule a divisão final."
porta.write(b"FIM_MEDIA\n")
print("Dados de média enviados. Aguardando processamento do MCU...")

# ============================================================================
# PAUSA DE SINCRONIZAÇÃO
# ============================================================================
# Aqui o código original comentou a leitura da resposta (handshake).
# O 'time.sleep(4)' é uma "espera cega": assume que em 4 segundos o STM32
# terminou de calcular a média e já preparou as variáveis para o treino.
# (O ideal seria descomentar as linhas abaixo para ter certeza que o MCU está pronto)
# resp = porta.readline().decode(errors='ignore').strip()
# print(f"MCU Diz: {resp}")

time.sleep(4) 

# ============================================================================
# FASE 2: LOOP DE TREINAMENTO (Épocas)
# ============================================================================
# O Gradient Descent precisa passar pelos mesmos dados várias vezes (Épocas).
# Como o STM32 não guardou os dados, o PC precisa ler o arquivo e reenviar TUDO novamente
# para CADA época.

print("\n=== Passagem 2: Treino (Reenviando dados por Época) ===")

# Repete o envio do arquivo 30 vezes (simulando 30 épocas de treino)
for epoca in range(30): 
    print(f"\n--- Iniciando Envio da Época {epoca+1}/30 ---")
    
    # Reabre o arquivo do zero a cada volta do loop
    with open(arquivo, 'r') as f:
        leitor = csv.reader(f)
        next(leitor, None) # Pula cabeçalho novamente
        
        for linha in leitor:
            x, y = linha
            
            # Envia o dado para o cálculo do gradiente
            porta.write(f"{x},{y}\n".encode())
            
            # Delay ligeiramente menor (0.005s) assumindo que o cálculo do gradiente 
            # de um ponto é rápido. Se travar, aumente este valor.
            time.sleep(0.005)
            
    # Ao final de cada leitura do arquivo, avisa o STM32 que a época acabou.
    # O STM32 vai atualizar os pesos (a, b) agora.
    porta.write(b"FIM_TREINO\n")

print("Todas as épocas enviadas. Aguardando resultado final...")

# (Opcional) Aqui você poderia adicionar um porta.readline() para ver o 'a' e 'b' finais

print("Simulação completa!")