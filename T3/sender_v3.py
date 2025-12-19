import serial
import csv
import time
import sys
import os

# ============================================================================
# CONFIGURACOES GERAIS
# ============================================================================

# Porta Serial (Ajuste conforme seu sistema: /dev/ttyACM0, COM3, etc.)
PORTA_SERIAL = '/dev/ttyACM0'
BAUDRATE = 115200
TIMEOUT_SERIAL = 2

# Arquivo de dados para o Modo Gravacao
ARQUIVO_CSV = 'data/dataset0.csv'

# MODO DE OPERACAO DO SCRIPT
# 'GRAVAR': Envia dados para a Flash (STM32 deve estar com MODE_FLASH_LOOP comentado)
# 'MONITORAR': Apenas le os prints do treino (STM32 deve estar com MODE_FLASH_LOOP ativo)

MODO_OPERACAO = 'GRAVAR' 
# MODO_OPERACAO = 'MONITORAR' 

# ============================================================================
# FUNCOES DE UTILIDADE
# ============================================================================

def conectar_serial():
    try:
        # Timeout ajuda a nao travar o script se o cabo desconectar
        ser = serial.Serial(PORTA_SERIAL, BAUDRATE, timeout=TIMEOUT_SERIAL)
        time.sleep(2) # Aguarda o reset da porta serial (importante no Linux/Arduino)
        ser.reset_input_buffer()
        print(f"[INFO] Conectado a porta {PORTA_SERIAL}")
        return ser
    except serial.SerialException as e:
        print(f"[ERRO] Nao foi possivel abrir a porta serial: {e}")
        print("Dica: Verifique se a porta esta correta ou se precisa de permissao (sudo).")
        sys.exit(1)

def carregar_dados_csv(caminho):
    if not os.path.exists(caminho):
        print(f"[ERRO] Arquivo CSV nao encontrado: {caminho}")
        sys.exit(1)
    
    dados = []
    try:
        with open(caminho, 'r') as f:
            leitor = csv.reader(f)
            # Tenta pular o cabeçalho se existir
            try:
                header = next(leitor, None)
            except StopIteration:
                pass
                
            for linha in leitor:
                # Garante que a linha tem pelo menos 2 colunas e nao esta vazia
                if linha and len(linha) >= 2:
                    dados.append((linha[0], linha[1]))
    except Exception as e:
        print(f"[ERRO] Falha ao ler CSV: {e}")
        sys.exit(1)
    
    print(f"[INFO] {len(dados)} pontos carregados do arquivo.")
    return dados

# ============================================================================
# LOGICA: MODO GRAVACAO (Flash)
# ============================================================================

def executar_modo_gravacao(ser):
    print("\n--- INICIANDO MODO GRAVACAO ---")
    print("Certifique-se de que '#define MODE_FLASH_LOOP' esta COMENTADO no C.")
    print("DICA: Se travar em 'AGUARDANDO', aperte o botao RESET da placa.\n")
    
    pontos = carregar_dados_csv(ARQUIVO_CSV)
    
    print("[AGUARDANDO] Esperando o STM32 enviar 'READY'...")
    sincronizado = False
    
    # 1. Aguarda sinal READY do microcontrolador
    # Loop infinito ate receber o sinal ou o usuario cancelar
    while not sincronizado:
        try:
            linha = ser.readline().decode('utf-8', errors='ignore').strip()
            if linha:
                print(f"[STM32] {linha}")
            
            if "READY" in linha:
                sincronizado = True
                print("[INFO] Sincronizacao OK! Iniciando envio...")
        except KeyboardInterrupt:
            print("\n[CANCELADO] Usuario cancelou durante o handshake.")
            sys.exit(0)

    # 2. Envia dados passo a passo (Handshake)
    total_enviados = 0
    inicio = time.time()
    
    for x, y in pontos:
        msg = f"{x},{y}\n"
        ser.write(msg.encode('utf-8'))
        
        # --- CORRECAO CRITICA ---
        # Pausa de 50ms para dar tempo do STM32 gravar na Flash
        # Sem isso, o buffer da serial estoura e o STM32 perde dados.
        time.sleep(0.05) 
        # ------------------------
        
        # Aguarda ACK
        resposta = ser.readline().decode('utf-8', errors='ignore').strip()
        
        if resposta == "ACK":
            total_enviados += 1
            if total_enviados % 50 == 0:
                print(f"[PROGRESSO] {total_enviados} pontos gravados...")
        elif resposta == "NACK":
            print(f"[ERRO] STM32 retornou NACK para o ponto {x},{y}")
        else:
            # Se nao receber ACK, imprime o que recebeu (pode ser erro de debug)
            if resposta:
                print(f"[DEBUG] Recebido estranho: {resposta}")

    tempo_total = time.time() - inicio
    print(f"[CONCLUIDO] {total_enviados} pontos enviados em {tempo_total:.2f}s")

    # 3. Finaliza
    print("[INFO] Enviando comando FIM...")
    time.sleep(0.1)
    ser.write(b"FIM\n")
    
    resp_fim = ser.readline().decode('utf-8', errors='ignore').strip()
    if "ACK_FIM" in resp_fim:
        print("[SUCESSO] STM32 confirmou o fim da gravacao.")
        print(">> AGORA: No STM32, descomente MODE_FLASH_LOOP e regrave o codigo.")
    else:
        print(f"[ALERTA] Resposta final inesperada: {resp_fim}")

# ============================================================================
# LOGICA: MODO MONITORAMENTO (Loop RAM)
# ============================================================================

def executar_modo_monitoramento(ser):
    print("\n--- INICIANDO MODO MONITORAMENTO ---")
    print("Certifique-se de que '#define MODE_FLASH_LOOP' esta DESCOMENTADO no C.")
    print("Pressione Ctrl+C para encerrar.\n")
    
    try:
        while True:
            linha = ser.readline().decode('utf-8', errors='ignore').strip()
            
            if linha:
                # Destaca linhas de resultado
                if "RESULTADO:" in linha:
                    print(f"\033[92m[DADOS]\033[0m {linha}") # Verde no Linux
                # Destaca mensagens de sistema
                elif ">>" in linha:
                    print(f"\033[93m[SISTEMA]\033[0m {linha}") # Amarelo no Linux
                # Outras mensagens
                else:
                    print(f"[STM32] {linha}")
                    
    except KeyboardInterrupt:
        print("\n[INFO] Monitoramento interrompido pelo usuario.")

# ============================================================================
# MAIN
# ============================================================================

if __name__ == "__main__":
    ser = conectar_serial()
    
    try:
        if MODO_OPERACAO == 'GRAVAR':
            executar_modo_gravacao(ser)
        elif MODO_OPERACAO == 'MONITORAR':
            executar_modo_monitoramento(ser)
        else:
            print("[ERRO] MODO_OPERACAO invalido na linha 23. Use 'GRAVAR' ou 'MONITORAR'.")
    except KeyboardInterrupt:
        print("\n[SAIDA] Programa encerrado.")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("[INFO] Conexao serial fechada.")