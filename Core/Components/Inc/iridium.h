/*
 * Iridium.h
 *
 *  Created on: Oct 28, 2022
 *      Author: Phil
 */

#ifndef SRC_IRIDIUM_H_
#define SRC_IRIDIUM_H_

#include "NEDWaves/rtwhalf.h"
#include "configuration.h"
#include "generic_uart_driver.h"
#include "gpio.h"
#include "microSWIFT_return_codes.h"
#include "sbd.h"
#include "tx_api.h"
#include <stdbool.h>
#include <time.h>

// Macros
#define IRIDIUM_INITIAL_CAP_CHARGE_TIME (TX_TIMER_TICKS_PER_SECOND * 30U)
#define IRIDIUM_TOP_UP_CAP_CHARGE_TIME (TX_TIMER_TICKS_PER_SECOND * 5U)
#define IRIDIUM_MAX_UART_TX_TICKS (TX_TIMER_TICKS_PER_SECOND)
#define IRIDIUM_MAX_UART_RX_TICKS_NO_TX (TX_TIMER_TICKS_PER_SECOND * 5U)
#define IRIDIUM_MAX_UART_RX_TICKS_TX (TX_TIMER_TICKS_PER_SECOND * 50U)
#define MODEM_SLEEP_TIME (TX_TIMER_TICKS_PER_SECOND * 28U)
#define ACK_MESSAGE_SIZE 9U
#define DISABLE_FLOW_CTRL_SIZE 12U
#define ENABLE_RI_SIZE 18U
#define STORE_CONFIG_SIZE 12U
#define SELECT_PWR_UP_SIZE 12U
#define FLUSH_TO_EPPROM_SIZE 11U
#define SBDWB_READY_RESPONSE_SIZE 22U
#define SBDWB_LOAD_RESPONSE_SIZE 11U
#define SBDWB_RESPONSE_CODE_INDEX 2U
#define SBDI_RESPONSE_CODE_INDEX 16U
#define SBDIX_RESPONSE_CODE_INDEX 18U
#define SBDI_RESPONSE_SIZE 19U
#define SBDIX_RESPONSE_SIZE 50U
#define SBDIX_CODES_START_INDEX 9U
#define SBDIX_ECHO_RESPONSE_SIZE 10U
#define SBDRT_ECHO_RESPONSE_SIZE 20U
#define SBDD_RESPONSE_SIZE 20U
#define SBDIX_WAIT_TIME ONE_SECOND * 13U
#define ERROR_MESSAGE_TYPE 99U
#define IRIDIUM_MAX_RESPONSE_SIZE 64U
#define IRIDIUM_DEFAULT_BAUD_RATE 19200U
#define CHECKSUM_FIRST_BYTE_INDEX 327U
#define CHECKSUM_SECOND_BYTE_INDEX 328U
#define ASCII_ZERO 48U
#define ASCII_FIVE 53U

// clang-format off
typedef enum
{
  MO_STATUS         = 0,
  MOMSN             = 1,
  MT_STATUS         = 2,
  MTMSN             = 3,
  MT_LENGTH         = 4,
  MT_QUEUED         = 5,
  SBDIX_NUM_CODES   = 6
} iridium_sbdix_response_codes_index_t;
// clang-format on

typedef struct Iridium {
  // Our global configuration struct
  microSWIFT_configuration *global_config;
  // UART driver
  generic_uart_driver uart_driver;
  // DMA handles for the Iridium UART port
  DMA_HandleTypeDef *uart_tx_dma_handle;
  DMA_HandleTypeDef *uart_rx_dma_handle;
  // Pointer to hardware timer handle
  TX_TIMER *timer;
  // Event flags
  TX_EVENT_FLAGS_GROUP *error_flags;
  // UART response buffer
  uint8_t response_buffer[IRIDIUM_MAX_RESPONSE_SIZE];
  uint8_t
      configuration_buffer[IRIDIUM_SBD_MAX_LENGTH + SBDRT_ECHO_RESPONSE_SIZE];
  // SBDIX response codes
  uint32_t sbdix_response_codes[SBDIX_NUM_CODES];

  gpio_pin_struct bus_5v_fet;
  gpio_pin_struct sleep_pin;

  bool timer_timeout;
  bool configuration_received;
  bool receive_to_idle;

  uSWIFT_return_code_t (*config)(void);
  uSWIFT_return_code_t (*self_test)(void);
  uSWIFT_return_code_t (*start_timer)(uint16_t timeout_in_minutes);
  uSWIFT_return_code_t (*stop_timer)(void);
  uSWIFT_return_code_t (*transmit_message)(uint8_t *msg, uint32_t msg_size);
  uSWIFT_return_code_t (*receive_configuration)(void);
  void (*charge_caps)(uint32_t caps_charge_time_ticks);
  void (*sleep)(void);
  void (*wake)(void);
  void (*on)(void);
  void (*off)(void);
  void (*cycle_power)(void);
} Iridium;

/* Function declarations */
void iridium_init(Iridium *struct_ptr, microSWIFT_configuration *global_config,
                  UART_HandleTypeDef *iridium_uart_handle,
                  TX_SEMAPHORE *uart_sema,
                  DMA_HandleTypeDef *uart_tx_dma_handle,
                  DMA_HandleTypeDef *uart_rx_dma_handle, TX_TIMER *timer,
                  TX_EVENT_FLAGS_GROUP *error_flags);
void iridium_deinit(void);
void iridium_timer_expired(ULONG expiration_input);
bool iridium_get_timeout_status(void);
bool iridium_get_configuration_received_status(void);

#endif /* SRC_IRIDIUM_H_ */
