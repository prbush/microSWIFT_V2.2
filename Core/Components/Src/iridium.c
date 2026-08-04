/*
 * Iridium.cpp
 *
 *  Created on: Oct 28, 2022
 *      Author: Phil
 */

#include "iridium.h"
#include "NEDWaves/rtwhalf.h"
#include "app_threadx.h"
#include "configuration.h"
#include "ext_rtc_server.h"
#include "logger.h"
#include "main.h"
#include "persistent_ram.h"
#include "stm32u5xx_hal.h"
#include "stm32u5xx_hal_tim.h"
#include "stm32u5xx_ll_dma.h"
#include "tx_api.h"
#include "usart.h"
#include "watchdog.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// Object pointer
static Iridium *iridium_self;
// Object functions
static uSWIFT_return_code_t _iridium_config(void);
static uSWIFT_return_code_t _iridium_self_test(void);
static uSWIFT_return_code_t _iridium_start_timer(uint16_t timeout_in_minutes);
static uSWIFT_return_code_t _iridium_stop_timer(void);
static uSWIFT_return_code_t _iridium_transmit_message(uint8_t *msg,
                                                      uint32_t msg_size);
static uSWIFT_return_code_t _iridium_receive_configuration(void);
static void _iridium_charge_caps(uint32_t caps_charge_time_ticks);
static void _iridium_sleep(void);
static void _iridium_wake(void);
static void _iridium_on(void);
static void _iridium_off(void);
static void _iridium_cycle_power(void);

// Helper functions
static uSWIFT_return_code_t __send_basic_command_message(const char *command,
                                                         uint8_t response_size);
static uSWIFT_return_code_t __internal_transmit_message(uint8_t *payload,
                                                        uint16_t payload_size);
static uSWIFT_return_code_t __internal_receive_message(uint8_t *receive_buffer,
                                                       uint16_t receive_size);
static void __assemble_ota_ack_message(microSWIFT_configuration *rcvd_config);
static uSWIFT_return_code_t __flush_mt_buffer(void);
static iridium_checksum_t __get_checksum(uint8_t *payload, size_t payload_size);
static void __reset_uart(void);
static int32_t __uart_read_dma(void *driver_ptr, uint8_t *read_buf,
                               uint16_t size, uint32_t timeout_ticks);
static int32_t __uart_write_dma(void *driver_ptr, uint8_t *write_buf,
                                uint16_t size, uint32_t timeout_ticks);

// AT commands
static const char *ack = "AT\r";
static const char *disable_flow_control = "AT&K0\r";
static const char *enable_ring_indications = "AT+SBDMTA=1\r";
static const char *store_config = "AT&W0\r";
static const char *select_power_up_profile = "AT&Y0\r";
// static const char *flush_to_eeprom              = "AT*F\r";
static const char *clear_MO = "AT+SBDD0\r";
static const char *send_sbd = "AT+SBDIX\r";
static const char *receive_msg = "AT+SBDRT\r";

/**
 * Initialize the Iridium struct
 *
 * @return void
 */
