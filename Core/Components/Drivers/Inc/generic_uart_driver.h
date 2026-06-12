/*
 * generic_uart_driver.h
 *
 *  Created on: Aug 5, 2024
 *      Author: philbush
 */

#ifndef COMPONENTS_DRIVERS_INC_GENERIC_UART_DRIVER_H_
#define COMPONENTS_DRIVERS_INC_GENERIC_UART_DRIVER_H_

#include "tx_api.h"
#include "usart.h"

#define GENERIC_UART_OK 0
#define GENERIC_UART_ERROR -1

typedef int32_t (*uart_init_fn)(void);
typedef int32_t (*uart_deinit_fn)(void);
typedef int32_t (*uart_read_fn)(void *driver_ptr, uint8_t *read_buf,
                                uint16_t size, uint32_t timeout_ticks);
typedef int32_t (*uart_write_fn)(void *driver_ptr, uint8_t *write_buf,
                                 uint16_t size, uint32_t timeout_ticks);

typedef struct {
  UART_HandleTypeDef *uart_handle;
  TX_SEMAPHORE *uart_sema;

  uart_init_fn init;
  uart_deinit_fn deinit;
  uart_read_fn read;
  uart_write_fn write;
} generic_uart_driver;

void generic_uart_register_io_functions(
    generic_uart_driver *driver_ptr, UART_HandleTypeDef *uart_handle,
    TX_SEMAPHORE *uart_sema, uart_init_fn init, uart_deinit_fn deinit,
    uart_read_fn override_read_fn, uart_write_fn override_write_fn);

#endif /* COMPONENTS_DRIVERS_INC_GENERIC_UART_DRIVER_H_ */
