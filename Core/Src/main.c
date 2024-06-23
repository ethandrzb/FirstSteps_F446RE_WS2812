/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "WS2812.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

// NOTE: It is not recommended to enable more than one example

//#define EXAMPLE_1
//#define EXAMPLE_2
//#define EXAMPLE_3
//#define SINGLE_COMET_EFFECT
//#define MULTI_COMET_EFFECT
#define MANUAL_MULTI_COMET_EFFECT
//#define EXAMPLE_SIMPLE_METER_EFFECT
//#define EXAMPLE_MIRRORED_METER_EFFECT
//#define EXAMPLE_SIMPLE_GATED_STROBE
//#define EXAMPLE_VARIABLE_GATED_STROBE

// Should not be enabled if either EXAMPLE_SIMPLE_METER_EFFECT or EXAMPLE_MIRRORED_METER_EFFECT is enabled
#define ENABLE_POTS_TO_BACKGROUND_COLOR
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

SPI_HandleTypeDef hspi3;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
uint8_t iterations = 0;

uint16_t rawADCData[3];

#if defined(EXAMPLE_SIMPLE_METER_EFFECT) || defined(EXAMPLE_MIRRORED_METER_EFFECT)
uint8_t meterLevels[] = {0, 0, 0};
#endif

#ifdef EXAMPLE_VARIABLE_GATED_STROBE
uint32_t strobePeriodOffsetTicks = 0;
uint32_t strobeActiveTicks = 0;
#endif
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SPI3_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
//	// Disable button interrupts
//	HAL_NVIC_DisableIRQ(EXTI4_IRQn);
//	HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
//	HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
//
//	// Start debounce timer
//	HAL_TIM_Base_Start_IT(&htim6);

#ifdef MANUAL_MULTI_COMET_EFFECT
	color color;

	switch(GPIO_Pin)
	{
		case BTN1_Pin:
			color.red = 96;
			color.green = 0;
			color.blue = 0;

			WS2812_AddComet(color, 2);
			break;
		case BTN2_Pin:
			color.red = 0;
			color.green = 96;
			color.blue = 0;

			WS2812_AddComet(color, 2);
			break;
		case BTN3_Pin:
			color.red = 0;
			color.green = 0;
			color.blue = 96;

			WS2812_AddComet(color, 2);
			break;
		case BTN4_Pin:
			color.red = 96;
			color.green = 96;
			color.blue = 96;

			WS2812_AddComet(color, 2);
			break;
	}
#endif

#if defined(EXAMPLE_SIMPLE_GATED_STROBE) || defined(EXAMPLE_VARIABLE_GATED_STROBE)
	if(GPIO_Pin == BTN1_Pin)
	{
		if(!HAL_GPIO_ReadPin(BTN1_GPIO_Port, BTN1_Pin))
		{
			HAL_TIM_Base_Start(&htim2);
		}
		else
		{
			HAL_TIM_Base_Stop(&htim2);
			htim2.Instance->CNT = 0;
		}
	}
#endif
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
#ifdef ENABLE_POTS_TO_BACKGROUND_COLOR
	// TODO: Add threshold or other system to prevent noise picked up by the ADC from turning on the LEDs
	// Masking out the LSBs works, but significantly reduces the resolution of the brightness control

	// >> 3 to scale to comfortable range
//	WS2812_SetBackgroundColor(rawADCData[0] >> 3, rawADCData[1] >> 3, rawADCData[2] >> 3);

	color rgb = WS2812_HSVToRGB((uint16_t)(((float)rawADCData[0] * 360) / 256.0f), ((float)rawADCData[1]) / 255.0f, ((float)rawADCData[2]) / 255.0f);

	WS2812_SetBackgroundColor(rgb.red, rgb.green, rgb.blue);
#endif
#if defined(EXAMPLE_SIMPLE_METER_EFFECT) || defined(EXAMPLE_MIRRORED_METER_EFFECT)
	meterLevels[0] = rawADCData[0] >> 1;
	meterLevels[1] = rawADCData[1] >> 1;
	meterLevels[2] = rawADCData[2] >> 1;
#endif

#ifdef EXAMPLE_VARIABLE_GATED_STROBE
	strobePeriodOffsetTicks = 80000 + (rawADCData[0] << 9);
	htim2.Instance->ARR = strobePeriodOffsetTicks;

	// Changing the auto-reload register (ARR) with an input clock this fast occasionally allows the count (CNT)
		// register to exceed the ARR and preventing it from resetting at that value
	// As a result, we have to add our own reset condition to prevent this from happening.
	if(htim2.Instance->CNT > htim2.Instance->ARR)
	{
		htim2.Instance->CNT = 0;
	}

	// Map ADC channel 1 to strobe active period
	// ADC is configured in 8-bit mode, so we multiply by the channel value divide by the max value (256)
		// to scale the measured value relative to the period
	// Bit shifting is faster than dividing, so we right shift by 8 instead of dividing by 256
	strobeActiveTicks = (strobePeriodOffsetTicks * rawADCData[1]) >> 8;

	// Reverse active and inactive periods
	strobeActiveTicks = strobePeriodOffsetTicks - strobeActiveTicks;
#endif
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
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_SPI3_Init();
  MX_ADC1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  // Initialize all LEDs to off
  WS2812_ClearLEDs();
  WS2812_SetBackgroundColor(0, 0, 0);
  WS2812_SendAll();

#if defined(MULTI_COMET_EFFECT) || defined(MANUAL_MULTI_COMET_EFFECT)
  WS2812_InitMultiCometEffect();