void iridium_init(Iridium *struct_ptr, microSWIFT_configuration *global_config,
                  UART_HandleTypeDef *iridium_uart_handle,
                  TX_SEMAPHORE *uart_sema,
                  DMA_HandleTypeDef *uart_tx_dma_handle,
                  DMA_HandleTypeDef *uart_rx_dma_handle, TX_TIMER *timer,
                  TX_EVENT_FLAGS_GROUP *error_flags) {
  // Assign the object pointer
  iridium_self = struct_ptr;

  iridium_self->global_config = global_config;
  iridium_self->uart_tx_dma_handle = uart_tx_dma_handle;
  iridium_self->uart_rx_dma_handle = uart_rx_dma_handle;
  iridium_self->timer = timer;
  iridium_self->error_flags = error_flags;

  memset(&(iridium_self->response_buffer[0]), 0, IRIDIUM_MAX_RESPONSE_SIZE);
  memset(&(iridium_self->configuration_buffer[0]), 0,
         sizeof(iridium_self->configuration_buffer));
  memset(&(iridium_self->sbdix_response_codes), 99U,
         sizeof(iridium_self->sbdix_response_codes));

  iridium_self->bus_5v_fet.port = BUS_5V_FET_GPIO_Port;
  iridium_self->bus_5v_fet.pin = BUS_5V_FET_Pin;
  iridium_self->sleep_pin.port = IRIDIUM_OnOff_GPIO_Port;
  iridium_self->sleep_pin.pin = IRIDIUM_OnOff_Pin;

  iridium_self->timer_timeout = false;
  iridium_self->configuration_received = false;
  iridium_self->receive_to_idle = false;

  iridium_self->config = _iridium_config;
  iridium_self->self_test = _iridium_self_test;
  iridium_self->start_timer = _iridium_start_timer;
  iridium_self->stop_timer = _iridium_stop_timer;
  iridium_self->transmit_message = _iridium_transmit_message;
  iridium_self->receive_configuration = _iridium_receive_configuration;
  iridium_self->charge_caps = _iridium_charge_caps;
  iridium_self->sleep = _iridium_sleep;
  iridium_self->wake = _iridium_wake;
  iridium_self->on = _iridium_on;
  iridium_self->off = _iridium_off;
  iridium_self->cycle_power = _iridium_cycle_power;

  generic_uart_register_io_functions(
      &iridium_self->uart_driver, iridium_uart_handle, uart_sema, uart4_init,
      uart4_deinit, __uart_read_dma, __uart_write_dma);
}

/**
 * Deinit the UART and DMA Tx/ Rx channels
 *
 * @return void
 */
void iridium_deinit(void) {
  (void)iridium_self->uart_driver.deinit();
  HAL_DMA_DeInit(iridium_self->uart_rx_dma_handle);
  HAL_DMA_DeInit(iridium_self->uart_tx_dma_handle);

  // Just to be overly sure we don't get an erroneous IRQ
  HAL_NVIC_ClearPendingIRQ(GPDMA1_Channel6_IRQn);
  HAL_NVIC_ClearPendingIRQ(GPDMA1_Channel7_IRQn);
  HAL_NVIC_ClearPendingIRQ(UART4_IRQn);

  HAL_NVIC_DisableIRQ(GPDMA1_Channel6_IRQn);
  HAL_NVIC_DisableIRQ(GPDMA1_Channel7_IRQn);
  HAL_NVIC_DisableIRQ(UART4_IRQn);
}

/**
 *
 * @return void
 */
void iridium_timer_expired(ULONG expiration_input) {
  (void)expiration_input;
  iridium_self->timer_timeout = true;
}

/**
 *
 *
 * @return bool
 */
bool iridium_get_timeout_status(void) { return iridium_self->timer_timeout; }

/**
 *
 *
 * @return bool
 */
bool iridium_get_configuration_received_status(void) {
  return iridium_self->configuration_received;
}

/**
 *
 *
 * @return uSWIFT_return_code_t
 */
static uSWIFT_return_code_t _iridium_config(void) {
  uSWIFT_return_code_t return_code;

  // Get an ack message
  return_code = __send_basic_command_message(ack, ACK_MESSAGE_SIZE);
  if (return_code != uSWIFT_SUCCESS) {
    return return_code;
  }

  if (is_first_sample_window()) {
    // disable flow control
    return_code = __send_basic_command_message(disable_flow_control,
                                               DISABLE_FLOW_CTRL_SIZE);
    if (return_code != uSWIFT_SUCCESS) {
      return return_code;
    }
    // enable SBD ring indications
    return_code =
        __send_basic_command_message(enable_ring_indications, ENABLE_RI_SIZE);
    if (return_code != uSWIFT_SUCCESS) {
      return return_code;
    }
    // Store this configuration as profile 0
    return_code = __send_basic_command_message(store_config, STORE_CONFIG_SIZE);
    if (return_code != uSWIFT_SUCCESS) {
      return return_code;
    }
    // set profile 0 as the power-up profile
    return_code = __send_basic_command_message(select_power_up_profile,
                                               SELECT_PWR_UP_SIZE);
    if (return_code != uSWIFT_SUCCESS) {
      return return_code;
    }
  }

  return return_code;
}

/**
 *
 *
 * @return uSWIFT_return_code_t
 */
static uSWIFT_return_code_t _iridium_self_test(void) {
  return __send_basic_command_message(ack, ACK_MESSAGE_SIZE);
}

