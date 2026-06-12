/*
 * CT.h
 *
 *  Created on: Oct 28, 2022
 *      Author: Phil
 */

#ifndef SRC_CT_H_
#define SRC_CT_H_

// #include "app_threadx.h"
#include "configuration.h"
#include "generic_uart_driver.h"
#include "gpio.h"
#include "microSWIFT_return_codes.h"
#include "tx_api.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#ifdef DEBUGGING_FAST_CYCLE
#define WARMUP_TIME 20
#else
#define WARMUP_TIME 20000
#endif
// The total length of a response sentence from the sensor
#define CT_DATA_ARRAY_SIZE 290
#define TEMP_MEASUREMENT_START_INDEX 70
#define TEMP_OFFSET_FROM_UNITS 6
#define SALINITY_OFFSET_FROM_UNITS 4
#define SAMPLE_TIME_IN_MILLISECONDS 2000
#define CT_VALUES_ERROR_CODE 0x70E2
#define CT_UART_TX_TIMEOUT_TICKS (TX_TIMER_TICKS_PER_SECOND / 10)
#define CT_UART_RX_TIMEOUT_TICKS (TX_TIMER_TICKS_PER_SECOND * 3)
#define TOTAL_CT_SAMPLES 10

typedef struct {
  float salinity;
  float temp;
} ct_sample;

typedef struct CT {
  // Our global configuration struct
  microSWIFT_configuration *global_config;
  generic_uart_driver uart_driver;
  // The UART and DMA handle for the CT interface
  UART_HandleTypeDef *uart_handle;
  DMA_HandleTypeDef *tx_dma_handle;
  DMA_HandleTypeDef *rx_dma_handle;
  // Event flags
  TX_EVENT_FLAGS_GROUP *error_flags;
  // Pointer to timer
  TX_TIMER *timer;
  // Power switch
  gpio_pin_struct pwr_fet;
  gpio_pin_struct rs232_forceoff;
  // The buffer written to by CT sensor
  char data_buf[CT_DATA_ARRAY_SIZE];
  // Arrays to hold conductivity/temp values
  ct_sample samples[TOTAL_CT_SAMPLES];
  ct_sample samples_averages;
  // Track the number of samples
  int32_t total_samples;
  // Sampling start/stop time
  time_t start_timestamp;
  time_t stop_timestamp;
  // Timer timeout
  bool timer_timeout;
  // Function pointers
  uSWIFT_return_code_t (*parse_sample)(void);
  uSWIFT_return_code_t (*get_averages)(ct_sample *readings);
  uSWIFT_return_code_t (*self_test)(bool add_warmup_time,
                                    ct_sample *optional_readings);
  uSWIFT_return_code_t (*uart_init)(void);
  uSWIFT_return_code_t (*reset_ct_uart)(void);
  uSWIFT_return_code_t (*start_timer)(uint16_t timeout_in_minutes);
  uSWIFT_return_code_t (*stop_timer)(void);
  void (*on)(void);
  void (*off)(void);
} CT;

void ct_init(CT *struct_ptr, microSWIFT_configuration *global_config,
             UART_HandleTypeDef *ct_uart_handle,
             DMA_HandleTypeDef *ct_tx_dma_handle,
             DMA_HandleTypeDef *ct_rx_dma_handle, TX_SEMAPHORE *uart_sema,
             TX_EVENT_FLAGS_GROUP *error_flags, TX_TIMER *timer);

void ct_deinit(void);

void ct_timer_expired(ULONG expiration_input);
bool ct_get_timeout_status(void);

#endif /* SRC_CT_H_ */
