"""
Script para enviar dataset embaralhado para STM32F030R8 (Versão Robusta)
Correção: Sincronização de Reset e Pace Control
"""

import serial
import pandas as pd
import time
import sys
from pathlib import Path

# ============================================================================
# CONFIGURAÇÕES
# ============================================================================

PORTA_SERIAL = '/dev/ttyACM0'
BAUDRATE = 115200
TIMEOUT_CONEXAO = 10  # Tempo para aguardar o boot do STM32

NUM_PONTOS_ENVIAR = 550
RANDOM_SEED = 42

# Pequeno atraso entre linhas para não estourar o buffer físico da UART
# O STM32 precisa de tempo para processar o sscanf e floats
DELAY_ENTRE_LINHAS = 0.005  # 5ms

# ============================================================================
# FUNÇÕES
# ============================================================================

def carregar_dataset(csv_path, num_pontos, random_seed=None):
    print(f"\nCarregando: {csv_path}")
    try:
        df = pd.read_csv(csv_path)
    except Exception as e:
        print(f"[ERRO] {e}")
        return None
    
    # Embaralhar
    if random_seed is not None:
        df = df.sample(n=num_pontos, random_state=random_seed).reset_index(drop=True)
    else:
        df = df.sample(n=num_pontos).reset_index(drop=True)
        
    print(f"Dataset pronto: {len(df)} pontos selecionados.")
    return df

def conectar_e_sincronizar(porta, baudrate):
    """
    Conecta e lê o fluxo de dados até encontrar 'READY'.
    Lida com o fato do STM32 reiniciar ao abrir a porta.
    """
    print(f"\nConectando em {porta}...")
    
    try:
        # DTR/RTS false tenta evitar reset em alguns drivers, mas nem sempre funciona
        ser = serial.Serial(porta, baudrate, timeout=1, dsrdtr=False, rtscts=False)
        
        # Não dormimos fixamente. Lemos o que vier.
        print("Aguardando reinicialização e comando READY do STM32...")
        
        inicio = time.time()
        
        while (time.time() - inicio) < TIMEOUT_CONEXAO:
            # Lê linha a linha
            linha = ser.readline().decode('utf-8', errors='ignore').strip()
            
            if linha:
                print(f"   [STM32]: {linha}")
                
                # Se o STM32 reiniciou, apenas informamos
                if "Sistema iniciado" in linha:
                    print("   [INFO] STM32 Reiniciou. Aguardando READY...")
                
                # O sinal que esperamos
                if "READY" in linha:
                    try:
                        pts = int(linha.split(':')[1])
                        print(f"Sincronizado! STM32 pede {pts} pontos.")
                        return ser, pts
                    except:
                        # Se vier só READY sem numero (versões antigas)
                        return ser, NUM_PONTOS_ENVIAR
                        
        print("[TIMEOUT] Aguardando READY.")
        return None, 0
        
    except serial.SerialException as e:
        print(f"[ERRO] Serial: {e}")
        return None, 0

def enviar_dados_com_ritmo(ser, df):
    """
    Envia dados sem esperar ACK linha a linha, mas com um micro-delay
    para dar tempo da CPU do STM32 processar o float.
    """
    total = len(df)
    print(f"\nEnviando {total} pontos...")
    inicio = time.time()
    
    for i, row in df.iterrows():
        msg = f"{row['x']:.4f},{row['y']:.4f}\n"
        ser.write(msg.encode('utf-8'))
        
        # Feedback visual a cada 50 pontos para não poluir o terminal
        if i % 50 == 0:
            sys.stdout.write(f"\r   Progresso: {i}/{total} pontos...")
            sys.stdout.flush()
            
        # O PULO DO GATO: Pequena pausa para o STM32 respirar
        time.sleep(DELAY_ENTRE_LINHAS)
        
    ser.flush()
    print(f"\nEnvio concluído em {time.time() - inicio:.2f}s")
    return True

def aguardar_resultado(ser):
    print("\nAguardando processamento e resultado...")
    print("   (O STM32 está calculando o gradiente...)")
    
    # Timeout generoso para o treino
    timeout_treino = 30 
    inicio = time.time()
    
    while (time.time() - inicio) < timeout_treino:
        linha = ser.readline().decode('utf-8', errors='ignore').strip()
        
        if linha:
            print(f"   [STM32]: {linha}")
            
            if "RESULTADO:" in linha:
                return linha
            
            # Se ele pedir READY de novo, algo deu errado e ele reiniciou o loop
            if "READY" in linha:
                print("[AVISO] STM32 voltou para o estado READY antes de dar o resultado.")
                return None

    print("[TIMEOUT] Aguardando resultado final.")
    return None

# ============================================================================
# MAIN
# ============================================================================

if __name__ == "__main__":
    csv_file = "data/dataset0.csv" 
    if len(sys.argv) > 1: csv_file = sys.argv[1]

    # 1. Preparar Dados
    df = carregar_dataset(csv_file, NUM_PONTOS_ENVIAR, RANDOM_SEED)
    if df is None: sys.exit(1)

    # 2. Conectar (Sincronizado)
    ser, qtd_solicitada = conectar_e_sincronizar(PORTA_SERIAL, BAUDRATE)
    if not ser: sys.exit(1)

    try:
        # Ajustar dataset se o STM32 pedir quantidade diferente
        if len(df) != qtd_solicitada:
            print(f"[AVISO] Ajustando envio para {qtd_solicitada} pontos conforme pedido.")
            df = df.iloc[:qtd_solicitada]

        # 3. Enviar Dados
        enviar_dados_com_ritmo(ser, df)

        # 4. Enviar FIM
        time.sleep(0.5) # Pausa dramática para garantir separação
        ser.write(b"FIM\n")
        print("Comando FIM enviado.")

        # 5. Ler Resultado
        res = aguardar_resultado(ser)
        
        if res:
            print(f"\n{'='*40}")
            print(f"{res}")
            print(f"{'='*40}\n")
        
    except KeyboardInterrupt:
        print("\nInterrompido pelo usuário.")
    finally:
        if ser: ser.close()