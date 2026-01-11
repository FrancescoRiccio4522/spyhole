/* Includes */
#include "main.h"
#include "string.h"

/* Defines */
#define SERVO_STOP   1400
#define SERVO_OPEN   1000
#define SERVO_CLOSE  1000
#define BUFFER_SIZE 64
#define MAX_FACE_ATTEMPTS 3
#define LOCKOUT_TIME 10000 // 10 secondi
#define SERVO_OPEN_TIME 200
#define LED_FAIL_TIME 2000

/* Typedef */
typedef enum {
    WAIT_ACCESS_COMMAND,
    WAIT_FACE_RESPONSE,
    WAIT_PIN,
    LOCKOUT
} AccessState;

/* Private variables */
TIM_HandleTypeDef htim1;
UART_HandleTypeDef huart2;   // Bluetooth
UART_HandleTypeDef huart3;   // ESP32-CAM

AccessState access_state = WAIT_ACCESS_COMMAND;

/* Bluetooth variables */
char bt_char;
char bt_buffer[BUFFER_SIZE];
uint8_t bt_index = 0;

/* CAM variables */
uint8_t cam_byte;

/* State management */
int face_attempts = 0;
int message_sent = 0;
uint32_t lockout_timer = 0;
uint32_t action_timer = 0;
uint8_t action_state = 0; // 0 idle, 1 success, 2 failure

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART3_UART_Init(void);
void Servo_Move(uint16_t);
void LED_Red(void);
void LED_Green(void);
void LED_Blue(void);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);

/* Functions */
void Servo_Move(uint16_t pulse_val) {
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pulse_val);
}

void LED_Red(void) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
}

void LED_Green(void) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
}

void LED_Blue(void) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);
}

int strcasecmp_custom(const char *a, const char *b) {
    while (*a && *b) {
        char ca = (*a >= 'A' && *a <= 'Z') ? *a + 32 : *a;
        char cb = (*b >= 'A' && *b <= 'Z') ? *b + 32 : *b;
        if (ca != cb) return ca - cb;
        a++; b++;
    }
    return *a - *b;
}

/* UART callback */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART2) // Bluetooth
    {
        if(access_state != LOCKOUT)
        {
            if(bt_char == '\r' || bt_char == '\n')
            {
                bt_buffer[bt_index] = 0; /*Aggiunge un terminatore nullo (\0) alla fine della stringa nel buffer.
                						   Questo trasforma il contenuto di bt_buffer in una stringa C corretta,
                						   pronta per funzioni come strcmp o strcasecmp.*/

                bt_index = 0;			/*Resetta l’indice del buffer per il prossimo comando.
                							Significa che il buffer è pronto a ricevere nuovi caratteri dal Bluetooth
                							 senza sovrascrivere i dati precedenti. */

                if(access_state == WAIT_ACCESS_COMMAND)
                {
                    if(strcasecmp_custom(bt_buffer, "Access") == 0)
                    {
                        LED_Blue();
                        uint8_t shoot_cmd[] = "2\n";
                        HAL_UART_Transmit(&huart3, shoot_cmd, sizeof(shoot_cmd)-1, 50);
                        access_state = WAIT_FACE_RESPONSE;

                        const char msg[] = "TRYING FACE RECOGNITION...\r\n";
                        HAL_UART_Transmit(&huart2, (uint8_t*)msg, sizeof(msg)-1, 100);
                        message_sent = 0;
                    }
                    else if(message_sent == 0)
                    {
                        const char msg[] = "WRITE 'ACCESS' TO START FACIAL RECOGNIZE\r\n";
                        HAL_UART_Transmit(&huart2, (uint8_t*)msg, sizeof(msg)-1, 100);
                        message_sent = 1;
                    }
                }
                else if(access_state == WAIT_PIN)
                {
                    if(strcmp(bt_buffer, "1234") == 0)
                    {
                        LED_Green();
                        Servo_Move(SERVO_OPEN);
                        action_timer = HAL_GetTick(); // memorizza il momento in cui il servo inizia a muoversi
                        action_state = 1;

                        const char msg[] = "ACCESS GRANTED\r\n";
                        HAL_UART_Transmit(&huart2, (uint8_t*)msg, sizeof(msg)-1, 100);

                        face_attempts = 0;
                        access_state = WAIT_ACCESS_COMMAND;
                        message_sent = 0;
                    }
                    else
                    {
                        LED_Red();
                        const char msg[] = "ACCESS DENIED. WAIT 10 SECONDS...\r\n";
                        HAL_UART_Transmit(&huart2, (uint8_t*)msg, sizeof(msg)-1, 100);

                        face_attempts = 0;
                        access_state = LOCKOUT;
                        lockout_timer = HAL_GetTick(); // memorizza il momento in cui è iniziato il lockout
                    }
                }
            }
            else
            {
                bt_buffer[bt_index++] = bt_char;
                if(bt_index >= sizeof(bt_buffer)) bt_index = 0;
            }
        }
        HAL_UART_Receive_IT(&huart2, &bt_char, 1); // riattiva sempre
    }

    else if(huart->Instance == USART3) // ESP32-CAM
    {
        if(access_state == WAIT_FACE_RESPONSE)
        {
            if(cam_byte == 'Y')
            {
                LED_Green();
                Servo_Move(SERVO_OPEN);
                action_timer = HAL_GetTick(); // memorizza il momento in cui il servo inizia a muoversi
                action_state = 1;

                face_attempts = 0;
                access_state = WAIT_ACCESS_COMMAND;

                const char msg[] = "ACCESS GRANTED\r\n";
                HAL_UART_Transmit(&huart2, (uint8_t*)msg, sizeof(msg)-1, 100);
                message_sent = 0;
            }
            else if(cam_byte == 'N')
            {
                face_attempts++;

                if(face_attempts < MAX_FACE_ATTEMPTS)
                {
                    const char msg[] = "FACE NOT RECOGNIZED. TRY AGAIN\r\n";
                    HAL_UART_Transmit(&huart2, (uint8_t*)msg, sizeof(msg)-1, 100);

                    LED_Red();
                    action_timer = HAL_GetTick(); // memorizza il momento in cui avviene il fallimento di accesso
                    action_state = 2;

                    access_state = WAIT_ACCESS_COMMAND;
                    message_sent = 0;
                }
                else
                {
                    access_state = WAIT_PIN;
                    face_attempts = 0;

                    const char msg[] = "MAX ATTEMPTS REACHED. INSERT PIN\r\n";
                    HAL_UART_Transmit(&huart2, (uint8_t*)msg, sizeof(msg)-1, 100);

                    LED_Red();
                    message_sent = 0;
                }
            }
        }
        HAL_UART_Receive_IT(&huart3, &cam_byte, 1); // riattiva sempre
    }
}

