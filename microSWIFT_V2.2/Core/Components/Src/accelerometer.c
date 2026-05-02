/*
 * Accelerometer.c
 *
 *      Author: Laura
 */

#include "accelerometer.h"
#include "sbd.h"

Accelerometer *accel_self;

// Private function prototypes; since not part of public interface, did not
// include them in .h file
uSWIFT_return_code_t _accel_self_test(accel_self_test_result_t *result);
uSWIFT_return_code_t _accel_set_time(uint32_t timestamp);
uSWIFT_return_code_t _accel_start_sampling(void);
uSWIFT_return_code_t _accel_parse_waves(sbd_message_type_55 *accel_msg);
uSWIFT_return_code_t _accel_uart_init(void);
uSWIFT_return_code_t _accel_uart_deinit(void);
uSWIFT_return_code_t _accel_uart_reset(void);
static void _accel_power_on(void);
static void _accel_power_off(void);

void accelerometer_init(Accelerometer *accel, UART_HandleTypeDef *uart_handle,
                        TX_SEMAPHORE *uart_sema) {
  accel_self = accel;

  accel_self->self_test = _accel_self_test;
  accel_self->set_time = _accel_set_time;
  accel_self->start_sampling = _accel_start_sampling;
  accel_self->parse_waves = _accel_parse_waves;

  accel_self->uart_init = _accel_uart_init;
  accel_self->uart_deinit = _accel_uart_deinit;
  accel_self->uart_reset = _accel_uart_reset;
  accel_self->power_on = _accel_power_on;
  accel_self->power_off = _accel_power_off;

  // Final two args are: override_read_fn, override_write_fn
  // We do not (for now?) need to overwrite read/write.
  generic_uart_register_io_functions(&accel_self->uart_driver, uart_handle,
                                     uart_sema, usart2_init, usart2_deinit,
                                     NULL, NULL);
}

void _accel_power_on(void) {
  HAL_GPIO_WritePin(EXP_GPIO_1_GPIO_Port, EXP_GPIO_1_Pin, GPIO_PIN_SET);
}

void _accel_power_off(void) {
  HAL_GPIO_WritePin(EXP_GPIO_1_GPIO_Port, EXP_GPIO_1_Pin, GPIO_PIN_RESET);
}

uSWIFT_return_code_t _accel_set_time(uint32_t timestamp) {
  uint8_t set_time_command[6];
  strcpy(set_time_command, "CK");
  memcpy(&set_time_command[2], &timestamp, sizeof(uint32_t));

  UINT ret;
  ret = accel_self->uart_driver.write(
      &accel_self->uart_driver, set_time_command, 6, ACCEL_MAX_UART_TX_TICKS);
  if (UART_OK != ret) {
    return uSWIFT_IO_ERROR;
  }
  return uSWIFT_SUCCESS;
}

uSWIFT_return_code_t _accel_start_sampling(void) {
  const char *start_sampling_command = "RW";
  UINT ret;
  ret = accel_self->uart_driver.write(
      &accel_self->uart_driver, (uint8_t *)&(start_sampling_command[0]),
      strlen(start_sampling_command), ACCEL_MAX_UART_TX_TICKS);
  if (UART_OK != ret) {
    return uSWIFT_IO_ERROR;
  }
  return uSWIFT_SUCCESS;
}

uSWIFT_return_code_t _accel_parse_waves(sbd_message_type_55 *accel_msg) {
  // TODO: It'd probably be cleaner to add some sentinel bytes to the start of
  // the struct, rather than having transmit and receive sides doing this.
  const char *start_sampling_command = "RW";

  static int response_length = 2 + 340;
  char waves_response[response_length];
  memset(waves_response, 0, response_length);
  // Blocking read for 19 minutes (at 3.9 Hz, 4096 samples is 17.5 minutes)
  UINT ret = accel_self->uart_driver.read(
      &accel_self->uart_driver, (uint8_t *)&(waves_response[0]),
      response_length, TX_TIMER_TICKS_PER_SECOND * 60 * 19);
  if (UART_OK != ret) {
    return uSWIFT_IO_ERROR;
  }
  if (0 != strncmp(start_sampling_command, waves_response, 2)) {
    // TODO: This should be robust to getting different messages; should wait
    // until it gets a response, and go with that one.
    // TODO(LEL): Create appropriate error for ACCEL_NO_DATA (separate from the
    // initialization failure.)
    return uSWIFT_IO_ERROR;
  }

  memcpy(accel_msg, &waves_response[2], sizeof(sbd_message_type_55));
  return uSWIFT_SUCCESS;
}

uSWIFT_return_code_t _accel_self_test(accel_self_test_result_t *result) {
  int32_t ret;

  const char *self_test_command = "ST";
  ret = accel_self->uart_driver.write(
      &accel_self->uart_driver, (uint8_t *)&(self_test_command[0]),
      strlen(self_test_command), ACCEL_MAX_UART_TX_TICKS);
  if (UART_OK != ret) {
    return uSWIFT_IO_ERROR;
  }

  // The slowest we'll run is 4Hz, so need to wait long enough for the next
  // sample to arrive. Half a second was too short.
  uint32_t read_timeout = TX_TIMER_TICKS_PER_SECOND;
  static int response_length = 2 + sizeof(accel_self_test_result_t);
  char self_test_response[response_length];
  memset(self_test_response, 0, response_length);
  ret = accel_self->uart_driver.read(&accel_self->uart_driver,
                                     (uint8_t *)&(self_test_response[0]),
                                     response_length, read_timeout);
  if (UART_OK != ret) {
    return uSWIFT_IO_ERROR;
  }
  if (0 != strncmp(self_test_command, self_test_response, 2)) {
    return uSWIFT_IO_ERROR;
  }

  memcpy(result, &self_test_response[2], sizeof(accel_self_test_result_t));

  return uSWIFT_SUCCESS;
}

uSWIFT_return_code_t _accel_uart_init() {
  int32_t ret;
  ret = accel_self->uart_driver.init();
  if (UART_OK != ret) {
    return uSWIFT_IO_ERROR;
  } else {
    return uSWIFT_SUCCESS;
  }
}

uSWIFT_return_code_t _accel_uart_deinit() {
  accel_self->uart_driver.deinit();

  HAL_DMA_DeInit(accel_self->tx_dma_handle);
  HAL_DMA_DeInit(accel_self->rx_dma_handle);

  // (These are linked to the Expansion port in app_threadx.c)
  HAL_NVIC_ClearPendingIRQ(GPDMA1_Channel2_IRQn);
  HAL_NVIC_ClearPendingIRQ(GPDMA1_Channel3_IRQn);
  HAL_NVIC_ClearPendingIRQ(USART2_IRQn);

  HAL_NVIC_DisableIRQ(GPDMA1_Channel2_IRQn);
  HAL_NVIC_DisableIRQ(GPDMA1_Channel3_IRQn);
  HAL_NVIC_DisableIRQ(USART2_IRQn);

  return uSWIFT_SUCCESS;
}

uSWIFT_return_code_t _accel_uart_reset() {
  // TODO: Fil this in if I use it!
  return uSWIFT_SUCCESS;
}
