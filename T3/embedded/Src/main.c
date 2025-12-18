/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body (Hibrido: Gravação Flash + Loop RAM)
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

// Importa as funções de memória
#include "dataset_flash.h"
#include "dataset_ram.h"
#include "random_utils.h"

// Importa as funções do algoritmo
#include "gradient_engine.h"

// --- SELEÇÃO DE MODO ---
// Comente esta linha para MODO GRAVAÇÃO (receber da UART e salvar na Flash)
// Descomente esta linha para MODO LOOP (ler da Flash, carregar RAM e treinar)

6#define MODE_FLASH_LOOP

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private define ------------------------------------------------------------*/
// Endereço da Flash onde os dados estão salvos (Página 48 do F030R8)
#define FLASH_USER_START_ADDR   0x0800C000

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

// Array temporário para sorteio (usado apenas no Modo Loop)
// Static para não estourar a pilha (Stack)
static uint16_t g_flash_indices[DATASET_TOTAL_SAMPLES];

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);

/* USER CODE BEGIN 0 */

int read_line(char *buf, int maxlen)
{
    int pos = 0;
    while (pos < maxlen - 1)
    {
        uint8_t c;
        if(HAL_UART_Receive(&huart2, &c, 1, HAL_MAX_DELAY) == HAL_OK)
        {
            if (c == '\n') break;
            if (c == '\r') continue;
            buf[pos++] = c;
        }
    }
    buf[pos] = '\0';
    return pos;
}

void printTX(const char *fmt, ...) {
    char tx_buff[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(tx_buff, sizeof(tx_buff), fmt, args);
    va_end(args);
    HAL_UART_Transmit(&huart2, (uint8_t*)tx_buff, strlen(tx_buff), HAL_MAX_DELAY);
}

// --- Funções Auxiliares para o Modo Loop ---

float read_flash_float(uint32_t address) {
    return *(__IO float*)address;
}

void load_dataset_from_flash_randomly(void) {
    printTX(">> MSG: Carregando Flash -> RAM (Shuffle 55%%)...\r\n");

    dataset_ram_reset();

    // 1. Inicializar array de índices [0, 1, ... 999]
    for (uint16_t i = 0; i < DATASET_TOTAL_SAMPLES; i++) {
        g_flash_indices[i] = i;
    }

    // 2. Semente aleatória baseada no tempo de boot
    random_seed(HAL_GetTick() + 123);

    // 3. Embaralhar indices
    shuffle_indices(g_flash_indices, DATASET_TOTAL_SAMPLES);

    // 4. Copiar os primeiros N pontos embaralhados da Flash para RAM
    int count = 0;
    uint32_t base_addr = FLASH_USER_START_ADDR;

    for (uint16_t i = 0; i < DATASET_RAM_SAMPLES; i++) {
        uint16_t idx = g_flash_indices[i];

        // Endereço na Flash: Base + (Index * 2 floats * 4 bytes)
        uint32_t addr_x = base_addr + (idx * 2 * sizeof(float));
        uint32_t addr_y = addr_x + sizeof(float);

        float x = read_flash_float(addr_x);
        float y = read_flash_float(addr_y);

        dataset_ram_add_point(x, y);
        count++;
    }
    printTX(">> MSG: Carga OK. %d pontos na RAM.\r\n", count);
}
/* USER CODE END 0 */

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART2_UART_Init();

  /* USER CODE BEGIN 2 */

#ifndef MODE_FLASH_LOOP
  /* ===================================================
   * MODO 1: GRAVAÇÃO (Recebe Serial -> Grava Flash)
   * =================================================== */
  printTX(">> [MODO GRAVACAO] Sistema iniciado.\r\n");

  char rx[64];
  float x, y;
  int index = 0;

#ifndef DEBUG
  printTX(">> Apagando Flash...\r\n");
  flash_dataset_erase();
#endif

  // Handshake inicial
  HAL_Delay(500);
  printTX("READY\r\n");

  while (1)
  {
      read_line(rx, sizeof(rx));

      if (strncmp(rx, "FIM", 3) == 0) {
          printTX("ACK_FIM\r\n");
          break;
      }

      if (sscanf(rx, "%f,%f", &x, &y) == 2)
      {
          flash_dataset_write_float(index++, x);
          flash_dataset_write_float(index++, y);
          // IMPORTANTE: ACK para controle de fluxo com Python
          printTX("ACK\r\n");
      }
      else {
          printTX("NACK\r\n");
      }

  }

  printTX(">> Gravacao finalizada. Ative MODE_FLASH_LOOP e regrave.\r\n");
  while (1); // Fim da execução

#else
  /* ===================================================
   * MODO 2: EXECUÇÃO EM LOOP (Lê Flash -> RAM -> Loop)
   * =================================================== */
  printTX(">> [MODO LOOP] Sistema iniciado.\r\n");

  // 1. Carrega dados da Flash, sorteia 55% e põe na RAM
  load_dataset_from_flash_randomly();

  int ciclo = 0;

  // 2. Loop Infinito
  while (1)
  {
      ciclo++;
      HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET); // LED ON

      // Executa gradiente usando dados da RAM
      gradient_run();

      HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET); // LED OFF

      // Reporta resultados
      printTX("RESULTADO: Ciclo=%d a=%.6f b=%.6f mse=%.6f\r\n",
              ciclo,
              gradient_a(),
              gradient_b(),
              gradient_mse());

      // Pequeno delay para visualização/estabilidade
      HAL_Delay(100);
  }

#endif
}

/**
  * @brief System Clock Configuration
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

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

static void MX_USART2_UART_Init(void)
{
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
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