/**
 *
 *
 * @return uSWIFT_return_code_t
 */
static uSWIFT_return_code_t _iridium_start_timer(uint16_t timeout_in_minutes) {
  ULONG timeout = TX_TIMER_TICKS_PER_SECOND * 60 * timeout_in_minutes;
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;

  if (tx_timer_change(iridium_self->timer, timeout, 0) != TX_SUCCESS) {
    ret = uSWIFT_TIMER_ERROR;
    return ret;
  }

  if (tx_timer_activate(iridium_self->timer) != TX_SUCCESS) {
    ret = uSWIFT_TIMER_ERROR;
  }

  return ret;
}

/**
 *
 *
 * @return uSWIFT_return_code_t
 */
static uSWIFT_return_code_t _iridium_stop_timer(void) {
  return (tx_timer_deactivate(iridium_self->timer) == TX_SUCCESS)
             ? uSWIFT_SUCCESS
             : uSWIFT_TIMER_ERROR;
}

/**
 *
 * @return uSWIFT_return_code_t
 */
static uSWIFT_return_code_t _iridium_transmit_message(uint8_t *msg,
                                                      uint32_t msg_size) {
  uSWIFT_return_code_t return_code = uSWIFT_SUCCESS;
  uSWIFT_return_code_t queue_return_code __attribute__((unused));

  watchdog_check_in(IRIDIUM_THREAD);

  return_code =
      __internal_transmit_message(msg, msg_size - IRIDIUM_CHECKSUM_LENGTH);

  if (return_code == uSWIFT_IO_ERROR) {
    iridium_self->cycle_power();
  }

  return return_code;
}

static uSWIFT_return_code_t _iridium_receive_configuration(void) {
  uSWIFT_return_code_t ret =
      __internal_receive_message(&(iridium_self->configuration_buffer[0]),
                                 sizeof(microSWIFT_configuration));
  // NOTE: the above call actually grabs more bytes than the input size:
  // SBDRT_ECHO_RESPONSE_SIZE + sizeof(microSWIFT_configuration)
  // so the offset here is correct despite looking weird.
  microSWIFT_configuration *rcvd_config = (microSWIFT_configuration *)&(
      iridium_self->configuration_buffer[SBDRT_ECHO_RESPONSE_SIZE]);

  if (ret != uSWIFT_SUCCESS) {
    LOG("Unable to read configuration message. error = %d. Power cycling "
        "Iridium.",
        (int)ret);
    iridium_self->cycle_power();
    return ret;
  } else {
    persistent_ram_set_device_config(rcvd_config, true);
    LOG("New configuration received and applied!");
    iridium_self->configuration_received = false;

    // Write the acknowledgment message
    __assemble_ota_ack_message(rcvd_config);
  }

  return ret;
}

/**
 *
 * @return void
 */
static void _iridium_charge_caps(uint32_t caps_charge_time_ticks) {
  // Power the unit by pulling the sleep pin to ground.
  iridium_self->on();
  iridium_self->wake();

  tx_thread_sleep(caps_charge_time_ticks);
}

/**
 *
 * @return void
 */
static void _iridium_sleep(void) {
  //  __send_basic_command_message (flush_to_eeprom, FLUSH_TO_EPPROM_SIZE);
  HAL_GPIO_WritePin(iridium_self->sleep_pin.port, iridium_self->sleep_pin.pin,
                    GPIO_PIN_RESET);
}

/**
 *
 * @return void
 */
static void _iridium_wake(void) {
  HAL_GPIO_WritePin(iridium_self->sleep_pin.port, iridium_self->sleep_pin.pin,
                    GPIO_PIN_SET);
}

/**
 *
 * @return void
 */
static void _iridium_on(void) {
  HAL_GPIO_WritePin(iridium_self->bus_5v_fet.port, iridium_self->bus_5v_fet.pin,
                    GPIO_PIN_SET);
}

/**
 *
 * @return void
 */
static void _iridium_off(void) {
  HAL_GPIO_WritePin(iridium_self->bus_5v_fet.port, iridium_self->bus_5v_fet.pin,
                    GPIO_PIN_RESET);
}

/**
 *
 * @return void
 */
