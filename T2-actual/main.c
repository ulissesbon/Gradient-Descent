/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  ******************************************************************************
  * @section desc_sec Apresentação Geral
  * Este arquivo contém a função `main()` e as configurações de hardware (HAL) do microcontrolador.
  * Ele atua como o orquestrador do sistema, gerenciando:
  * 1. A inicialização dos periféricos (Clock, GPIO, UART).
  * 2. O protocolo de comunicação serial (Handshake) para receber datasets do PC.
  * 3. A persistência dos dados na memória Flash.
  * 4. O disparo do motor de treinamento (Gradient Descent).
  * 5. O retorno dos resultados processados para o computador.
  *
  * @section protocol_sec Protocolo de Comunicação
  * O sistema utiliza um protocolo "Stop-and-Wait" para garantir a integridade da gravação na Flash:
  * - **READY:** Enviado pela placa quando a Flash foi apagada e está pronta para receber dados.
  * - **ACK:** Enviado pela placa após processar e salvar com sucesso uma linha "x,y".
  * - **ACK_FIM:** Enviado para confirmar o fim da transmissão do dataset.
  * - **RESULTADO:** Tag utilizada para enviar os coeficientes finais (a, b, mse).
  *
  * @section copyright_sec Permissões de Uso
  * Copyright (c) 2025 STMicroelectronics & Autores do Projeto.
  * Código base gerado por STM32CubeMX, lógica de aplicação desenvolvida para fins educacionais.
  *
  * @section io_sec Entrada e Saída
  * - **Entrada (UART):** Strings no formato "float,float\n" ou comandos de controle ("FIM").
  * - **Saída (UART):** Mensagens de status, confirmações (ACK) e resultado formatado.
  *
  * @author Ulisses Gonçalves Bonfim e Raquel Maciel Coelho de Sousa
  * @date 19 de Novembro de 2025
  * @context Trabalho da disciplina de Sistemas Embarcados
  * @platform ARM Cortex-M (STM32 NUCLEO-F030R8).
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

// Importa as funções criadas para interagir com a memória Flash
#include "dataset_flash.h"
// Importa as funções de Regressão Linear
#include "gradient_engine.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/**
 * @brief Handle global para controle da UART2.
 */
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
 * @brief  Lê uma linha de texto da UART2 caractere por caractere.
 * @note   Esta função é bloqueante. Ela retorna apenas quando encontra um caractere 
 * de nova linha ('\\n') ou quando o buffer atinge seu tamanho máximo.
 * Caracteres de retorno de carro ('\\r') são ignorados.
 * * @param  buf: Ponteiro para o buffer onde a string lida será armazenada.
 * @param  maxlen: Tamanho máximo do buffer (incluindo o caractere nulo).
 * @retval int: O número de caracteres lidos e armazenados no buffer.
 */
int read_line(char *buf, int maxlen)
{
  int pos = 0; // Posição atual no buffer

  // Loop de leitura de caractere
  while (pos < maxlen - 1)
  {
    uint8_t c;
    // HAL_UART_Receive é bloqueante aqui (HAL_MAX_DELAY)
    HAL_UART_Receive(&huart2, &c, 1, HAL_MAX_DELAY);

    if (c == '\n') break;     // Fim de linha encontrado
    if (c == '\r') continue;  // Ignora carriage return

    buf[pos++] = c; // Adiciona o caractere válido ao buffer
  }

  buf[pos] = '\0'; // Adiciona o terminador nulo para formar uma string C
  return pos;
}

/**
 * @brief  Envia uma string formatada pela UART2 (wrapper simplificado para printf).
 * @note   Utiliza um buffer interno de 128 bytes. Certifique-se de que a string formatada
 * não exceda este tamanho para evitar estouro de buffer.
 * * @param  fmt: String de formato (ex: "Valor: %d").
 * @param  ...: Lista variável de argumentos para formatar.
 * @retval None
 */
