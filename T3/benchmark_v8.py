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
NUMERO_EXECUCOES_TESTE = 5 # Quantas vezes rodar o benchmark completo
EPOCAS_SGD = 5             # Quantas vezes enviar o dataset inteiro para o STM32 aprender
NOME_ARQUIVO_SAIDA = 'resultados_sgd_online.txt'
# =================================================

def carregar_dados_csv(caminho):
    dados = []
    if not os.path.exists(caminho):
        print(f"[ERRO] Arquivo {caminho} não encontrado.")
        sys.exit(1)
    with open(caminho, 'r') as f:
        leitor = csv.reader(f)
        try: next(leitor)
        except: pass
        for linha in leitor:
            if len(linha) >= 2:
                dados.append((linha[0], linha[1]))
    return dados

def realizar_ciclo(ser, dados_completos, n_ciclo):
    print(f"\n--- TESTE {n_ciclo + 1}/{NUMERO_EXECUCOES_TESTE} ---")
    
    # Reset buffer
    ser.reset_input_buffer()
    
    # Aguarda READY
    print("[AGUARDANDO] Esperando 'READY'...")
    while True:
        try:
            linha = ser.readline().decode('utf-8', errors='ignore').strip()
            if "READY" in linha: break
        except: pass

    # Inicia Streaming de Dados (SGD)
    print(f"[SGD] Enviando dataset {EPOCAS_SGD} vezes (Simulando Épocas via UART)...")
    
    total_pontos_enviados = 0
    t_inicio = time.time()

    for epoca in range(EPOCAS_SGD):
        # Embaralha a cada época para melhorar a convergência do SGD
        batch_atual = dados_completos[:]
        random.shuffle(batch_atual)
        
        # print(f"  > Enviando Época {epoca+1}/{EPOCAS_SGD}...")
        
        for x, y in batch_atual:
            ser.write(f"{x},{y}\n".encode())
            # Pequeno delay para a UART não engasgar
            time.sleep(0.001) 
            
            # Aguarda ACK (Handshake ponto a ponto)
            while True:
                try:
                    resp = ser.readline().decode('utf-8', errors='ignore').strip()
                    if "ACK" in resp: break
                except: pass
            total_pontos_enviados += 1
            
    t_fim = time.time()
    duracao_envio = t_fim - t_inicio
    
    # Finaliza
    ser.write(b"FIM\n")
    print(f"[SGD] Fim do envio. Duração total transmissão: {duracao_envio:.2f}s")

    # Captura Resultado
    while True:
        try:
            linha = ser.readline().decode('utf-8', errors='ignore').strip()
        except: continue

        if "RESULTADO:" in linha:
            print(f"[RETORNO] {linha}")
            
            match_a = re.search(r'a=([-\d\.eEnNanInf]+)', linha)
            match_b = re.search(r'b=([-\d\.eEnNanInf]+)', linha)
            match_mse = re.search(r'mse=([-\d\.eEnNanInf]+)', linha)
            
            if match_a and match_b and match_mse:
                try:
                    res = {
                        'iteracao': n_ciclo + 1,
                        'a': float(match_a.group(1)),
                        'b': float(match_b.group(1)),
                        'mse': float(match_mse.group(1)),
                        'samples': total_pontos_enviados
                    }
                    print(f"--> Modelo: a={res['a']:.4f}, b={res['b']:.4f}")
                    return res
                except: return None

def salvar_relatorio_txt(dados_coletados):
    if not dados_coletados: return

    with open(NOME_ARQUIVO_SAIDA, 'w', encoding='utf-8') as f:
        f.write("=================================================================================\n")
        f.write(f" RELATÓRIO: SGD ONLINE (MEMORIA O(1))\n")
        f.write("=================================================================================\n")
        f.write(f"Dataset Base: {ARQUIVO_CSV}\n")
        f.write(f"Épocas (Re-envios Python): {EPOCAS_SGD}\n\n")
        
        header = f"{'#':<3} | {'Samples':<8} | {'A':<15} | {'B':<15} | {'MSE (Médio)':<15}\n"
        f.write(header + "-"*len(header) + "\n")
        
        for d in dados_coletados:
            f.write(f"{d['iteracao']:<3} | {d['samples']:<8} | {d['a']:<15.6f} | {d['b']:<15.6f} | {d['mse']:<15.6f}\n")

    print(f"\n[SUCESSO] Relatório salvo: {NOME_ARQUIVO_SAIDA}")

def main():
    try:
        ser = serial.Serial(PORTA_SERIAL, BAUDRATE, timeout=10)
        time.sleep(2) 
        ser.reset_input_buffer()
    except Exception as e:
        print(f"[ERRO] Serial: {e}")
        return

    pontos = carregar_dados_csv(ARQUIVO_CSV)
    # Aqui usamos TODOS os pontos do CSV, pois não temos limite de RAM!
    print(f"[CONFIG] Dataset carregado: {len(pontos)} pontos.")

    resultados = []
    try:
        for i in range(NUMERO_EXECUCOES_TESTE):
            res = realizar_ciclo(ser, pontos, i)
            if res: resultados.append(res)
    except KeyboardInterrupt:
        print("\nCancelado.")
    finally:
        ser.close()

    salvar_relatorio_txt(resultados)

if __name__ == "__main__":
    main()