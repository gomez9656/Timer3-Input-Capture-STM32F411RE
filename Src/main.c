/*
 * main.c
 *
 *  Created on: 23/01/2021
 *      Author: PC
 */

#include "stm32f4xx.h"
#include "main.h"
#include <string.h>

/*
 * Functions prototypes
 */
void SystemCoreClockConfig(uint8_t clock_freq);
void Error_handler(void);
void GPIO_Init(void);
void TIMER3_Init(void);
void UART2_Init(void);
void LSE_Configuration(void);
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim);

TIM_HandleTypeDef htimer3;
UART_HandleTypeDef huart2; //Handle of UART 2
uint32_t input_capture[2] = {0};
uint8_t count = 1;
uint8_t is_capture_done = FALSE;


int main(){

	uint32_t capture_difference = 0;
	double timer3_count_freq = 0;
	double timer3_count_resolution = 0;
	double user_signal_time_period = 0;
	double user_signal_freq = 0;
	char user_msg[100];

	/* Basic initialization  */
	HAL_Init();
	SystemCoreClockConfig(SYS_CLOCK_FREQ_50_MHZ);
	GPIO_Init();

	UART2_Init();

	TIMER3_Init();

	LSE_Configuration();

	HAL_TIM_IC_Start_IT(&htimer3,TIM_CHANNEL_1);

	while(1){

		if(is_capture_done){

			if(input_capture[1] > input_capture[0])
				capture_difference = input_capture[1] - input_capture[0];
			else
				capture_difference = (0xFFFFFFFF - input_capture[0]) + input_capture[1];

		timer3_count_freq = (HAL_RCC_GetPCLK1Freq() * 2) / (htimer3.Init.Prescaler + 1);
		timer3_count_resolution = 1 / timer3_count_freq;
		user_signal_time_period = capture_difference * timer3_count_resolution;
		user_signal_freq = 1 / user_signal_time_period;

		sprintf(user_msg, "Frequency of the signal applied = %0.2f\r\n", user_signal_freq);
		HAL_UART_Transmit(&huart2, (uint8_t*)user_msg, strlen(user_msg), HAL_MAX_DELAY);

		is_capture_done = FALSE;
		}
	}
	return 0;
}


void TIMER3_Init(void){

	TIM_IC_InitTypeDef timer3IC_Config;

	//High level initialization of the TIMER 3
	htimer3.Instance = TIM3;
	htimer3.Init.CounterMode = TIM_COUNTERMODE_UP;
	htimer3.Init.Period = 0xFFFFFFFF;
	htimer3.Init.Prescaler = 1;

	if( HAL_TIM_IC_Init(&htimer3) != HAL_OK){
		Error_handler();
	}

	//Low level initialization for the Input Capture
	timer3IC_Config.ICFilter = 0;
	timer3IC_Config.ICPolarity = TIM_ICPOLARITY_RISING;
	timer3IC_Config.ICPrescaler = TIM_ICPSC_DIV1;
	timer3IC_Config.ICSelection = TIM_ICSELECTION_DIRECTTI;

	if( HAL_TIM_IC_ConfigChannel(&htimer3, &timer3IC_Config, TIM_CHANNEL_1) != HAL_OK){
		Error_handler();
	}
}

void LSE_Configuration(void){

#if 0
	//Activate the LSE crystal oscillator
	RCC_OscInitTypeDef Osc_Init;
	Osc_Init.OscillatorType = RCC_OSCILLATORTYPE_LSE;
	Osc_Init.LSEState = RCC_LSE_ON;

	if (HAL_RCC_OscConfig(&Osc_Init) != HAL_OK){
		Error_handler();
	}
#endif

	//Use MCO1 as the output of the LSE. This can be done in the PA8 as alternate function
	HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_LSE, RCC_MCODIV_1);

}



void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim){

	if(!is_capture_done){
	//Save the values when the signal rises
		if ( count == 1){

			input_capture[0] = __HAL_TIM_GET_COMPARE(htim, TIM_CHANNEL_1);
			count++;
		}
		else if( count == 2){

			input_capture[1] = __HAL_TIM_GET_COMPARE(htim, TIM_CHANNEL_1);
			count = 1;
			is_capture_done = TRUE;
		}
	}
}

