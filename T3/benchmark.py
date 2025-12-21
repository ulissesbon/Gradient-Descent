import serial
import time
import csv
import sys
import os
import re

# ================= CONFIGURAÇÕES =================
PORTA_SERIAL = '/dev/ttyACM0'
BAUDRATE = 115200
ARQUIVO_CSV = 'data/original/dataset_original0.csv'
NUMERO_EXECUCOES = 20
# =================================================

def carregar_dados_csv(caminho):
    dados = []
    if not os.path.exists(caminho):
        print(f"[ERRO] Arquivo {caminho} não encontrado.")
        sys.exit(1)
    with open(caminho, 'r') as f:
        leitor = csv.reader(f)
        try:
            next(leitor) # Pula cabeçalho se existir
        except: pass
        for linha in leitor:
            if len(linha) >= 2:
                dados.append((linha[0], linha[1]))
    return dados

def realizar_ciclo(ser, dados, n_ciclo):
    print(f"\n--- EXECUÇÃO {n_ciclo + 1}/{NUMERO_EXECUCOES} ---")
    
    # 1. Aguarda o STM32 estar pronto (reset ou loop)
    # O STM32 envia "READY" quando termina de apagar a flash e está pronto
    print("[AGUARDANDO] Esperando 'READY' do STM32...")
    while True:
        linha = ser.readline().decode('utf-8', errors='ignore').strip()
        if linha:
            # print(f"[STM32] {linha}") # Descomente para debug
            if "READY" in linha:
                break

    # 2. Envia os dados
    print(f"[ENVIO] Enviando {len(dados)} pontos...")
    for x, y in dados:
        ser.write(f"{x},{y}\n".encode())
        time.sleep(0.002) # Pequeno delay para não estourar o buffer da UART
        # Espera ACK (opcional para velocidade, mas seguro)
        while True:
            resp = ser.readline().decode('utf-8', errors='ignore').strip()
            if "ACK" in resp: break
    
    # 3. Finaliza envio e inicia treino
    ser.write(b"FIM\n")
    print("[PROCESSANDO] Aguardando término do treino...")

    # 4. Aguarda o resultado com o tempo
    while True:
        linha = ser.readline().decode('utf-8', errors='ignore').strip()
        if "RESULTADO:" in linha:
            print(f"[RETORNO] {linha}")
            
            # Extrai o tempo usando Regex
            # Procura por "TEMPO=123" na string
            match = re.search(r'TEMPO=(\d+)', linha)
            if match:
                tempo_ms = int(match.group(1))
                print(f"--> Tempo medido: {tempo_ms} ms")
                return tempo_ms
            else:
                print("[ERRO] Campo TEMPO não encontrado na resposta.")
                return None

def main():
    try:
        ser = serial.Serial(PORTA_SERIAL, BAUDRATE, timeout=10)
        time.sleep(2) # Reset do Arduino/STM32
        ser.reset_input_buffer()
    except Exception as e:
        print(f"[ERRO] Falha na serial: {e}")
        return

    pontos = carregar_dados_csv(ARQUIVO_CSV)
    tempos = []

    try:
        for i in range(NUMERO_EXECUCOES):
            t = realizar_ciclo(ser, pontos, i)
            if t is not None:
                tempos.append(t)
            else:
                print("[AVISO] Leitura falhou nesta rodada.")

    except KeyboardInterrupt:
        print("\nCancelado pelo usuário.")
    finally:
        ser.close()

    # Estatísticas Finais
    if tempos:
        media = sum(tempos) / len(tempos)
        print("\n" + "="*40)
        print(" RELATÓRIO FINAL DE PERFORMANCE")
        print("="*40)
        print(f"Execuções válidas: {len(tempos)}")
        print(f"Tempos (ms): {tempos}")
        print(f"MÉDIA: {media:.2f} ms")
        print("="*40)
    else:
        print("\nNenhum dado de tempo coletado.")

if __name__ == "__main__":
    main()