import serial
import time
import csv
import sys
import os
import re
import math

# ================= CONFIGURAÇÕES =================
PORTA_SERIAL = '/dev/ttyACM0'
BAUDRATE = 115200
ARQUIVO_CSV = 'data/original/dataset_original0.csv'
NUMERO_EXECUCOES = 20
NOME_ARQUIVO_SAIDA = 'resultados_T3_otimizado_O3.txt'
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
    
    print("[AGUARDANDO] Esperando 'READY' do STM32...")
    while True:
        try:
            linha = ser.readline().decode('utf-8', errors='ignore').strip()
        except: continue
        
        if linha:
            if "READY" in linha:
                break

    print(f"[ENVIO] Enviando {len(dados)} pontos...")
    for x, y in dados:
        ser.write(f"{x},{y}\n".encode())
        time.sleep(0.002) 
        while True:
            try:
                resp = ser.readline().decode('utf-8', errors='ignore').strip()
                if "ACK" in resp: break
            except: pass
    
    ser.write(b"FIM\n")
    print("[PROCESSANDO] Aguardando término do treino...")

    while True:
        try:
            linha = ser.readline().decode('utf-8', errors='ignore').strip()
        except: continue

        if "RESULTADO:" in linha:
            print(f"[RETORNO] {linha}")
            
            match_tempo = re.search(r'TEMPO=(\d+)', linha)
            match_epocas = re.search(r'EPOCAS=(\d+)', linha)
            match_mse = re.search(r'mse=([\d\.]+)', linha)
            
            tempo_ms = int(match_tempo.group(1)) if match_tempo else None
            epocas = int(match_epocas.group(1)) if match_epocas else 0
            mse = float(match_mse.group(1)) if match_mse else 0.0

            if tempo_ms is not None:
                print(f"--> Tempo: {tempo_ms} ms | Épocas: {epocas}")
                return {
                    'iteracao': n_ciclo + 1,
                    'tempo': tempo_ms,
                    'epocas': epocas,
                    'mse': mse
                }
            else:
                print("[ERRO] Campo TEMPO não encontrado.")
                return None

def salvar_relatorio_txt(dados_coletados):
    if not dados_coletados:
        print("[AVISO] Sem dados para salvar.")
        return

    tempos = [d['tempo'] for d in dados_coletados]
    media = sum(tempos) / len(tempos)
    variancia = sum((t - media) ** 2 for t in tempos) / len(tempos)
    desvio_padrao = math.sqrt(variancia)
    
    epocas_config = dados_coletados[0]['epocas']

    with open(NOME_ARQUIVO_SAIDA, 'w', encoding='utf-8') as f:
        f.write("============================================================\n")
        # MUDANÇA 2: Título do relatório indica que é a versão OTIMIZADA
        f.write(f" RELATÓRIO DE PERFORMANCE: VERSÃO T3 OTIMIZADA (COMPILADOR)\n")
        f.write("============================================================\n\n")
        
        f.write(f"Data/Hora: {time.strftime('%Y-%m-%d %H:%M:%S')}\n")
        f.write(f"Configuração de Épocas: {epocas_config}\n")
        f.write(f"Total de Execuções: {len(dados_coletados)}\n\n")
        
        f.write(f"{'#':<5} | {'TEMPO (ms)':<12} | {'MSE Final':<15} | {'ÉPOCAS':<8}\n")
        f.write("-" * 50 + "\n")
        
        for dado in dados_coletados:
            f.write(f"{dado['iteracao']:<5} | {dado['tempo']:<12} | {dado['mse']:<15.6f} | {dado['epocas']:<8}\n")
            
        f.write("-" * 50 + "\n\n")
        
        f.write("ESTATÍSTICAS:\n")
        f.write(f"  > Tempo Médio:      {media:.2f} ms\n")
        f.write(f"  > Mínimo:           {min(tempos)} ms\n")
        f.write(f"  > Máximo:           {max(tempos)} ms\n")
        f.write(f"  > Desvio Padrão:    {desvio_padrao:.2f} ms\n")
        f.write("============================================================\n")

    print(f"\n[SUCESSO] Relatório salvo em: {NOME_ARQUIVO_SAIDA}")
    with open(NOME_ARQUIVO_SAIDA, 'r') as f:
        print(f.read())

def main():
    try:
        ser = serial.Serial(PORTA_SERIAL, BAUDRATE, timeout=10)
        time.sleep(2) 
        ser.reset_input_buffer()
    except Exception as e:
        print(f"[ERRO] Falha na serial: {e}")
        return

    pontos = carregar_dados_csv(ARQUIVO_CSV)
    resultados = []

    try:
        for i in range(NUMERO_EXECUCOES):
            resultado = realizar_ciclo(ser, pontos, i)
            if resultado is not None:
                resultados.append(resultado)
            else:
                print("[AVISO] Falha na leitura desta rodada.")

    except KeyboardInterrupt:
        print("\nCancelado pelo usuário.")
    finally:
        ser.close()

    salvar_relatorio_txt(resultados)

if __name__ == "__main__":
    main()