static void _iridium_cycle_power(void) {
  iridium_self->sleep();
  iridium_self->off();
  tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND * 2);

  iridium_self->on();
  iridium_self->wake();
  tx_thread_sleep(250);
}

/**
 *
 * @return uSWIFT_return_code_t
 */
static uSWIFT_return_code_t
__send_basic_command_message(const char *command, uint8_t response_size) {
  char *needle;

  memset(&(iridium_self->response_buffer[0]), 0, IRIDIUM_MAX_RESPONSE_SIZE);

  if (iridium_self->uart_driver.write(&iridium_self->uart_driver,
                                      (uint8_t *)&(command[0]), strlen(command),
                                      IRIDIUM_MAX_UART_TX_TICKS) != UART_OK) {
    return uSWIFT_IO_ERROR;
  }

  if (iridium_self->uart_driver.read(
          &iridium_self->uart_driver, &(iridium_self->response_buffer[0]),
          response_size, IRIDIUM_MAX_UART_RX_TICKS_NO_TX) != UART_OK) {
    return uSWIFT_IO_ERROR;
  }

  needle = strstr((char *)&(iridium_self->response_buffer[1]), "OK");
  memset(&(iridium_self->response_buffer[0]), 0, IRIDIUM_MAX_RESPONSE_SIZE);
  return (needle == NULL) ? uSWIFT_IO_ERROR : uSWIFT_SUCCESS;
}

/**
 *
 * @return uSWIFT_return_code_t
 */
