import serial, csv, time

# === Configuração da serial ===
porta = serial.Serial('/dev/ttyACM0', 115200)
time.sleep(2)

arquivo = '/data/dataset0.csv'

# === PASSAGEM 1: cálculo das médias ===
print("\n=== Passagem 1: cálculo das médias ===")
with open(arquivo, 'r') as f:
    leitor = csv.reader(f)
    next(leitor, None)  # pula cabeçalho se existir
    for linha in leitor:
        x, y = linha
        porta.write(f"{x},{y}\n".encode())
        time.sleep(0.01)

porta.write(b"FIM_MEDIA\n")
print("Primeira passagem concluída. Aguardando MCU...")

# espera o MCU sinalizar que acabou
# resp = porta.readline().decode(errors='ignore').strip()
# print(f"MCU: {resp}")

time.sleep(4)  # espera um pouco antes da próxima passagem

# === PASSAGEM 2: treino com médias ===
print("\n=== Passagem 2: treino com médias ===")
for epoca in range(30):  # mesmo número que EPOCAS_TREINAMENTO
    print(f"\n--- Época {epoca+1} ---")
    with open(arquivo, 'r') as f:
        leitor = csv.reader(f)
        next(leitor, None)
        for linha in leitor:
            x, y = linha
            porta.write(f"{x},{y}\n".encode())
            time.sleep(0.005)
    porta.write(b"FIM_TREINO\n")

print("Segunda passagem concluída. Aguardando resultado...")


print("Simulação completa!")
