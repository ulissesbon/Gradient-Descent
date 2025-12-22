import serial
import time
import csv
import sys
import os
import re
import math

# ================= CONFIGURAÇÕES =================
PORTA_SERIAL = '/dev/ttyACM0'  # Linux/Mac. No Windows use 'COM3', 'COM4'
BAUDRATE = 115200
ARQUIVO_CSV = 'data/original/dataset_original0.csv' # Caminho do dataset
NUMERO_EXECUCOES = 20
NOME_ARQUIVO_SAIDA = 'resultados_T3_original.txt'
# =================================================

def carregar_dados_csv(caminho):
    dados = []
    if not os.path.exists(caminho):
        print(f"[ERRO] Arquivo {caminho} não encontrado.")
        sys.exit(1)
    with open(caminho, 'r') as f:
        leitor = csv.reader(f)
        try:
            next(leitor)
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
        if linha and "READY" in linha:
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
            
            # Regex para capturar float (incluindo notação científica)
            match_a = re.search(r'a=([\d\.\-eE]+)', linha)
            match_b = re.search(r'b=([\d\.\-eE]+)', linha)
            match_mse = re.search(r'mse=([\d\.\-eE]+)', linha)
            match_tempo = re.search(r'TEMPO=(\d+)', linha)
            match_epocas = re.search(r'EPOCAS=(\d+)', linha)
            
            if match_tempo and match_a and match_b and match_mse:
                tempo_ms = int(match_tempo.group(1))
                tempo_s = tempo_ms / 1000.0  # Converte ms para segundos
                
                res = {
                    'iteracao': n_ciclo + 1,
                    'tempo_s': tempo_s,
                    'a': float(match_a.group(1)),
                    'b': float(match_b.group(1)),
                    'mse': float(match_mse.group(1)),
                    'epocas': int(match_epocas.group(1)) if match_epocas else 0
                }
                print(f"--> Tempo: {res['tempo_s']:.3f} s | MSE: {res['mse']:.6f}")
                return res
            else:
                print("[ERRO] Falha no parse dos dados.")
                return None

def salvar_relatorio_txt(dados_coletados):
    if not dados_coletados:
        print("[AVISO] Sem dados para salvar.")
        return

    tempos = [d['tempo_s'] for d in dados_coletados]
    media = sum(tempos) / len(tempos)
    variancia = sum((t - media) ** 2 for t in tempos) / len(tempos)
    desvio_padrao = math.sqrt(variancia)
    epocas_config = dados_coletados[0]['epocas']

    with open(NOME_ARQUIVO_SAIDA, 'w', encoding='utf-8') as f:
        f.write("=================================================================================\n")
        f.write(f" RELATÓRIO DE PERFORMANCE: VERSÃO T3 ORIGINAL\n")
        f.write("=================================================================================\n\n")
        f.write(f"Configuração de Épocas: {epocas_config}\n")
        f.write(f"Total de Execuções: {len(dados_coletados)}\n\n")
        
        header = f"{'#':<3} | {'Tempo (s)':<10} | {'A (Inclinação)':<15} | {'B (Intercepto)':<15} | {'MSE Final':<15}\n"
        f.write(header)
        f.write("-" * len(header) + "\n")
        
        for d in dados_coletados:
            line = f"{d['iteracao']:<3} | {d['tempo_s']:<10.3f} | {d['a']:<15.6f} | {d['b']:<15.6f} | {d['mse']:<15.6f}\n"
            f.write(line)
            
        f.write("-" * len(header) + "\n\n")
        f.write("ESTATÍSTICAS DE TEMPO:\n")
        f.write(f"  > Média:            {media:.3f} s\n")
        f.write(f"  > Desvio Padrão:    {desvio_padrao:.3f} s\n")
        f.write(f"  > Mínimo:           {min(tempos):.3f} s\n")
        f.write(f"  > Máximo:           {max(tempos):.3f} s\n")
        f.write("=================================================================================\n")

    print(f"\n[SUCESSO] Relatório salvo em: {NOME_ARQUIVO_SAIDA}")

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
            res = realizar_ciclo(ser, pontos, i)
            if res: resultados.append(res)
    except KeyboardInterrupt:
        print("\nCancelado.")
    finally:
        ser.close()

    salvar_relatorio_txt(resultados)

if __name__ == "__main__":
    main()