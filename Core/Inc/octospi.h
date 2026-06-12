/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    octospi.h
 * @brief   This file contains all the function prototypes for
 *          the octospi.c file
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
#ifndef __OCTOSPI_H__
#define __OCTOSPI_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include "stdbool.h"
/* USER CODE END Includes */

extern OSPI_HandleTypeDef hospi1;

/* USER CODE BEGIN Private defines */
#define OCTOSPI_OK 0
#define OCTOSPI_ERROR -1

#define EXT_RAM_OCTOSPI OCTOSPI1
/* USER CODE END Private defines */

void MX_OCTOSPI1_Init(void);

/* USER CODE BEGIN Prototypes */
int32_t octospi1_init ( void );
int32_t octospi1_deinit ( void );

bool octospi_interface_init_status ( OCTOSPI_TypeDef *instance );
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __OCTOSPI_H__ */