#endif

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
#ifdef EXAMPLE_1
	  // Color values can be between 0 and 255, but 255 is really bright!
	for(int i = 0; i < NUM_LEDS; i++)
	{
		WS2812_SetLED(i, 8, 0, 0);
	}
	WS2812_SendAll();
	HAL_Delay(1000);

	for(int i = 0; i < NUM_LEDS; i++)
	{
		WS2812_SetLED(i, 0, 8, 0);
	}
	WS2812_SendAll();
	HAL_Delay(1000);

	for(int i = 0; i < NUM_LEDS; i++)
	{
		WS2812_SetLED(i, 0, 0, 8);
	}
	WS2812_SendAll();
	HAL_Delay(1000);
#endif
#ifdef EXAMPLE_2
	for(int i = 0; i < 128; i++)
	{
		for(int led = 0; led < NUM_LEDS; led++)
		{
			WS2812_SetLED(led, i, i, i);
		}
		WS2812_SendAll();
		HAL_Delay(10);
	}
	for(int i = 128; i >= 0; i--)
	{
		for(int led = 0; led < NUM_LEDS; led++)
		{
			WS2812_SetLED(led, i, i, i);
		}
		WS2812_SendAll();
		HAL_Delay(10);
	}
#endif
#ifdef EXAMPLE_3
	WS2812_SetLED(0, 64, 64, 64);
	WS2812_SendAll();

	uint8_t shiftAmount = 1;
	for(int j = 0; j < NUM_LEDS / shiftAmount; j++)
	{
		if(j % 10 == 0)
		{
			WS2812_SetLED(0, 64, 64, 64);
		}

		WS2812_ShiftLEDs(shiftAmount);
		WS2812_SendAll();
		HAL_Delay(12);
	}
#endif
#ifdef SINGLE_COMET_EFFECT
	WS2812_CometEffect();
	WS2812_SendAll();
	HAL_Delay(10);
#endif
#ifdef MULTI_COMET_EFFECT
	if(iterations > 50)
	{
		iterations = 0;

		color color;
		color.red = 64;
		color.green = 64;
		color.blue = 64;

		WS2812_AddComet(color, 2);
	}
	WS2812_MultiCometEffect();
	WS2812_SendAll();
	iterations++;

//	HAL_Delay(50);
#endif
#ifdef MANUAL_MULTI_COMET_EFFECT
	WS2812_MultiCometEffect();
	WS2812_SendAll();
#endif

#ifdef EXAMPLE_SIMPLE_METER_EFFECT
	WS2812_ClearLEDs();

	color meterColor = {.red = 32, .green = 32, .blue = 0};
	WS2812_SimpleMeterEffect(meterColor, meterLevels[0], true);
	meterColor.red = 0;
	meterColor.green = 32;
	meterColor.blue = 32;
	WS2812_SimpleMeterEffect(meterColor, meterLevels[1], true);
	meterColor.red = 32;
	meterColor.green = 0;
	meterColor.blue = 32;
	WS2812_SimpleMeterEffect(meterColor, meterLevels[2], true);
	WS2812_SendAll();
#endif

#ifdef EXAMPLE_MIRRORED_METER_EFFECT
	WS2812_ClearLEDs();
	color meterColor = {.red = 32, .green = 32, .blue = 0};
	WS2812_MirroredMeterEffect(meterColor, meterLevels[0], false);
	meterColor.red = 0;
	meterColor.green = 32;
	meterColor.blue = 32;
	WS2812_MirroredMeterEffect(meterColor, meterLevels[1], true);
	meterColor.red = 32;
	meterColor.green = 0;
	meterColor.blue = 32;
	WS2812_MirroredMeterEffect(meterColor, meterLevels[2], true);
	WS2812_SendAll();
#endif

#ifdef EXAMPLE_SIMPLE_GATED_STROBE
	if(htim2.Instance->CNT > 40000)
	{
		WS2812_SetAllLEDs(32, 32, 32);
	}
	else
	{
		WS2812_SetAllLEDs(0, 0, 0);
	}

	WS2812_SendAll();
#endif

#ifdef EXAMPLE_VARIABLE_GATED_STROBE
	if(htim2.Instance->CNT > strobeActiveTicks)
	{
		WS2812_SetAllLEDs(32, 32, 32);
	}
	else
	{
		WS2812_SetAllLEDs(0, 0, 0);
	}

	WS2812_SendAll();
#endif
	HAL_ADC_Start_DMA(&hadc1, (uint32_t *) rawADCData, 3);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 160;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_8B;
  hadc1.Init.ScanConvMode = ENABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 3;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = 2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = 3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief SPI3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI3_Init(void)
{

  /* USER CODE BEGIN SPI3_Init 0 */

  /* USER CODE END SPI3_Init 0 */

  /* USER CODE BEGIN SPI3_Init 1 */

  /* USER CODE END SPI3_Init 1 */
  /* SPI3 parameter configuration*/
  hspi3.Instance = SPI3;
  hspi3.Init.Mode = SPI_MODE_MASTER;
  hspi3.Init.Direction = SPI_DIRECTION_1LINE;
  hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi3.Init.NSS = SPI_NSS_SOFT;
  hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi3.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI3_Init 2 */

  /* USER CODE END SPI3_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 80-1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 80000-1;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

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
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

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

  /*Configure GPIO pin : BTN2_Pin */
  GPIO_InitStruct.Pin = BTN2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(BTN2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : BTN1_Pin */
  GPIO_InitStruct.Pin = BTN1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(BTN1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : BTN3_Pin BTN4_Pin */
  GPIO_InitStruct.Pin = BTN3_Pin|BTN4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

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
