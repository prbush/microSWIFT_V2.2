/*
 * error_handler.c
 *
 *  Created on: Aug 22, 2024
 *      Author: philbush
 */

#include "error_handler.h"
#include "threadx_support.h"
#include "persistent_ram.h"
#include "app_threadx.h"
#include "main.h"
#include "battery.h"
#include "sdmmc.h"

int skip_safe_mode = 0;

void Error_Handler ( void )
{
  if ( skip_safe_mode == 0U )
  {
    safe_mode ();
  }

  persistent_ram_deinit ();

  while ( 1 )
  {
    HAL_NVIC_SystemReset ();
  }
}

void safe_mode ( void )
{
  int i, j;
  // Shut down all interfaces
  (void) i2c2_deinit ();
  (void) octospi1_deinit ();
  (void) sdmmc1_deinit ();
  (void) spi1_deinit ();
  (void) spi2_deinit ();
  (void) lpuart1_deinit ();
  (void) usart1_deinit ();
  (void) usart2_deinit ();
  (void) usart3_deinit ();
  (void) uart4_deinit ();
  (void) battery_deinit ();

  __disable_irq ();

  // Turn everything off
  HAL_GPIO_WritePin (CT_FET_GPIO_Port, CT_FET_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin (RF_SWITCH_VCTL_GPIO_Port, RF_SWITCH_VCTL_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin (GNSS_FET_GPIO_Port, GNSS_FET_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin (BUS_5V_FET_GPIO_Port, BUS_5V_FET_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin (SD_CARD_FET_GPIO_Port, SD_CARD_FET_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin (RS232_FORCEOFF_GPIO_Port, RS232_FORCEOFF_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin (LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin (LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin (TEMPERATURE_FET_GPIO_Port, TEMPERATURE_FET_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin (TURBIDITY_FET_GPIO_Port, TURBIDITY_FET_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin (LIGHT_FET_GPIO_Port, LIGHT_FET_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin (RTC_WDOG_OR_INPUT_GPIO_Port, RTC_WDOG_OR_INPUT_Pin, GPIO_PIN_RESET);

  // Short delay with no dependencies
  for ( i = 0; i < 1600000; i++ )
  {
    j = i;
    i = j;
  }
}
