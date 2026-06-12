/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    usart.h
 * @brief   This file contains all the function prototypes for
 *          the usart.c file
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
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include "stdbool.h"

#define UART_OK 0
#define UART_ERR -1

/* USER CODE END Includes */

extern UART_HandleTypeDef hlpuart1;

extern UART_HandleTypeDef huart4;

extern UART_HandleTypeDef huart1;

extern UART_HandleTypeDef huart2;

extern UART_HandleTypeDef huart3;

/* USER CODE BEGIN Private defines */
extern DMA_HandleTypeDef handle_LPDMA1_Channel0;
extern DMA_HandleTypeDef handle_LPDMA1_Channel1;
extern DMA_HandleTypeDef handle_GPDMA1_Channel0;
extern DMA_HandleTypeDef handle_GPDMA1_Channel1;
extern DMA_HandleTypeDef handle_GPDMA1_Channel2;
extern DMA_HandleTypeDef handle_GPDMA1_Channel3;
extern DMA_HandleTypeDef handle_GPDMA1_Channel4;
extern DMA_HandleTypeDef handle_GPDMA1_Channel5;
extern DMA_HandleTypeDef handle_GPDMA1_Channel6;
extern DMA_HandleTypeDef handle_GPDMA1_Channel7;
extern DMA_HandleTypeDef handle_GPDMA1_Channel8;
extern DMA_HandleTypeDef handle_GPDMA1_Channel9;

#define GNSS_UART LPUART1
#define CT_UART USART1
#define EXPANSION_UART USART2
#define LOGGER_UART USART3
#define IRIDIUM_UART UART4

/* USER CODE END Private defines */

void MX_LPUART1_UART_Init(void);
void MX_UART4_Init(void);
void MX_USART1_UART_Init(void);
void MX_USART2_UART_Init(void);
void MX_USART3_UART_Init(void);

/* USER CODE BEGIN Prototypes */
int32_t lpuart1_init ( void );
int32_t usart1_init ( void );
int32_t usart2_init ( void );
int32_t usart3_init ( void );
int32_t uart4_init ( void );

int32_t lpuart1_deinit ( void );
int32_t usart1_deinit ( void );
int32_t usart2_deinit ( void );
int32_t usart3_deinit ( void );
int32_t uart4_deinit ( void );

bool uart_init_status ( UART_HandleTypeDef *port );
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

