import serial
import time
import csv
import sys
import os
import re
import math
import random

# ================= CONFIGURAÇÕES =================
PORTA_SERIAL = '/dev/ttyACM0'  
BAUDRATE = 115200
ARQUIVO_CSV = 'data/original/dataset_original0.csv'
NUMERO_EXECUCOES = 20
TAMANHO_BATCH = 550
NOME_ARQUIVO_SAIDA = 'resultados_ram_shuffle_550.txt'
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

def realizar_ciclo(ser, dados_para_enviar, n_ciclo):
    """
    Recebe uma lista de dados JÁ EMBARALHADA e envia.
    """
    print(f"\n--- EXECUÇÃO {n_ciclo + 1}/{NUMERO_EXECUCOES} ---")
    print(f"[PYTHON] Enviando batch de {len(dados_para_enviar)} pontos (Ordem Aleatória).")

    # 1. Aguarda o STM32
    print("[AGUARDANDO] Esperando 'READY' do STM32...")
    while True:
        try:
            linha = ser.readline().decode('utf-8', errors='ignore').strip()
        except: continue
        if linha and "READY" in linha:
            break

    # 2. Envia os dados
    print(f"[ENVIO] Transmitindo...")
    for x, y in dados_para_enviar:
        ser.write(f"{x},{y}\n".encode())
        time.sleep(0.002) 
        while True:
            try:
                resp = ser.readline().decode('utf-8', errors='ignore').strip()
                if "ACK" in resp: break
            except: pass
    
    # 3. Finaliza envio
    ser.write(b"FIM\n")
    print("[PROCESSANDO] Aguardando término do treino...")

    # 4. Captura Resultados
    while True:
        try:
            linha = ser.readline().decode('utf-8', errors='ignore').strip()
        except: continue

        if "RESULTADO:" in linha:
            print(f"[RETORNO] {linha}")
            
            # Regex completo para todos os campos
            match_a = re.search(r'a=([\d\.\-eE]+)', linha)
            match_b = re.search(r'b=([\d\.\-eE]+)', linha)
            match_mse = re.search(r'mse=([\d\.\-eE]+)', linha)
            match_tempo = re.search(r'TEMPO=(\d+)', linha)
            match_epocas = re.search(r'EPOCAS=(\d+)', linha)
            
            if match_tempo and match_a and match_b and match_mse:
                tempo_ms = int(match_tempo.group(1))
                tempo_s = tempo_ms / 1000.0
                
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
        f.write(f" RELATÓRIO: TREINAMENTO RAM (VALORES FIXOS, ORDEM SHUFFLED)\n")
        f.write("=================================================================================\n\n")
        f.write(f"Data/Hora: {time.strftime('%Y-%m-%d %H:%M:%S')}\n")
        f.write(f"Dataset Base: {ARQUIVO_CSV}\n")
        f.write(f"Amostras Fixas: {TAMANHO_BATCH} (Mesmos valores, ordem variável)\n")
        f.write(f"Configuração de Épocas: {epocas_config}\n")
        f.write(f"Total de Execuções: {len(dados_coletados)}\n\n")
        
        # Tabela Detalhada
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

    print(f"[INFO] Carregando dataset base...")
    pontos_todos = carregar_dados_csv(ARQUIVO_CSV)
    print(f"[INFO] Total de pontos no CSV: {len(pontos_todos)}")

    if len(pontos_todos) < TAMANHO_BATCH:
        print(f"[ERRO] Dataset tem menos que {TAMANHO_BATCH} pontos.")
        return

    # --- LÓGICA DE SELEÇÃO FIXA ---
    # Seleciona os primeiros 550 pontos (ou qualquer outra fatia) UMA ÚNICA VEZ.
    # Esses são os "Valores Fixos" que serão usados em todas as 20 rodadas.
    batch_fixo = pontos_todos[:TAMANHO_BATCH]
    print(f"[CONFIG] Batch de {len(batch_fixo)} pontos FIXADO para o teste.")

    resultados = []

    try:
        for i in range(NUMERO_EXECUCOES):
            # --- LÓGICA DE EMBARALHAMENTO ---
            # Cria uma cópia dos valores fixos e embaralha APENAS A ORDEM
            batch_da_vez = batch_fixo[:] 
            random.shuffle(batch_da_vez)
            
            res = realizar_ciclo(ser, batch_da_vez, i)
            if res: resultados.append(res)
            
    except KeyboardInterrupt:
        print("\nCancelado.")
    finally:
        ser.close()

    salvar_relatorio_txt(resultados)

if __name__ == "__main__":
    main()