import time
import serial
import pandas as pd
import sys

# --- CONFIGURAÇÃO ---
PORTA = "/dev/ttyACM0" 
BAUDRATE = 115200

try:
    # Timeout curto para handshake
    ser = serial.Serial(PORTA, BAUDRATE, timeout=1)
except serial.SerialException:
    print(f"ERRO: Porta {PORTA} ocupada ou não encontrada.")
    print("DICA: Execute 'sudo fuser -k /dev/ttyACM0' se estiver travada.")
    sys.exit()

# Reinicia a placa via DTR (opcional, ajuda a sincronizar o início)
ser.dtr = False
time.sleep(0.5)
ser.dtr = True
time.sleep(2) 
ser.reset_input_buffer()

print(f"Conectado em {PORTA}. Iniciando protocolo Sincronizado...\n")

def wait_for_ready():
    """
    Função que bloqueia o Python até a placa terminar de apagar a flash
    e enviar 'READY'.
    """
    print("Aguardando placa ficar pronta (Apagando Flash)...")
    ser.timeout = None # Bloqueante infinito até receber algo
    while True:
        line = ser.readline().decode(errors='ignore').strip()
        # Se a placa reiniciou ou acabou de apagar a flash, ela manda READY
        if line == "READY":
            print(">> Placa PRONTA.")
            break
        elif ">>" in line:
             # Mostra mensagens de debug do boot se houver
             print(f"[BOOT] {line}")
    ser.timeout = 1 # Retorna timeout para 1s para o envio de dados

# Arquivo de saída
with open("resultados_finais.txt", "w") as f_out:
    
    for file_number in range(4): 
        
        # --- PASSO CRUCIAL: ESPERAR A PLACA ESTAR PRONTA ---
        # Isso evita enviar dados enquanto a placa está apagando a flash
        wait_for_ready()
        # ---------------------------------------------------

        print(f"--- Enviando Dataset {file_number} ---")
        try:
            df = pd.read_csv(f"data/dataset{file_number}.csv")
        except FileNotFoundError:
            print(f"Arquivo dataset{file_number}.csv não encontrado.")
            continue
        
        start_time = time.time()
        total_linhas = len(df)

        # === ENVIO COM HANDSHAKE ===
        for i in range(total_linhas):
            dado_x = df.iloc[i,0]
            dado_y = df.iloc[i,1]
            mensagem = f"{dado_x},{dado_y}\n"
            
            enviado = False
            while not enviado:
                ser.write(mensagem.encode())
                
                try:
                    resp = ser.readline().decode(errors='ignore').strip()
                    if resp == "ACK":
                        enviado = True
                    elif resp == "NACK":
                        print(f"Reenviando linha {i}...")
                    # Se receber READY aqui, é pq a placa resetou, o script pode se perder,
                    # mas geralmente cairá no timeout e tentará reenviar.
                except serial.SerialTimeoutException:
                    pass # Tenta enviar de novo
        
        print(f"Upload {file_number} concluído. Finalizando lote...")

        # === ENVIA FIM ===
        while True:
            ser.write(b"FIM\n")
            resp = ser.readline().decode(errors='ignore').strip()
            if "ACK_FIM" in resp:
                break
        
        print("Aguardando Treinamento...")

        # === AGUARDANDO RESULTADO ===
        ser.timeout = 60 # Aumenta timeout para esperar o treino
        while True:
            line = ser.readline().decode(errors='ignore').strip()
            if not line: continue
            
            if ">>" in line: # Prints de status da placa
                print(f"[PLACA] {line}")

            if line.startswith("RESULTADO:"):
                dados = line.replace("RESULTADO:", "").strip()
                texto_final = f"(Emb) Dataset {file_number}: {dados}"
                
                print("\n" + "*"*40)
                print(texto_final)
                print("*"*40 + "\n")
                
                f_out.write(texto_final + "\n")
                f_out.flush()
                break
        
        ser.timeout = 1 # Restaura timeout curto

print("Processo finalizado com sucesso.")
ser.close()