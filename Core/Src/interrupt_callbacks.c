/*
 * interrupt_callbacks.c
 *
 *  Created on: Aug 2, 2024
 *      Author: philbush
 */

#include "app_threadx.h"
#include "battery.h"
#include "error_handler.h"
#include "ext_rtc.h"
#include "gnss.h"
#include "gpio.h"
#include "spi.h"
#include "usart.h"

/**
 * @brief Tx Transfer completed callback.
 * @param  hspi: pointer to a SPI_HandleTypeDef structure that contains
 *               the configuration information for SPI module.
 * @retval None
 */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
  if (hspi->Instance == RTC_SPI) {
    (void)tx_semaphore_put(&ext_rtc_spi_sema);
  }
}

/**
 * @brief Rx Transfer completed callback.
 * @param  hspi: pointer to a SPI_HandleTypeDef structure that contains
 *               the configuration information for SPI module.
 * @retval None
 */
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {
  if (hspi->Instance == RTC_SPI) {
    (void)tx_semaphore_put(&ext_rtc_spi_sema);
  }
}

/**
 * @brief Tx and Rx Transfer completed callback.
 * @param  hspi: pointer to a SPI_HandleTypeDef structure that contains
 *               the configuration information for SPI module.
 * @retval None
 */
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
  if (hspi->Instance == RTC_SPI) {
    (void)tx_semaphore_put(&ext_rtc_spi_sema);
  }
}

/**
 * @brief Tx Transfer completed callback.
 * @param huart UART handle.
 * @retval None
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == CT_UART) {
    (void)tx_semaphore_put(&ct_uart_sema);
  } else if (huart->Instance == IRIDIUM_UART) {
    (void)tx_semaphore_put(&iridium_uart_sema);
  } else if (huart->Instance == EXPANSION_UART) {
    (void)tx_semaphore_put(&expansion_uart_sema);
  } else if (huart->Instance == GNSS_UART) {
    (void)tx_event_flags_set(&irq_flags, GNSS_TX_COMPLETE, TX_OR);
  } else if (huart->Instance == LOGGER_UART) {
    (void)tx_semaphore_put(&logger_sema);
  }
}

/**
 * @brief  Rx Transfer completed callback.
 * @param  huart UART handle.
 * @retval None
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == CT_UART) {
    (void)tx_semaphore_put(&ct_uart_sema);
  } else if (huart->Instance == IRIDIUM_UART) {
    (void)tx_semaphore_put(&iridium_uart_sema);
  } else if (huart->Instance == EXPANSION_UART) {
    (void)tx_semaphore_put(&expansion_uart_sema);
  } else if (huart->Instance == GNSS_UART) {
    if (gnss_get_configured_status()) {
      (void)tx_event_flags_set(&irq_flags, GNSS_MSG_RECEIVED, TX_OR);
    } else {
      (void)tx_event_flags_set(&irq_flags, GNSS_CONFIG_RECVD, TX_OR);
    }
  }
}

/**
 * @brief  Reception Event Callback (Rx event notification called after use of
 * advanced reception service).
 * @param  huart UART handle
 * @param  size  Number of data available in application reception buffer
 * (indicates a position in reception buffer until which, data are available)
 * @retval None
 */
uint16_t msg_size = 0;
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size) {
  if (huart->Instance == CT_UART) {
    (void)tx_semaphore_put(&ct_uart_sema);
  } else if (huart->Instance == IRIDIUM_UART) {
    (void)tx_semaphore_put(&iridium_uart_sema);
  } else if (huart->Instance == EXPANSION_UART)
    (void)tx_semaphore_put(&expansion_uart_sema);
  else if (huart->Instance == GNSS_UART) {
    if (!gnss_get_configured_status()) {
      if (size == FRAME_SYNC_RX_SIZE) {
        (void)tx_event_flags_set(&irq_flags, GNSS_CONFIG_RECVD, TX_OR);
      }
    } else if (size < UBX_NAV_PVT_MESSAGE_LENGTH) {
      msg_size = size;
      (void)tx_event_flags_set(&irq_flags, GNSS_MSG_INCOMPLETE, TX_OR);
    } else {
      (void)tx_event_flags_set(&irq_flags, GNSS_MSG_RECEIVED, TX_OR);
    }
  }
}

