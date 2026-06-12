/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    gpio.h
 * @brief   This file contains all the function prototypes for
 *          the gpio.c file
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __GPIO_H__
#define __GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include "stdbool.h"
/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */
typedef struct
{
  GPIO_TypeDef *port;
  uint16_t pin;
} gpio_pin_struct;
/* USER CODE END Private defines */

void MX_GPIO_Init(void);

/* USER CODE BEGIN Prototypes */
void configure_iridium_onoff_pin ( bool v3f );
void toggle_rtc_timestamp1_pin ( void );

static inline GPIO_PinState gpio_read_pin ( gpio_pin_struct pin )
{
  return HAL_GPIO_ReadPin (pin.port, pin.pin);
}

static inline void gpio_write_pin ( gpio_pin_struct pin, GPIO_PinState state )
{
  HAL_GPIO_WritePin (pin.port, pin.pin, state);
}
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /*__ GPIO_H__ */