static uSWIFT_return_code_t __internal_transmit_message(uint8_t *payload,
                                                        uint16_t payload_size) {
  uSWIFT_return_code_t return_code = uSWIFT_TIMEOUT;
  iridium_checksum_t checksum;
  char *needle;
  char *token_str =
      (char *)&(iridium_self->response_buffer[SBDIX_CODES_START_INDEX]);
  char payload_size_str[4];
  char load_sbd[15] = "AT+SBDWB=";
  char SBDWB_response_code;
  char *SBDIX_code_str;

  // Assemble the load_sbd string
  itoa(payload_size, payload_size_str, 10);
  strcat(load_sbd, payload_size_str);
  strcat(load_sbd, "\r");

  while (!iridium_self->timer_timeout) {

    return_code = __send_basic_command_message(ack, ACK_MESSAGE_SIZE);
    if (return_code != uSWIFT_SUCCESS) {
      goto io_error;
    }

    // get the checksum
    checksum = __get_checksum((uint8_t *)payload, payload_size);
    memcpy(&(payload[payload_size]), &checksum, sizeof(iridium_checksum_t));

    // Tell the modem we want to send a message
    watchdog_check_in(IRIDIUM_THREAD);

    if (iridium_self->uart_driver.write(
            &iridium_self->uart_driver, (uint8_t *)&(load_sbd[0]),
            strlen(load_sbd), IRIDIUM_MAX_UART_TX_TICKS) != UART_OK) {
      goto io_error;
    }

    if (iridium_self->uart_driver.read(
            &iridium_self->uart_driver, &(iridium_self->response_buffer[0]),
            SBDWB_READY_RESPONSE_SIZE,
            IRIDIUM_MAX_UART_RX_TICKS_NO_TX) != UART_OK) {
      goto io_error;
    }

    needle = strstr((char *)&(iridium_self->response_buffer[0]), "READY");

    // Clear the response buffer and reset UART for the next step
    memset(&(iridium_self->response_buffer[0]), 0, IRIDIUM_MAX_RESPONSE_SIZE);

    // Fail case
    if (needle == NULL) {
      goto io_error;
    }

    tx_thread_sleep(2);

    watchdog_check_in(IRIDIUM_THREAD);

    // Send over the payload + checksum
    if (iridium_self->uart_driver.write(&iridium_self->uart_driver,
                                        (uint8_t *)&(payload[0]),
                                        payload_size + IRIDIUM_CHECKSUM_LENGTH,
                                        IRIDIUM_MAX_UART_TX_TICKS) != UART_OK) {
      goto io_error;
    }

    if (iridium_self->uart_driver.read(
            &iridium_self->uart_driver, &(iridium_self->response_buffer[0]),
            SBDWB_LOAD_RESPONSE_SIZE,
            IRIDIUM_MAX_UART_RX_TICKS_NO_TX) != UART_OK) {
      goto io_error;
    }

    needle = strstr((char *)&(iridium_self->response_buffer[0]), "\r\n");

    if (needle == NULL) {
      goto io_error;
    }

    SBDWB_response_code = *(needle + 2);

    memset(&(iridium_self->response_buffer[0]), 0, IRIDIUM_MAX_RESPONSE_SIZE);

    // 0 indicates checksum matches
    if (SBDWB_response_code != '0') {
      goto io_error;
    }

    tx_thread_sleep(2);

    watchdog_check_in(IRIDIUM_THREAD);

    // Tell the modem to send the message
    if (iridium_self->uart_driver.write(
            &iridium_self->uart_driver, (uint8_t *)&(send_sbd[0]),
            strlen(send_sbd), IRIDIUM_MAX_UART_TX_TICKS) != UART_OK) {
      goto io_error;
    }

    // This is just to grab the echo response, this will be thrown away
    if (iridium_self->uart_driver.read(
            &iridium_self->uart_driver, &(iridium_self->response_buffer[0]),
            SBDIX_ECHO_RESPONSE_SIZE,
            IRIDIUM_MAX_UART_RX_TICKS_NO_TX) != UART_OK) {
      goto io_error;
    }

    watchdog_check_in(IRIDIUM_THREAD);

    // This is to grab a variable length response code
    iridium_self->receive_to_idle = true;
    if (iridium_self->uart_driver.read(
            &iridium_self->uart_driver, &(iridium_self->response_buffer[0]),
            SBDIX_RESPONSE_SIZE, IRIDIUM_MAX_UART_RX_TICKS_TX) != UART_OK) {
      iridium_self->receive_to_idle = false;
      goto io_error;
    }

    tx_thread_sleep(5);

    iridium_self->receive_to_idle = false;
    watchdog_check_in(IRIDIUM_THREAD);

    for (int i = 0; i < SBDIX_NUM_CODES; i++) {
      SBDIX_code_str = strsep(&token_str, ",");
      if (SBDIX_code_str == NULL) {
        break;
      }
      iridium_self->sbdix_response_codes[i] = atoi(SBDIX_code_str);
    }

    if ((iridium_self->sbdix_response_codes[MT_STATUS] == 1U)) {
      if (iridium_self->sbdix_response_codes[MT_LENGTH] !=
          sizeof(microSWIFT_configuration)) {
        LOG("Message of invalid length received. Length = %lu",
            iridium_self->sbdix_response_codes[MT_LENGTH]);
        (void)__flush_mt_buffer();
      } else {
        LOG("Notified of pending configuration message. Length = %lu",
            iridium_self->sbdix_response_codes[MT_LENGTH]);
        iridium_self->configuration_received = true;
      }
    }

    if (iridium_self->sbdix_response_codes[MO_STATUS] <= 4) {
      // Success case
      __send_basic_command_message(clear_MO, SBDD_RESPONSE_SIZE);
      LOG("Iridium transmission successful. MO status: %d",
          iridium_self->sbdix_response_codes[MO_STATUS]);
      return_code = uSWIFT_SUCCESS;
      break;
    }

    LOG("Iridium transmission unsuccessful. MO status: %d",
        iridium_self->sbdix_response_codes[MO_STATUS]);

    // If message Tx failed, put the modem to sleep and delay for a total of 30
    // seconds
    iridium_self->sleep();

    if (iridium_self->timer_timeout) {
      return_code = uSWIFT_TIMEOUT;
      break;
    }
    // Make sure we've got enough time left to try again
    else if (get_timer_remaining_ticks(iridium_self->timer) <
             (MODEM_SLEEP_TIME + IRIDIUM_TOP_UP_CAP_CHARGE_TIME +
              (TX_TIMER_TICKS_PER_SECOND * 5))) {
      return_code = uSWIFT_TIMEOUT;
      break;
    }

    watchdog_check_in(IRIDIUM_THREAD);
    tx_thread_sleep(MODEM_SLEEP_TIME);
    watchdog_check_in(IRIDIUM_THREAD);

    if (iridium_self->timer_timeout) {
      return_code = uSWIFT_TIMEOUT;
      break;
    }

    iridium_self->wake();
    iridium_self->charge_caps(IRIDIUM_TOP_UP_CAP_CHARGE_TIME);
    watchdog_check_in(IRIDIUM_THREAD);

    memset(&(iridium_self->response_buffer[0]), 0, IRIDIUM_MAX_RESPONSE_SIZE);
  }

  if (iridium_self->timer_timeout) {
    return_code = uSWIFT_TIMEOUT;
  }

  watchdog_check_in(IRIDIUM_THREAD);
  return return_code;

io_error:
  return_code = uSWIFT_IO_ERROR;
  __reset_uart();
  return return_code;
}

