/*
 * generic_uart_driver.c
 *
 *  Created on: Aug 5, 2024
 *      Author: philbush
 */

#include "generic_uart_driver.h"
#include <stddef.h>
#include <stdint.h>

static int32_t _generic_uart_read(void *driver_ptr, uint8_t *read_buf,
                                  uint16_t size, uint32_t timeout_ticks);
static int32_t _generic_uart_write(void *driver_ptr, uint8_t *write_buf,
                                   uint16_t size, uint32_t timeout_ticks);

void generic_uart_register_io_functions(
    generic_uart_driver *driver_ptr, UART_HandleTypeDef *uart_handle,
    TX_SEMAPHORE *uart_sema, uart_init_fn init, uart_deinit_fn deinit,
    uart_read_fn override_read_fn, uart_write_fn override_write_fn) {
  driver_ptr->uart_handle = uart_handle;
  driver_ptr->uart_sema = uart_sema;
  driver_ptr->init = init;
  driver_ptr->deinit = deinit;

  if (override_read_fn != NULL) {
    driver_ptr->read = override_read_fn;
  } else {
    driver_ptr->read = _generic_uart_read;
  }

  if (override_write_fn != NULL) {
    driver_ptr->write = override_write_fn;
  } else {
    driver_ptr->write = _generic_uart_write;
  }
}

static int32_t _generic_uart_read(void *driver_ptr, uint8_t *read_buf,
                                  uint16_t size, uint32_t timeout_ticks) {
  generic_uart_driver *driver_handle = (generic_uart_driver *)driver_ptr;

  HAL_StatusTypeDef uart_ret;
  // the iridium code appears to overwrite the _generic_uart_read in order to
  // run these in case of errors. CT and accel use the generic.
  HAL_UART_DMAStop(driver_handle->uart_handle);
  HAL_UART_Abort(driver_handle->uart_handle);
  memset(read_buf, 0, size);
  uart_ret = HAL_UART_Receive_DMA(driver_handle->uart_handle, read_buf, size);
  if (HAL_OK != uart_ret) {
    return UART_ERR;
  }

  UINT tx_ret;
  tx_ret = tx_semaphore_get(driver_handle->uart_sema, timeout_ticks);
  if (TX_SUCCESS != tx_ret) {
    return UART_ERR;
  }

  return UART_OK;
}

static int32_t _generic_uart_write(void *driver_ptr, uint8_t *write_buf,
                                   uint16_t size, uint32_t timeout_ticks) {
  generic_uart_driver *driver_handle = (generic_uart_driver *)driver_ptr;

  if (HAL_UART_Transmit_DMA(driver_handle->uart_handle, write_buf, size) !=
      HAL_OK) {
    return UART_ERR;
  }

  if (tx_semaphore_get(driver_handle->uart_sema, timeout_ticks) != TX_SUCCESS) {
    return UART_ERR;
  }

  return UART_OK;
}