void printTX(const char *fmt, ...) {
  char tx_buff[128];
  va_list args;
  va_start(args, fmt);
  // Formata a string no buffer
  vsnprintf(tx_buff, sizeof(tx_buff), fmt, args);
  va_end(args);
  // Envia via UART (Bloqueante)
  HAL_UART_Transmit(&huart2, (uint8_t*)tx_buff, strlen(tx_buff), HAL_MAX_DELAY);
}
/* USER CODE END 0 */

/**
  * @brief  Ponto de entrada da aplicação (Application Entry Point).
  * @details
  * O fluxo principal consiste em:
  * 1. Inicializar o hardware.
  * 2. Entrar em loop infinito:
  * - Limpar a área de dataset na Flash (exceto se DEBUG estiver definido).
  * - Enviar sinal de prontidão (READY).
  * - Receber dados ponto a ponto via UART (x, y) e salvar na Flash.
  * - Executar o algoritmo de Gradiente Descendente ao receber comando "FIM".
  * - Retornar os coeficientes da regressão (a, b) e o erro (MSE).
  * * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();        // Configura os pinos
  MX_USART2_UART_Init(); // Configura a porta serial para comunicação

  printTX(">> Sistema iniciado. UART OK.\r\n");

  /* USER CODE BEGIN 2 */

  char rx[64];  // Buffer para receber linhas da UART
  float x, y;   // Variáveis temporárias para armazenar os floats lidos
  int index;    // Índice de controle para escrita sequencial na Flash

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    
    index=0;

    // Verifica se o modo DEBUG está ativo para evitar o desgaste da Flash durante testes rápidos
    #ifdef DEBUG
      printTX(">> DEBUG: ERASE da flash desativado.\r\n");
    #else
      // Apaga a página/setor de memória reservada para o dataset
      flash_dataset_erase();
    #endif

    HAL_Delay(500);
    // Avisa o Script Python que o microcontrolador está pronto para receber dados
    printTX("READY\r\n");

    // --- LOOP DE RECEBIMENTO DE DADOS ---
    while (1)
    {
      // Lê uma linha (bloqueante até chegar \n ou encher buffer)
      read_line(rx, sizeof(rx));

      // Verifica se é o comando de encerramento da transmissão
      if (strncmp(rx, "FIM", 3) == 0)
      {
        printTX("ACK_FIM\r\n"); // Confirma saída do loop de recepção
        break;
      }

      // Tenta fazer o parse da string recebida (esperado: "float,float")
      if (sscanf(rx, "%f,%f", &x, &y) == 2)
      {
        // Salva X e Y sequencialmente na memória Flash
        flash_dataset_write_float(index++, x);
        flash_dataset_write_float(index++, y);

        // Envia confirmação (Acknowledge) para o Python enviar o próximo ponto
        printTX("ACK\r\n");
      }
      else
      {
        // Se o formato estiver incorreto ou houver ruído, envia não-reconhecimento
        printTX("NACK\r\n");
      }
    }

    // --- FASE DE TREINAMENTO ---
    printTX(">> MSG: Iniciando treino...\r\n");
    
    // Executa o motor de regressão linear (Gradient Descent)
    gradient_run();
    
    printTX(">> MSG: Treino finalizado.\r\n");

    // --- FASE DE RESULTADO ---
    // Envia resultado final formatado com a TAG para o Python filtrar
    printTX("RESULTADO: a=%.10f b=%.10f mse=%.10f\r\n", gradient_a(), gradient_b(), gradient_mse());
  
  }
  /* USER CODE END WHILE */

  /* USER CODE END 2 */
}

/**
  * @brief System Clock Configuration
  * @note  Configura o clock do sistema para rodar a partir do HSI (High Speed Internal)
  * e utiliza o PLL para atingir a frequência de operação desejada.
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12; // Multiplicador do PLL
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @note  Configura a UART2 para comunicação serial.
  * - BaudRate: 115200
  * - Word Length: 8 Bits
  * - Stop Bits: 1
  * - Parity: None
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @note  Configura os pinos de I/O genéricos.
  * - PA5: LED (LD2) [Saída]
  * - PC13: Botão do Usuário (B1) [Entrada com Interrupção]
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  * where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */