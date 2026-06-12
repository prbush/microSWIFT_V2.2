/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
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
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32u5xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdbool.h"

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

#define COMPILE_TIME_DATE_BUFFER_SIZE 32
extern char compile_date[COMPILE_TIME_DATE_BUFFER_SIZE];
extern char compile_time[COMPILE_TIME_DATE_BUFFER_SIZE];
/* USER CODE END PV */
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define CT_FET_Pin GPIO_PIN_2
#define CT_FET_GPIO_Port GPIOE
#define SYS_I2C_SDA_Pin GPIO_PIN_0
#define SYS_I2C_SDA_GPIO_Port GPIOF
#define SYS_I2C_SCL_Pin GPIO_PIN_1
#define SYS_I2C_SCL_GPIO_Port GPIOF
#define RAM_OCTOSPI_SIO_3_Pin GPIO_PIN_6
#define RAM_OCTOSPI_SIO_3_GPIO_Port GPIOF
#define RAM_OCTOSPI_SIO_2_Pin GPIO_PIN_7
#define RAM_OCTOSPI_SIO_2_GPIO_Port GPIOF
#define RAM_OCTOSPI_SIO_0_Pin GPIO_PIN_8
#define RAM_OCTOSPI_SIO_0_GPIO_Port GPIOF
#define RAM_OCTOSPI_SIO_1_Pin GPIO_PIN_9
#define RAM_OCTOSPI_SIO_1_GPIO_Port GPIOF
#define RAM_OCTOSPI_CLK_Pin GPIO_PIN_10
#define RAM_OCTOSPI_CLK_GPIO_Port GPIOF
#define VBATT_ADC_IN_Pin GPIO_PIN_0
#define VBATT_ADC_IN_GPIO_Port GPIOC
#define EXP_SPI_MISO_Pin GPIO_PIN_2
#define EXP_SPI_MISO_GPIO_Port GPIOC
#define IRIDIUM_UART_TX_Pin GPIO_PIN_0
#define IRIDIUM_UART_TX_GPIO_Port GPIOA
#define IRIDIUM_UART_RX_Pin GPIO_PIN_1
#define IRIDIUM_UART_RX_GPIO_Port GPIOA
#define EXP_GPIO_1_Pin GPIO_PIN_2
#define EXP_GPIO_1_GPIO_Port GPIOA
#define RAM_OCTOSPI_CSn_Pin GPIO_PIN_4
#define RAM_OCTOSPI_CSn_GPIO_Port GPIOA
#define RTC_SPI_SCK_Pin GPIO_PIN_5
#define RTC_SPI_SCK_GPIO_Port GPIOA
#define RTC_SPI_MISO_Pin GPIO_PIN_6
#define RTC_SPI_MISO_GPIO_Port GPIOA
#define RTC_SPI_MOSI_Pin GPIO_PIN_7
#define RTC_SPI_MOSI_GPIO_Port GPIOA
#define RTC_INT_B_Pin GPIO_PIN_2
#define RTC_INT_B_GPIO_Port GPIOB
#define RTC_INT_B_EXTI_IRQn EXTI2_IRQn
#define RTC_WDOG_OR_INPUT_Pin GPIO_PIN_13
#define RTC_WDOG_OR_INPUT_GPIO_Port GPIOF
#define RTC_TIMESTAMP_1_Pin GPIO_PIN_14
#define RTC_TIMESTAMP_1_GPIO_Port GPIOF
#define RTC_TIMESTAMP_2_Pin GPIO_PIN_15
#define RTC_TIMESTAMP_2_GPIO_Port GPIOF
#define RTC_TIMESTAMP_3_Pin GPIO_PIN_0
#define RTC_TIMESTAMP_3_GPIO_Port GPIOG
#define RTC_TIMESTAMP_4_Pin GPIO_PIN_1
#define RTC_TIMESTAMP_4_GPIO_Port GPIOG
#define LIGHT_FET_Pin GPIO_PIN_7
#define LIGHT_FET_GPIO_Port GPIOE
#define TOP_HAT_GPIO_SPARE_Pin GPIO_PIN_8
#define TOP_HAT_GPIO_SPARE_GPIO_Port GPIOE
#define BOOT_GPIO_SPARE_Pin GPIO_PIN_9
#define BOOT_GPIO_SPARE_GPIO_Port GPIOE
#define TURBIDITY_FET_Pin GPIO_PIN_10
#define TURBIDITY_FET_GPIO_Port GPIOE
#define TEMPERATURE_FET_Pin GPIO_PIN_11
#define TEMPERATURE_FET_GPIO_Port GPIOE
#define VCP_UART_TX_Pin GPIO_PIN_10
#define VCP_UART_TX_GPIO_Port GPIOB
#define EXP_SPI_SCK_Pin GPIO_PIN_13
#define EXP_SPI_SCK_GPIO_Port GPIOB
#define EXP_SPI_CSn_Pin GPIO_PIN_14
#define EXP_SPI_CSn_GPIO_Port GPIOB
#define EXP_SPI_MOSI_Pin GPIO_PIN_15
#define EXP_SPI_MOSI_GPIO_Port GPIOB
#define LED_RED_Pin GPIO_PIN_8
#define LED_RED_GPIO_Port GPIOD
#define VCP_UART_RX_Pin GPIO_PIN_9
#define VCP_UART_RX_GPIO_Port GPIOD
#define LED_GREEN_Pin GPIO_PIN_10
#define LED_GREEN_GPIO_Port GPIOD
#define RS232_FORCEOFF_Pin GPIO_PIN_11
#define RS232_FORCEOFF_GPIO_Port GPIOD
#define RTC_SPI_CS_Pin GPIO_PIN_14
#define RTC_SPI_CS_GPIO_Port GPIOD
#define IRIDIUM_OnOff_Pin GPIO_PIN_3
#define IRIDIUM_OnOff_GPIO_Port GPIOG
#define GNSS_LPUART_TX_Pin GPIO_PIN_7
#define GNSS_LPUART_TX_GPIO_Port GPIOG
#define GNSS_LPUART_RX_Pin GPIO_PIN_8
#define GNSS_LPUART_RX_GPIO_Port GPIOG
#define SD_CARD_D0_Pin GPIO_PIN_8
#define SD_CARD_D0_GPIO_Port GPIOC
#define CT_UART_TX_Pin GPIO_PIN_9
#define CT_UART_TX_GPIO_Port GPIOA
#define CT_UART_RX_Pin GPIO_PIN_10
#define CT_UART_RX_GPIO_Port GPIOA
#define DEBUG_SWDIO_Pin GPIO_PIN_13
#define DEBUG_SWDIO_GPIO_Port GPIOA
#define DEBUG_SWCLK_Pin GPIO_PIN_14
#define DEBUG_SWCLK_GPIO_Port GPIOA
#define EXP_UART_RX_Pin GPIO_PIN_15
#define EXP_UART_RX_GPIO_Port GPIOA
#define SD_CARD_SCK_Pin GPIO_PIN_12
#define SD_CARD_SCK_GPIO_Port GPIOC
#define IRIDIUM_RI_N_Pin GPIO_PIN_0
#define IRIDIUM_RI_N_GPIO_Port GPIOD
#define IRIDIUM_RI_N_EXTI_IRQn EXTI0_IRQn
#define SD_CARD_FET_Pin GPIO_PIN_1
#define SD_CARD_FET_GPIO_Port GPIOD
#define SD_CARD_CMD_Pin GPIO_PIN_2
#define SD_CARD_CMD_GPIO_Port GPIOD
#define BUS_5V_FET_Pin GPIO_PIN_4
#define BUS_5V_FET_GPIO_Port GPIOD
#define EXP_UART_TX_Pin GPIO_PIN_5
#define EXP_UART_TX_GPIO_Port GPIOD
#define DEBUG_SWO_Pin GPIO_PIN_3
#define DEBUG_SWO_GPIO_Port GPIOB
#define GNSS_FET_Pin GPIO_PIN_4
#define GNSS_FET_GPIO_Port GPIOB
#define RF_SWITCH_VCTL_Pin GPIO_PIN_1
#define RF_SWITCH_VCTL_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
