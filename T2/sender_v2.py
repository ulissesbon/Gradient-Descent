import serial
import csv
import sys

# ============================================================================
# CONFIGURAÇÃO DA COMUNICAÇÃO SERIAL
# ============================================================================
# Define a porta onde o STM32 está conectado (Linux: /dev/tty..., Windows: COMx)
PORTA = '/dev/ttyACM0' 

# Baudrate deve ser IDÊNTICO ao configurado no main.c (huart2.Init.BaudRate)
BAUDRATE = 115200 

try:
    # Abre a porta serial. 
    # timeout=2 significa: se tentar ler e não chegar nada em 2 segundos, o código continua.
    # Isso impede que o Python trave para sempre se o cabo desconectar.
    porta = serial.Serial(PORTA, BAUDRATE, timeout=2)
    
    # Limpa qualquer lixo que tenha ficado no buffer da serial antes de começar
    porta.reset_input_buffer()
    
except serial.SerialException as e:
    print(f"Erro crítico ao abrir serial: {e}")
    sys.exit(1) # Encerra o script indicando erro

arquivo = 'data/dataset0.csv'

print(f"Conectado em {PORTA}. Aguardando STM32 inicializar...")

# ============================================================================
# 1. SINCRONIZAÇÃO INICIAL (HANDSHAKE)
# ============================================================================
# O STM32 executa 'flash_dataset_erase()' no boot, o que trava a CPU dele por 
# alguns milissegundos. Se enviarmos dados agora, eles seriam perdidos.
# Por isso, ficamos num loop passivo esperando o STM32 gritar "READY".

stm32_pronto = False
print("[AGUARDANDO] Pressione o botão RESET na placa se esta mensagem não sair daqui...")

while not stm32_pronto:
    # Lê uma linha inteira da serial
    # decode: converte bytes para string. errors='ignore': evita crash se vier lixo
    linha = porta.readline().decode(errors='ignore').strip()
    
    if linha:
        print(f"[STM32 Diz] {linha}") # Mostra tudo que a placa fala no boot
    
    # Se a palavra mágica "READY" aparecer, sabemos que o erase terminou
    if "READY" in linha:
        stm32_pronto = True
        print("[SUCESSO] Sincronia estabelecida! Iniciando envio...")

print("\n=== Iniciando envio de dados para a FLASH (Modo Seguro) ===")

# ============================================================================
# 2. ENVIO DOS DADOS (PROTOCOLO STOP-AND-WAIT)
# ============================================================================
# A gravação na Flash é lenta. Se o Python mandar tudo de uma vez, o buffer do 
# STM32 estoura (Overrun).
# A Lógica: Envia 1 ponto -> Espera confirmação (ACK) -> Envia próximo.

total_enviados = 0

try:
    with open(arquivo, 'r') as f:
        leitor = csv.reader(f)
        next(leitor, None)  # Pula a primeira linha (cabeçalho: x,y)
        
        for linha_csv in leitor:
            if not linha_csv: 
                continue # Ignora linhas vazias
            
            x, y = linha_csv
            mensagem = f"{x},{y}\n" # Formato esperado pelo sscanf no C
            
            # --- Passo A: Envia ---
            porta.write(mensagem.encode())
            
            # --- Passo B: Espera confirmação (Bloqueante) ---
            # O Python fica parado aqui até o STM32 gravar na flash e responder.
            confirmacao = porta.readline().decode(errors='ignore').strip()
            
            # --- Passo C: Verifica a resposta ---
            if confirmacao == "ACK":
                total_enviados += 1
                # Feedback visual a cada 10 pontos para não poluir o terminal
                if total_enviados % 10 == 0:
                    print(f"\rPontos gravados: {total_enviados}...", end='', flush=True)
            
            elif confirmacao == "NACK":
                # O STM32 avisa se falhou ao gravar
                print(f"\n[ERRO] STM32 rejeitou o ponto {x},{y}")
            
            else:
                # Se receber algo que não é ACK nem NACK, pode ser erro de sincronia
                # ou prints de debug que esquecemos no código C.
                print(f"\n[ALERTA] Resposta inesperada: '{confirmacao}'")

except KeyboardInterrupt:
    print("\n[CANCELADO] Envio interrompido pelo usuário.")
    sys.exit(0)

print(f"\nEnvio concluído. Total: {total_enviados} pontos.")

# ============================================================================
# 3. FINALIZAÇÃO DO ENVIO
# ============================================================================
# Avisa o STM32 para sair do loop de gravação e ir para o treinamento

print("Enviando comando FIM...")
porta.write(b"FIM\n")

# Espera o STM32 confirmar que entendeu que acabou
resp = porta.readline().decode(errors='ignore').strip()
if "ACK_FIM" in resp:
    print("STM32 confirmou fim da gravação.")
else:
    print(f"Resposta estranha ao FIM: {resp}")

# ============================================================================
# 4. MONITORAMENTO DOS RESULTADOS (LOOP INFINITO)
# ============================================================================
# Agora o STM32 está rodando o Gradient Descent. O Python atua apenas como
# um "monitor" passivo mostrando o que chega