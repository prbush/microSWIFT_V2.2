/*
 * shared_i2c_bus->c
 *
 *  Created on: Dec 13, 2024
 *      Author: philbush
 */

#include "shared_i2c_bus.h"

static Shared_I2C_Bus *i2c_bus;
static Shared_I2C_Server i2c_server;

static uSWIFT_return_code_t _i2c_bus_read(uint8_t dev_addr, uint8_t dev_reg,
                                          uint8_t *input_output_buffer,
                                          uint8_t read_len);
static uSWIFT_return_code_t _i2c_bus_write(uint8_t dev_addr, uint8_t dev_reg,
                                           uint8_t *input_output_buffer,
                                           uint8_t write_len);

bool shared_i2c_init(Shared_I2C_Bus *shared_bus, I2C_HandleTypeDef *i2c_handle,
                     TX_SEMAPHORE *i2c_int_sema) {
  i2c_bus = shared_bus;

  i2c_bus->i2c_handle = i2c_handle;
  i2c_bus->i2c_sema = i2c_int_sema;

  i2c_bus->read = _i2c_bus_read;
  i2c_bus->write = _i2c_bus_write;

  return (i2c2_init() == I2C_OK);
}

void shared_i2c_server_init(TX_QUEUE *i2c_queue,
                            TX_EVENT_FLAGS_GROUP *operation_complete_flags) {
  i2c_server.i2c_queue = i2c_queue;
  i2c_server.operation_complete_flags = operation_complete_flags;
}

bool shared_i2c_deinit(void) { return (i2c2_deinit() == I2C_OK); }

uSWIFT_return_code_t enqueue_i2c_read(uint8_t dev_addr, uint8_t dev_reg,
                                      uint8_t *input_output_buffer,
                                      uint8_t read_len, UINT complete_flag) {
  uSWIFT_return_code_t read_result = uSWIFT_SUCCESS;
  i2c_queue_message queue_msg = {0};
  ULONG event_flags;

  queue_msg.input_output_buffer = input_output_buffer;
  queue_msg.return_code = &read_result;
  queue_msg.complete_flag = complete_flag;
  queue_msg.dev_addr = dev_addr;
  queue_msg.dev_reg = dev_reg;
  queue_msg.data_len = read_len;
  queue_msg.operation_type = I2C_READ;

  if (TX_SUCCESS != tx_queue_send(i2c_server.i2c_queue, &queue_msg, 1)) {
    return uSWIFT_MESSAGE_QUEUE_ERROR;
  }

  if (TX_SUCCESS != tx_event_flags_get(i2c_server.operation_complete_flags,
                                       complete_flag, TX_OR_CLEAR, &event_flags,
                                       SHARED_I2C_WAIT_TICKS)) {
    return uSWIFT_TIMEOUT;
  }

  return read_result;
}

uSWIFT_return_code_t enqueue_i2c_write(uint8_t dev_addr, uint8_t dev_reg,
                                       uint8_t *input_output_buffer,
                                       uint8_t write_len, UINT complete_flag) {
  uSWIFT_return_code_t write_result = uSWIFT_SUCCESS;
  i2c_queue_message queue_msg = {0};
  ULONG event_flags;

  queue_msg.input_output_buffer = input_output_buffer;
  queue_msg.return_code = &write_result;
  queue_msg.complete_flag = complete_flag;
  queue_msg.dev_addr = dev_addr;
  queue_msg.dev_reg = dev_reg;
  queue_msg.data_len = write_len;
  queue_msg.operation_type = I2C_WRITE;

  if (TX_SUCCESS != tx_queue_send(i2c_server.i2c_queue, &queue_msg, 1)) {
    return uSWIFT_MESSAGE_QUEUE_ERROR;
  }

  if (TX_SUCCESS != tx_event_flags_get(i2c_server.operation_complete_flags,
                                       complete_flag, TX_OR_CLEAR, &event_flags,
                                       SHARED_I2C_WAIT_TICKS)) {
    return uSWIFT_TIMEOUT;
  }

  return write_result;
}

static uSWIFT_return_code_t _i2c_bus_read(uint8_t dev_addr, uint8_t dev_reg,
                                          uint8_t *input_output_buffer,
                                          uint8_t read_len) {
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;

  // In testing, when the semaphore timed out, the I2C bus was often still
  // busy, leading to a series of IO_ERRORS since transactions were attempted
  // in quick succession.
  int busy_count = 0;
  while (busy_count < 5 && HAL_I2C_STATE_READY != i2c_bus->i2c_handle->State) {
    tx_thread_sleep(1);
    busy_count++;
  }
  if (i2c_bus->i2c_handle->State != HAL_I2C_STATE_READY) {
    return uSWIFT_I2C_BUSY_ERROR;
  }

  HAL_StatusTypeDef read_result = HAL_I2C_Mem_Read_IT(
      i2c_bus->i2c_handle, dev_addr, dev_reg, 1, input_output_buffer, read_len);
  if (HAL_OK != read_result) {
    return uSWIFT_I2C_READ_ERROR;
  }

  UINT sema_result = tx_semaphore_get(i2c_bus->i2c_sema, HAL_I2C_WAIT_TICKS);
  if (TX_SUCCESS != sema_result) {
    return uSWIFT_I2C_SEMAPHORE_ERROR;
  }

  return uSWIFT_SUCCESS;
}

static uSWIFT_return_code_t _i2c_bus_write(uint8_t dev_addr, uint8_t dev_reg,
                                           uint8_t *input_output_buffer,
                                           uint8_t write_len) {
  HAL_StatusTypeDef write_result = HAL_OK;
  UINT sema_result;

  int busy_count = 0;
  while (busy_count < 5 && HAL_I2C_STATE_READY != i2c_bus->i2c_handle->State) {
    tx_thread_sleep(1);
    busy_count++;
  }
  if (i2c_bus->i2c_handle->State != HAL_I2C_STATE_READY) {
    return uSWIFT_I2C_BUSY_ERROR;
  }

  if (write_len == 0) {
    write_result = HAL_I2C_Master_Transmit_IT(i2c_bus->i2c_handle, dev_addr,
                                              input_output_buffer, 1);
    if (HAL_OK != write_result) {
      return uSWIFT_I2C_WRITE_ERROR;
    }
  } else {
    write_result = HAL_I2C_Mem_Write_IT(i2c_bus->i2c_handle, dev_addr, dev_reg,
                                        1, input_output_buffer, write_len);
    if (HAL_OK != write_result) {
      return uSWIFT_I2C_WRITE_ERROR;
    }
  }

  sema_result = tx_semaphore_get(i2c_bus->i2c_sema, HAL_I2C_WAIT_TICKS);
  if (sema_result != TX_SUCCESS) {
    return uSWIFT_I2C_SEMAPHORE_ERROR;
  }

  return uSWIFT_SUCCESS;
}