/* Main */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM1_Init();
    MX_USART2_UART_Init();
    MX_USART3_UART_Init();

    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    Servo_Move(SERVO_STOP);
    LED_Blue();

    HAL_UART_Receive_IT(&huart2, &bt_char, 1);
    HAL_UART_Receive_IT(&huart3, &cam_byte, 1);

    const char msg[] = "WRITE 'ACCESS' TO START FACIAL RECOGNIZE\r\n";
    HAL_UART_Transmit(&huart2, (uint8_t*)msg, sizeof(msg)-1, 100);

    while(1)
    {
        // gestione servo/LED temporizzati
    	if(action_state == 1 && HAL_GetTick() - action_timer >= SERVO_OPEN_TIME)
    	{
    	    Servo_Move(SERVO_CLOSE);   // apertura inversa per simulare chiusura
    	    HAL_Delay(200);             // breve pausa per completare il movimento
    	    Servo_Move(SERVO_STOP);    // ferma il servo
    	    LED_Blue();
    	    action_state = 0;
    	}


        else if(action_state == 2 && HAL_GetTick() - action_timer >= LED_FAIL_TIME)
        {
            LED_Blue();
            action_state = 0;
        }

        // gestione lockout
        if(access_state == LOCKOUT)
        {
            if(HAL_GetTick() - lockout_timer >= LOCKOUT_TIME)
            {
                access_state = WAIT_ACCESS_COMMAND;
                message_sent = 0;

                LED_Blue();

                const char msg2[] = "WRITE 'ACCESS' TO START FACIAL RECOGNIZE\r\n";
                HAL_UART_Transmit(&huart2, (uint8_t*)msg2, sizeof(msg2)-1, 100);
            }
        }
    }
}


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2|RCC_PERIPHCLK_USART3
                              |RCC_PERIPHCLK_TIM1;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  PeriphClkInit.Usart3ClockSelection = RCC_USART3CLKSOURCE_SYSCLK;
  PeriphClkInit.Tim1ClockSelection = RCC_TIM1CLK_HCLK;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 72-1;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 20000-1;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 1500;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.BreakFilter = 0;
  sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;
  sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
  sBreakDeadTimeConfig.Break2Filter = 0;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

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
  huart2.Init.BaudRate = 9600;
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
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

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
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7, GPIO_PIN_RESET);

  /*Configure GPIO pins : PA5 PA6 PA7 */
  GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

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
