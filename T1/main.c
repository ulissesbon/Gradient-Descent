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
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>


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



/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* === Parâmetros do algoritmo === */
#define EPOCAS_TREINAMENTO              30    // Quantidade de iterações por amostra
#define TAXA_APRENDIZADO_INCLINACAO  1e-5f    // Learning rate para 'a'
#define TAXA_APRENDIZADO_INTERCEPTO  1e-5f    // Learning rate para 'b'

/* === Variáveis globais do modelo === */
static float gradientA = 0.0f;  // Coeficiente angular (inicialmente 0)
static float gradientB = 0.0f;  // Intercepto (inicialmente 0)
static float MSE = 0.0f;        // Erro médio quadrático final

/* === Variáveis auxiliares === */
static float soma_x = 0.0f, soma_y = 0.0f;   // Acumuladores para cálculo das médias
static int contador = 0;                     // Contador de amostras
static float media_x = 0.0f, media_y = 0.0f; // Médias de x e y

UART_HandleTypeDef huart2; // UART2 (conectada ao ST-Link Virtual COM)

/* === Função auxiliar para enviar texto formatado pela UART === */
void printTX(const char *fmt, ...) {
    char tx_buff[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(tx_buff, sizeof(tx_buff), fmt, args);
    va_end(args);
    HAL_UART_Transmit(&huart2, (uint8_t*)tx_buff, strlen(tx_buff), HAL_MAX_DELAY);
}

/* === Fase 3: cálculo do erro médio quadrático (MSE) ===
 * Essa função é chamada após o treino.
 * Ela lê novamente todos os pares (x, y) enviados via UART
 * e calcula o erro quadrático médio sem armazenar os dados. */
static float calcular_mse_streaming(float a, float b)
{
    char rx_line[64] = {0};
    uint8_t rx_char;
    int idx = 0;
    float x, y;
    float soma_erros_quadrados = 0.0f;
    int n = 0;

    printTX("\r\n>>> Fase MSE (reenvie os dados + FIM_MSE)\r\n");

    while (1)
    {
        // Recebe dados UART, caractere a caractere
        if (HAL_UART_Receive(&huart2, &rx_char, 1, HAL_MAX_DELAY) == HAL_OK)
        {
            // Quando chega um '\n' ou '\r', processa a linha completa
            if (rx_char == '\n' || rx_char == '\r')
            {
                if (idx == 0) continue;  // ignora linhas vazias
                rx_line[idx] = '\0';

                // Sinal de término da fase
                if (strncmp(rx_line, "FIM_MSE", 7) == 0)
                    break;

                // Lê x,y enviados
                if (sscanf(rx_line, "%f,%f", &x, &y) == 2)
                {
                    float y_pred = a * x + b;
                    float erro = y_pred - y;
                    soma_erros_quadrados += erro * erro;
                    n++;
                }
                idx = 0;
            }
            else if (idx < sizeof(rx_line)-1)
                rx_line[idx++] = rx_char; // acumula caracteres até o fim da linha
        }
    }

    return soma_erros_quadrados / (float)n;
}

/* === Enumeração de fases do algoritmo === */
typedef enum {
    FASE_MEDIA,   // Cálculo de médias
    FASE_TREINO   // Treinamento incremental
};

static Fase fase_atual = FASE_MEDIA;  // Começa na fase de médias

/* === Função principal de recepção UART ===
 * É usada tanto na fase de médias quanto na de treino.
 * Lê linhas no formato "x,y" e executa o cálculo apropriado.
 */
void receber_dados_uart(void)
{
    char rx_line[64] = {0};
    uint8_t rx_char;
    int idx = 0;
    float x, y;

    printTX("\r\n>>> Fase atual: %s\r\n", (fase_atual == FASE_MEDIA) ? "MEDIA" : "TREINO");

    while (1)
    {
        if (HAL_UART_Receive(&huart2, &rx_char, 1, HAL_MAX_DELAY) == HAL_OK)
        {
            if (rx_char == '\n' || rx_char == '\r')
            {
                if (idx == 0) continue;
                rx_line[idx] = '\0';

                /* --- Comandos de controle enviados pelo Python --- */

                // Término da fase de médias
                if (strncmp(rx_line, "FIM_MEDIA", 9) == 0)
                {
                    media_x = soma_x / contador;
                    media_y = soma_y / contador;
                    printTX("\r\nMédias calculadas: X=%.6f, Y=%.6f\r\n", media_x, media_y);
                    printTX("MEDIAS_OK\r\n");  // confirma para o Python
                    break;
                }

                // Término da fase de treino
                else if (strncmp(rx_line, "FIM_TREINO", 10) == 0)
                {
                    printTX("\r\nFim da fase de treino.\r\n");
                    break;
                }

                /* --- Leitura dos dados (x,y) --- */
                if (sscanf(rx_line, "%f,%f", &x, &y) == 2)
                {
                    if (fase_atual == FASE_MEDIA)
                    {
                        // Acumula somas para médias
                        soma_x += x;
                        soma_y += y;
                        contador++;
                    }
                    else if (fase_atual == FASE_TREINO)
                    {
                        // Aplica o gradiente descendente incrementalmente
                        for (int epoca = 0; epoca < EPOCAS_TREINAMENTO; epoca++)
                        {
                        	printTX("\nÉpoca %d", epoca);
                            // Centraliza x em torno da média
                            float x_c = x - media_x;

                            // Predição com o modelo atual
                            float y_pred = gradientA * x_c + gradientB;

                            // Calcula erro
                            float erro = y_pred - y;

                            // Calcula derivadas parciais
                            float grad_a = (2.0f * erro * x_c);
                            float grad_b = (2.0f * erro);

                            // Atualiza os parâmetros A e B
                            gradientA -= TAXA_APRENDIZADO_INCLINACAO * grad_a;
                            gradientB -= TAXA_APRENDIZADO_INTERCEPTO * grad_b;
                        }
                    }
                }
                idx = 0; // Reinicia o buffer para próxima linha
            }
            else if (idx < sizeof(rx_line)-1)
                rx_line[idx++] = rx_char;

        }
    }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
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
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  /* === FASE 1: Cálculo das médias === */
  printTX("\r\n=== Fase 1: médias ===\r\n");
  receber_dados_uart(); // recebe dados (x,y) e calcula médias
  soma_x = soma_y = 0.0f;
  contador = 0;

  /* Inicializa parâmetros para treino */
  fase_atual = FASE_TREINO;
  gradientA = 0.0f;
  gradientB = media_y;  // igual ao modelo original

  /* === FASE 2: Treino incremental === */
  printTX("\r\n=== Fase 2: treino ===\r\n");
  receber_dados_uart(); // executa o gradiente descendente

  /* === FASE 3: Cálculo do erro (MSE) === */
  MSE = calcular_mse_streaming(gradientA, gradientB);

  /* === Exibe resultado final === */
  printTX("\r\nFINAL:\r\nA=%.6f\r\nB=%.6f\r\nMSE=%.6f\r\n", gradientA, gradientB, MSE);

  while (1); // loop infinito

  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
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
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
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

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
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
	