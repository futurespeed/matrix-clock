/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
//GPIOB
#define R1_Pin GPIO_PIN_0
#define G1_Pin GPIO_PIN_1
#define B1_Pin GPIO_PIN_6
#define LAT_Pin GPIO_PIN_10
#define OE_Pin GPIO_PIN_11
#define ADDR_A_Pin GPIO_PIN_12
#define ADDR_B_Pin GPIO_PIN_13
#define ADDR_C_Pin GPIO_PIN_14
#define ADDR_D_Pin GPIO_PIN_15
#define ADDR_E_Pin GPIO_PIN_7
#define B2_Pin GPIO_PIN_3
#define G2_Pin GPIO_PIN_4
#define R2_Pin GPIO_PIN_5
#define CLK_Pin GPIO_PIN_8

#define UART_BUFF_SIZE 1024

/* USER CODE BEGIN Private defines */
typedef enum
{
    MODE_WAIT = 0,
    MODE_CLOCK,
    MODE_ANIMATION,
    MODE_MAX
} clock_mode_t;

typedef enum
{
    CMD_MODE = 1,
    CMD_BRIGHTNESS = 2,
    CMD_TIME = 10,
    CMD_LUNAR = 11,
    CMD_TEMPERATURE = 12,
    CMD_WEATHER = 13
} uart_cmd_t;

typedef enum
{
    WEATHER_NONE = 0,
    WEATHER_SUN,
    WEATHER_RAIN,
    WEATHER_CLOUD,
    WEATHER_SNOW
} weather_t;

typedef struct
{
	clock_mode_t mode;
	int brightness;
	int temperature;
	weather_t weather;
	uint64_t sync_time;
	uint32_t lunar_year;
	uint32_t lunar_month;
	uint32_t lunar_day;
} clock_info_t;

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