// NOTE: this ignores input parameter, instead always putting data into the
// configuration_buffer.
static uSWIFT_return_code_t __internal_receive_message(uint8_t *receive_buffer,
                                                       uint16_t receive_size) {

  int write_ret = iridium_self->uart_driver.write(
      &iridium_self->uart_driver, (uint8_t *)&(receive_msg[0]),
      strlen(receive_msg), IRIDIUM_MAX_UART_TX_TICKS);
  if (UART_OK != write_ret) {
    LOG("Trying to receive iridium; uart_driver.write returned %d",
        (int)write_ret);
    goto io_error;
  }

  // NOTE(LEL): The above write call waits for a semaphore signaling that the
  // write command is finished.

  // the below call fails, with the error "BUSY".
  int read_ret = iridium_self->uart_driver.read(
      &iridium_self->uart_driver, &(iridium_self->configuration_buffer[0]),
      SBDRT_ECHO_RESPONSE_SIZE + receive_size, IRIDIUM_MAX_UART_RX_TICKS_NO_TX);
  if (UART_OK != read_ret) {
    LOG("Trying to receive iridium; uart_driver.read returned %d",
        (int)read_ret);
    goto io_error;
  }

  return uSWIFT_SUCCESS;

io_error:
  __reset_uart();
  return uSWIFT_IO_ERROR;
}

static void __assemble_ota_ack_message(microSWIFT_configuration *rcvd_config) {
  sbd_message_type_99 ack_msg = {0};
  uint32_t timestamp = (uint32_t)get_system_time();
  int32_t lat = 0;
  int32_t lon = 0;
  float msg_lat, msg_lon;
  char v3f, gnss_hi_perf, ct_en, temp_en, light_en, obs_en;

  v3f = (rcvd_config->iridium_v3f) ? 'T' : 'F';
  gnss_hi_perf = (rcvd_config->gnss_high_performance_mode) ? 'T' : 'F';
  ct_en = (rcvd_config->ct_enabled) ? 'T' : 'F';
  temp_en = (rcvd_config->temperature_enabled) ? 'T' : 'F';
  light_en = (rcvd_config->light_enabled) ? 'T' : 'F';
  obs_en = (rcvd_config->turbidity_enabled) ? 'T' : 'F';

  gnss_get_current_lat_lon(&lat, &lon);

  msg_lat = (float)lat / LAT_LON_CONVERSION_FACTOR;
  msg_lon = (float)lon / LAT_LON_CONVERSION_FACTOR;

  memcpy(&ack_msg.timestamp, &timestamp, sizeof(uint32_t));
  memcpy(&ack_msg.latitude, &msg_lat, sizeof(float));
  memcpy(&ack_msg.longitude, &msg_lon, sizeof(float));

  (void)snprintf(
      &(ack_msg.message_body[0]), sizeof(sbd_message_type_99),
      "\r\n\r\nConfig recvd:\r\n"
      "Tracking num: %lu\r\nGNSS samples: %lu\r\nDuty cycle: %lu\r\nTx time: "
      "%lu\r\nGNSS acq "
      "time: %lu\r\nGNSS freq: %lu\r\nLight samples: %lu\r\nLight gain: "
      "%lu\r\nOBS samples: "
      "%lu\r\nV3f: %c\r\nGNSS hi perf: %c\r\nCT en: %c\r\nTemp en: %c\r\nLight "
      "en: %c\r\n"
      "OBS en: %c",
      rcvd_config->tracking_number, rcvd_config->gnss_samples_per_window,
      rcvd_config->duty_cycle, rcvd_config->iridium_max_transmit_time,
      rcvd_config->gnss_max_acquisition_wait_time,
      rcvd_config->gnss_sampling_rate, rcvd_config->total_light_samples,
      rcvd_config->light_sensor_gain, rcvd_config->total_turbidity_samples, v3f,
      gnss_hi_perf, ct_en, temp_en, light_en, obs_en);

  persistent_ram_set_ota_ack_msg(&ack_msg);
  LOG("Sending ack message: %s", ack_msg);
}