/**
 * @brief  UART error callback.
 * @param  huart UART handle.
 * @retval None
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
  /* Prevent unused argument(s) compilation warning */
  uint8_t dummy = 0;
  uint8_t i;

  if (huart->Instance == GNSS_UART) {
    dummy = 1;
    i = dummy;
    dummy = i;
  }
  // TODO(LEL): Should this propagate up somehow? For all supported UARTs?
}

/**
 * @brief  Period elapsed callback in non blocking mode
 * @note   This function is called  when TIM16 interrupt took place, inside
 * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
 * a global variable "uwTick" used as application time base.
 *
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  // High frequency, low overhead ISR, no need to save/restore context
  if (htim->Instance == TIM6) {
    HAL_IncTick();
  }
}

/**
 * @brief  ADC conversion complete callback
 *
 * @param  hadc : ADC handle
 * @retval None
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
  static int16_t num_conversions = 0;
  static uint32_t conversion_accumulator = 0;
  static uint32_t calibration_offset = 0;

  if (num_conversions == 0) {
    calibration_offset = HAL_ADCEx_Calibration_GetValue(hadc, ADC_SINGLE_ENDED);
  }

  if (num_conversions++ < 100) {
    conversion_accumulator += HAL_ADC_GetValue(hadc) + calibration_offset;
  } else {
    conversion_accumulator /= 100;

    // Write the voltage to the battery struct
    battery_set_voltage(conversion_accumulator);

    // Set the conversion complete flag
    tx_event_flags_set(&irq_flags, BATTERY_CONVERSION_COMPLETE, TX_OR);

    // Done with our samples, shut it down.
    HAL_ADC_Stop_IT(hadc);
  }
}

/**
 * @brief  EXTI line rising detection callback.
 * @param  GPIO_Pin: Specifies the port pin connected to corresponding EXTI
 * line.
 * @retval None
 */
void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin) { UNUSED(GPIO_Pin); }

/**
 * @brief  EXTI line falling detection callback.
 * @param  GPIO_Pin: Specifies the port pin connected to corresponding EXTI
 * line.
 * @retval None
 */
void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin) {
  if (GPIO_Pin == RTC_INT_B_Pin) {
    increment_system_time();
  }
  //  else if ( GPIO_Pin == IRIDIUM_RI_N_Pin )
  //  {
  //    // Inform Iridium thread there is a message
  //  }
}

/**
 * @brief  Memory Tx Transfer completed callback.
 * @param  hi2c Pointer to a I2C_HandleTypeDef structure that contains
 *                the configuration information for the specified I2C.
 * @retval None
 */
void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c) {
  if (hi2c->Instance == CORE_I2C_BUS) {
    (void)tx_semaphore_put(&core_i2c_sema);
  }
}

/**
 * @brief  Memory Rx Transfer completed callback.
 * @param  hi2c Pointer to a I2C_HandleTypeDef structure that contains
 *                the configuration information for the specified I2C.
 * @retval None
 */
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c) {
  if (hi2c->Instance == CORE_I2C_BUS) {
    (void)tx_semaphore_put(&core_i2c_sema);
  }
}

/**
 * @brief  Master Tx Transfer completed callback.
 * @param  hi2c Pointer to a I2C_HandleTypeDef structure that contains
 *                the configuration information for the specified I2C.
 * @retval None
 */
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c) {
  if (hi2c->Instance == CORE_I2C_BUS) {
    (void)tx_semaphore_put(&core_i2c_sema);
  }
}

/**
 * @brief  Master Rx Transfer completed callback.
 * @param  hi2c Pointer to a I2C_HandleTypeDef structure that contains
 *                the configuration information for the specified I2C.
 * @retval None
 */
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c) {
  if (hi2c->Instance == CORE_I2C_BUS) {
    (void)tx_semaphore_put(&core_i2c_sema);
  }
}
