/*
 * ext_rtc.h
 *
 *  Created on: Jul 29, 2024
 *      Author: philbush
 */

#ifndef COMPONENTS_INC_EXT_RTC_H_
#define COMPONENTS_INC_EXT_RTC_H_

#include "gpio.h"
#include "microSWIFT_return_codes.h"
#include "spi.h"
#include "time.h"
#include "tx_api.h"
#include <ext_rtc_server.h>

//@formatter:off

#define WATCHDOG_PERIOD 75000
#define RTC_SPI_TIMEOUT 25
#define RTC_SPI_BUF_SIZE 32
#define RTC_WATCHDOG_MIN_REFRESH 10000
#define SECONDS_TO_MILLISECONDS(x) (x * 1000)
#define MILLISECONDS_TO_SECONDS(x) (x / 1000)

typedef struct {
  dev_ctx_t dev_ctx;

  SPI_HandleTypeDef *rtc_spi_bus;

  TX_SEMAPHORE *spi_sema;

  gpio_pin_struct int_b_pin;
  gpio_pin_struct watchdog_or_gate_input;
  gpio_pin_struct ts_pins[NUMBER_OF_TIMESTAMPS];

  bool ts_in_use[NUMBER_OF_TIMESTAMPS];

  uint8_t watchdog_refresh_time_val;

  pcf2131_irq_config_struct irq_config;

  uSWIFT_return_code_t (*setup_rtc)(void);
  uSWIFT_return_code_t (*refresh_watchdog)(void);
  uSWIFT_return_code_t (*set_date_time)(struct tm *input_date_time);
  uSWIFT_return_code_t (*get_date_time)(struct tm *return_date_time);
  uSWIFT_return_code_t (*set_timestamp)(pcf2131_timestamp_t which_timestamp);
  uSWIFT_return_code_t (*get_timestamp)(pcf2131_timestamp_t which_timestamp,
                                        struct tm *return_timestamp);
  uSWIFT_return_code_t (*set_alarm)(rtc_alarm_struct alarm_setting);
  uSWIFT_return_code_t (*clear_flag)(rtc_flag_t which_flag);
} Ext_RTC;

uSWIFT_return_code_t ext_rtc_init(Ext_RTC *struct_ptr,
                                  SPI_HandleTypeDef *rtc_spi_bus,
                                  TX_SEMAPHORE *rtc_spi_sema);

//@formatter:on
#endif /* COMPONENTS_INC_EXT_RTC_H_ */
