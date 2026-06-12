/*
 * generic_i2c_driver.h
 *
 *  Created on: Sep 17, 2024
 *      Author: philbush
 */

#ifndef COMPONENTS_DRIVERS_INC_generic_I2C_DRIVER_H_
#define COMPONENTS_DRIVERS_INC_generic_I2C_DRIVER_H_

#include "i2c.h"
#include "stdbool.h"
#include "tx_api.h"

// @formatter:off
#define GENERIC_I2C_OK 0
#define GENERIC_I2C_ERROR -1
#define GENERIC_I2C_BUSY -2

#define GENERIC_I2C_BLOCKING_TIMEOUT 5 // ms

typedef int32_t (*i2c_init_fn)(void);
typedef int32_t (*i2c_deinit_fn)(void);
typedef int32_t (*i2c_read_fn)(void *driver_ptr, uint8_t dev_addr,
                               uint8_t reg_addr, uint8_t *read_buf,
                               uint16_t size);
typedef int32_t (*i2c_write_fn)(void *driver_ptr, uint8_t dev_addr,
                                uint8_t reg_addr, uint8_t *write_buf,
                                uint16_t size);

typedef struct {
  I2C_HandleTypeDef *i2c_handle;
  TX_MUTEX *i2c_mutex;

  ULONG mutex_wait_ticks;

  i2c_init_fn init;
  i2c_deinit_fn deinit;
  i2c_read_fn read;
  i2c_write_fn write;

  bool is_shared;
} generic_i2c_driver;
//@formatter:on
void generic_i2c_register_io_functions(
    generic_i2c_driver *driver_ptr, I2C_HandleTypeDef *i2c_handle,
    TX_MUTEX *i2c_mutex, ULONG mutex_wait_ticks, i2c_init_fn init,
    i2c_deinit_fn deinit, i2c_read_fn override_read_fn,
    i2c_write_fn override_write_fn, bool is_shared_bus);
void generic_i2c_set_timeout_ticks(generic_i2c_driver *driver_ptr,
                                   ULONG tx_timeout_ticks,
                                   ULONG rx_timeout_ticks);

#endif /* COMPONENTS_DRIVERS_INC_generic_I2C_DRIVER_H_ */