static uSWIFT_return_code_t __flush_mt_buffer(void) {
  uSWIFT_return_code_t ret =
      __internal_receive_message(&(iridium_self->configuration_buffer[0]),
                                 iridium_self->sbdix_response_codes[MT_LENGTH]);

  memset(&(iridium_self->configuration_buffer[0]), 0,
         sizeof(iridium_self->configuration_buffer));

  return ret;
}

/**
 *
 * @return void
 */
static iridium_checksum_t __get_checksum(uint8_t *payload,
                                         size_t payload_size) {
  iridium_checksum_t checksum = {0};
  uint16_t temp_checksum = 0;

  // calculate checksum
  for (int i = 0; i < payload_size; i++) {
    temp_checksum += payload[i];
  }

  checksum.checksum_a = (uint8_t)((temp_checksum & 0xFF00) >> 8);
  checksum.checksum_b = (uint8_t)(temp_checksum & 0xFF);

  return checksum;
}

static void __reset_uart(void) {
  iridium_self->uart_driver.deinit();
  iridium_self->uart_driver.init();
  tx_thread_sleep(5);
}

static int32_t __uart_read_dma(void *driver_ptr, uint8_t *read_buf,
                               uint16_t size, uint32_t timeout_ticks) {
  generic_uart_driver *driver_handle = (generic_uart_driver *)driver_ptr;
  // LOG("__uart_read_dma requesting %u bytes", size);

  HAL_StatusTypeDef rx_result;
  if (iridium_self->receive_to_idle) {
    rx_result = HAL_UARTEx_ReceiveToIdle_DMA(driver_handle->uart_handle,
                                             read_buf, size);
    if (HAL_OK != rx_result) {
      LOG("__uart_read_dma HAL_UARTEx_ReceiveToIdle_DMA returned error %d",
          (int)rx_result);
      goto uart_error;
    }
  } else {
    rx_result =
        HAL_UART_Receive_DMA(driver_handle->uart_handle, read_buf, size);
    if (HAL_OK != rx_result) {
      // NOTE(LEL): This is where trying to receive iridium configuration fails;
      // we get rx_result=2, which indicates "busy".
      LOG("__uart_read_dma HAL_UART_Receive_DMA returned error %d",
          (int)rx_result);
      goto uart_error;
    }
  }
  UINT sema_result = tx_semaphore_get(driver_handle->uart_sema, timeout_ticks);
  if (TX_SUCCESS != sema_result) {
    LOG("__uart_read_dma tx_semaphore_get returned error %d", (int)sema_result);
    goto uart_error;
  }

  // LOG("...uart_read_dma finished successfully. %s", read_buf);

  return UART_OK;

uart_error:
  HAL_UART_DMAStop(driver_handle->uart_handle);
  HAL_UART_Abort(driver_handle->uart_handle);
  return UART_ERR;
}

static int32_t __uart_write_dma(void *driver_ptr, uint8_t *write_buf,
                                uint16_t size, uint32_t timeout_ticks) {
  generic_uart_driver *driver_handle = (generic_uart_driver *)driver_ptr;

  // LOG("__uart_write_dma sending %u bytes: %s", size, write_buf);
  if (HAL_UART_Transmit_DMA(driver_handle->uart_handle, write_buf, size) !=
      HAL_OK) {
    goto uart_error;
  }

  if (tx_semaphore_get(driver_handle->uart_sema, timeout_ticks) != TX_SUCCESS) {
    goto uart_error;
  }

  // LOG("...uart_write_dma sent successfully");
  return UART_OK;

uart_error:
  HAL_UART_DMAStop(driver_handle->uart_handle);
  HAL_UART_Abort(driver_handle->uart_handle);
  // LOG("...uart_write_dma failed.");

  return UART_ERR;
}
