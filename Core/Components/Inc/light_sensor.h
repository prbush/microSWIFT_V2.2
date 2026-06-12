/*
 * light_sensor.h
 *
 *  Created on: Aug 21, 2024
 *      Author: philbush
 */

#ifndef COMPONENTS_INC_LIGHT_SENSOR_H_
#define COMPONENTS_INC_LIGHT_SENSOR_H_

#include <stdint.h>

#include "as7341_reg.h"
#include "configuration.h"
#include "gpio.h"
#include "i2c.h"
#include "microSWIFT_return_codes.h"
#include "sbd.h"
#include "tx_api.h"

#define LIGHT_I2C_BUF_SIZE 32
#define LIGHT_I2C_TIMEOUT 50
#define NUM_LIGHT_CHANNELS 11
#define INTEGRATION_TIME_MS 50
#define LIGHT_SENSOR_BYTE_POOL_BUFFER_SIZE                                     \
  86400 // max size = 60 mins of qty 12 4-byte measurements @ 0.5Hz

// @formatter:off

typedef enum {
  F1_CHANNEL = 0,
  F2_CHANNEL = 1,
  F3_CHANNEL = 2,
  F4_CHANNEL = 3,
  F5_CHANNEL = 4,
  F6_CHANNEL = 5,
  F7_CHANNEL = 6,
  F8_CHANNEL = 7,
  NIR_CHANNEL = 8,
  CLEAR_CHANNEL = 9,
  DARK_CHANNEL = 10
} light_channel_index_t;

typedef struct {
  uint16_t f1_chan;
  uint16_t f2_chan;
  uint16_t f3_chan;
  uint16_t f4_chan;
  uint16_t f5_chan;
  uint16_t f6_chan;
  uint16_t f7_chan;
  uint16_t f8_chan;
  uint16_t nir_chan;
  uint16_t clear_chan;
  uint16_t dark_chan;
  uint16_t spare_chan;
} light_raw_counts;

typedef struct {
  uint32_t f1_chan;
  uint32_t f2_chan;
  uint32_t f3_chan;
  uint32_t f4_chan;
  uint32_t f5_chan;
  uint32_t f6_chan;
  uint32_t f7_chan;
  uint32_t f8_chan;
  uint32_t nir_chan;
  uint32_t clear_chan;
  uint32_t dark_chan;
  uint32_t spare_chan;
} light_basic_counts;

typedef struct {
  microSWIFT_configuration *global_config;

  TX_TIMER *timer;
  bool timer_timeout;

  gpio_pin_struct pwr_fet;

  dev_ctx_t dev_ctx;
  as7341_smux_assignment smux_assignment_low_channels;
  as7341_smux_assignment smux_assignment_high_channels;
  light_raw_counts raw_counts;
  light_basic_counts basic_counts;
  int32_t as7341_current_reg_bank;
  as7341_again_t sensor_gain;

  uint16_t failed_samples;
  uint16_t valid_samples;
  light_basic_counts samples_max;
  light_basic_counts samples_min;
  light_basic_counts samples_averages_accumulator;
  light_basic_counts *samples_series;
  int32_t start_lat;
  int32_t start_lon;
  int32_t end_lat;
  int32_t end_lon;
  time_t start_timestamp;
  time_t end_timestamp;

  uSWIFT_return_code_t (*self_test)(void);
  uSWIFT_return_code_t (*setup_sensor)(void);
  uSWIFT_return_code_t (*read_all_channels)(void);
  uSWIFT_return_code_t (*start_timer)(uint16_t timeout_in_minutes);
  uSWIFT_return_code_t (*stop_timer)(void);
  uSWIFT_return_code_t (*process_measurements)(void);
  uSWIFT_return_code_t (*get_samples_averages)(void);
  void (*assemble_telemetry_message_element)(sbd_message_type_54_element *msg);
  void (*get_raw_measurements)(light_raw_counts *buffer);
  void (*get_basic_counts)(light_basic_counts *buffer);
  void (*get_single_measurement)(uint16_t *raw_measurement,
                                 uint32_t *basic_count,
                                 light_channel_index_t which_channel);
  void (*standby)(void);
  void (*idle)(void);
  void (*on)(void);
  void (*off)(void);

} Light_Sensor;

void light_sensor_init(Light_Sensor *struct_ptr,
                       microSWIFT_configuration *global_config,
                       light_basic_counts *samples_series_buffer,
                       TX_TIMER *timer);
void light_timer_expired(ULONG expiration_input);
bool light_get_timeout_status(void);

// @formatter:on
#endif /* COMPONENTS_INC_LIGHT_SENSOR_H_ */
