/*
 * CT.h
 *      Author: Laura
 */

#ifndef SRC_ACCELEROMETER_H_
#define SRC_ACCELEROMETER_H_

#include "generic_uart_driver.h"
#include "microSWIFT_return_codes.h"
#include "sbd.h"
#include "tx_api.h"
#include <stdint.h>

#define ACCEL_MAX_UART_TX_TICKS (TX_TIMER_TICKS_PER_SECOND)

struct Accelerometer;

// This must also be defined in the accelerometer code; the accelerometer
// and controller do not currently have a way to share header files.
typedef struct accel_self_test_result_t {
  uint32_t timestamp;
  float temp_c;
  float x_g;
  float y_g;
  float z_g;
} accel_self_test_result_t;

typedef struct Accelerometer {
  // Global configuration struct.
  // The accelerometer will probably (eventually) need it for
  // setting thresholds for wake-on-shake.
  // microSWIFT_configuration  *global_config;

  generic_uart_driver uart_driver;

  // CT had ct_uart_handle, which I think is unused?
  // CT had {rx,tx}_dma_handle, but they're ONLY used in the deinit function
  // TODO(lindzey): Figure out if these are actually used, and if so, whether
  //    they should be grouped in the uart_driver rather than the Accelerometer
  //    struct
  DMA_HandleTypeDef *tx_dma_handle;
  DMA_HandleTypeDef *rx_dma_handle;

  uSWIFT_return_code_t (*self_test)(accel_self_test_result_t *result);
  uSWIFT_return_code_t (*set_time)(uint32_t timestamp);
  uSWIFT_return_code_t (*run_once)(void);
  uSWIFT_return_code_t (*start_continuous)(void);
  uSWIFT_return_code_t (*parse_waves)(sbd_message_type_55 *accel_msg,
                                      float *priority);
  uSWIFT_return_code_t (*next_spectra)(sbd_message_type_55 *accel_msg,
                                       float *priority);
  uSWIFT_return_code_t (*uart_init)(void);
  uSWIFT_return_code_t (*uart_deinit)(void);
  uSWIFT_return_code_t (*uart_reset)(void);
  void (*power_on)(void);
  void (*power_off)(void);

} Accelerometer;

void accelerometer_init(Accelerometer *struct_ptr,
                        UART_HandleTypeDef *accel_uart_handle,
                        TX_SEMAPHORE *uart_sema);

#endif // 1SRC_ACCELEROMETER_H_