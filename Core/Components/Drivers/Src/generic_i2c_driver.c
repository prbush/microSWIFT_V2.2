/*
 * generic_i2c_driver.c
 *
 *  Created on: Sep 17, 2024
 *      Author: philbush
 */

#include "generic_i2c_driver.h"
#include "microSWIFT_return_codes.h"

static int32_t _generic_i2c_read(void *driver_ptr, uint8_t dev_addr,
                                 uint8_t reg_addr, uint8_t *read_buf,
                                 uint16_t size);
static int32_t _generic_i2c_write(void *driver_ptr, uint8_t dev_addr,
                                  uint8_t reg_addr, uint8_t *write_buf,
                                  uint16_t size);

void generic_i2c_register_io_functions(
    generic_i2c_driver *driver_ptr, I2C_HandleTypeDef *i2c_handle,
    TX_MUTEX *i2c_mutex, ULONG mutex_wait_ticks, i2c_init_fn init,
    i2c_deinit_fn deinit, i2c_read_fn override_read_fn,
    i2c_write_fn override_write_fn, bool is_shared_bus) {
  driver_ptr->i2c_handle = i2c_handle;
  driver_ptr->i2c_mutex = i2c_mutex;
  driver_ptr->mutex_wait_ticks = mutex_wait_ticks;
  driver_ptr->init = init;
  driver_ptr->deinit = deinit;
  driver_ptr->is_shared = is_shared_bus;

  if (override_read_fn != NULL) {
    driver_ptr->read = override_read_fn;
  } else {
    driver_ptr->read = _generic_i2c_read;
  }

  if (override_write_fn != NULL) {
    driver_ptr->write = override_write_fn;
  } else {
    driver_ptr->write = _generic_i2c_write;
  }
}

static int32_t _generic_i2c_read(void *driver_ptr, uint8_t dev_addr,
                                 uint8_t reg_addr, uint8_t *read_buf,
                                 uint16_t size) {
  generic_i2c_driver *driver_handle = (generic_i2c_driver *)driver_ptr;
  UINT tx_ret;
  int32_t ret = GENERIC_I2C_OK;

  if (driver_handle->is_shared) {
    tx_ret =
        tx_mutex_get(driver_handle->i2c_mutex, driver_handle->mutex_wait_ticks);
    if (tx_ret != TX_SUCCESS) {
      ret = GENERIC_I2C_BUSY;
    }
  }

  if (ret == GENERIC_I2C_OK) {
    if (HAL_I2C_Mem_Read(driver_handle->i2c_handle, dev_addr, reg_addr, 1,
                         read_buf, size,
                         GENERIC_I2C_BLOCKING_TIMEOUT) != HAL_OK) {
      ret = GENERIC_I2C_ERROR;
    }
  }

  if (driver_handle->is_shared) {
    (void)tx_mutex_put(driver_handle->i2c_mutex);
  }

  return ret;
}

static int32_t _generic_i2c_write(void *driver_ptr, uint8_t dev_addr,
                                  uint8_t reg_addr, uint8_t *write_buf,
                                  uint16_t size) {
  generic_i2c_driver *driver_handle = (generic_i2c_driver *)driver_ptr;
  UINT tx_ret;
  int32_t ret = GENERIC_I2C_OK;

  if (driver_handle->is_shared) {
    tx_ret =
        tx_mutex_get(driver_handle->i2c_mutex, driver_handle->mutex_wait_ticks);
    if (tx_ret != TX_SUCCESS) {
      ret = GENERIC_I2C_BUSY;
    }
  }

  if (ret == GENERIC_I2C_OK) {
    if (HAL_I2C_Mem_Write(driver_handle->i2c_handle, dev_addr, reg_addr, 1,
                          write_buf, size,
                          GENERIC_I2C_BLOCKING_TIMEOUT) != HAL_OK) {
      ret = GENERIC_I2C_ERROR;
    }
  }

  if (driver_handle->is_shared) {
    (void)tx_mutex_put(driver_handle->i2c_mutex);
  }

  return ret;
}