/*
 * You can use it when you need an specific clock
 * By default will be internal clock
 */
void SystemCoreClockConfig(uint8_t clock_freq){

	RCC_OscInitTypeDef Osc_Init;
	RCC_ClkInitTypeDef Clock_Init;

	Osc_Init.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_LSE;
	Osc_Init.HSIState = RCC_HSI_ON;
	Osc_Init.LSEState = RCC_LSE_ON;
	Osc_Init.HSICalibrationValue = 16;
	Osc_Init.PLL.PLLState = RCC_PLL_ON;
	Osc_Init.PLL.PLLSource = RCC_PLLSOURCE_HSI;

	switch(clock_freq){
	  case SYS_CLOCK_FREQ_50_MHZ:
			  Osc_Init.PLL.PLLM = 8;
			  Osc_Init.PLL.PLLN = 50;
			  Osc_Init.PLL.PLLP = RCC_PLLP_DIV2;
			  Osc_Init.PLL.PLLQ = 2;
			  Clock_Init.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
		                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
			  Clock_Init.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
			  Clock_Init.AHBCLKDivider = RCC_SYSCLK_DIV1;
			  Clock_Init.APB1CLKDivider = RCC_HCLK_DIV2;
			  Clock_Init.APB2CLKDivider = RCC_HCLK_DIV1;
		     break;

		  case SYS_CLOCK_FREQ_84_MHZ:
			  Osc_Init.PLL.PLLM = 8;
			  Osc_Init.PLL.PLLN = 84;
			  Osc_Init.PLL.PLLP = RCC_PLLP_DIV2;
			  Osc_Init.PLL.PLLQ = 2;
			  Clock_Init.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
		                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
			  Clock_Init.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
			  Clock_Init.AHBCLKDivider = RCC_SYSCLK_DIV1;
			  Clock_Init.APB1CLKDivider = RCC_HCLK_DIV2;
			  Clock_Init.APB2CLKDivider = RCC_HCLK_DIV1;
		     break;

		  case SYS_CLOCK_FREQ_120_MHZ:
			  Osc_Init.PLL.PLLM = 8;
			  Osc_Init.PLL.PLLN = 120;
			  Osc_Init.PLL.PLLP = RCC_PLLP_DIV2;
			  Osc_Init.PLL.PLLQ = 2;
			  Clock_Init.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
		                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
			  Clock_Init.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
			  Clock_Init.AHBCLKDivider = RCC_SYSCLK_DIV1;
			  Clock_Init.APB1CLKDivider = RCC_HCLK_DIV4;
			  Clock_Init.APB2CLKDivider = RCC_HCLK_DIV2;
		     break;

		  default:
		   return ;
		 }

			if (HAL_RCC_OscConfig(&Osc_Init) != HAL_OK)
		{
				Error_handler();
		}



		if (HAL_RCC_ClockConfig(&Clock_Init, FLASH_LATENCY_2) != HAL_OK)
		{
			Error_handler();
		}


		/*Configure the systick timer interrupt frequency (for every 1 ms) */
		uint32_t hclk_freq = HAL_RCC_GetHCLKFreq();
		HAL_SYSTICK_Config(hclk_freq/1000);

		/**Configure the Systick
		*/
		HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

		/* SysTick_IRQn interrupt configuration */
		HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

void UART2_Init(void){

	//1. Linking handle struct to base address
	huart2.Instance = USART2;

	//2. Init the handle variable
	huart2.Init.BaudRate = 115200;
	huart2.Init.WordLength = UART_WORDLENGTH_8B;
	huart2.Init.StopBits = UART_STOPBITS_1;
	huart2.Init.Parity = UART_PARITY_NONE;
	huart2.Init.Mode = UART_MODE_TX_RX;
	huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;

	//3. Use the API to initialize
	if(HAL_UART_Init(&huart2) != HAL_OK){

		//There is a problem
		Error_handler();
	}
}


void GPIO_Init(void){

	__HAL_RCC_GPIOA_CLK_ENABLE();
	GPIO_InitTypeDef ledgpio;
	ledgpio.Pin = GPIO_PIN_5;
	ledgpio.Mode = GPIO_MODE_OUTPUT_PP;
	ledgpio.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &ledgpio);
}

void Error_handler(void){
	while(1);
}
