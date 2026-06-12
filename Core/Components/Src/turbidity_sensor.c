/*
 * turbidity_sensor.c
 *
 *  Created on: Aug 21, 2024
 *      Author: philbush
 */

#include "turbidity_sensor.h"
#include "app_threadx.h"
#include "ext_rtc_server.h"
#include "gnss.h"
#include "shared_i2c_bus.h"
#include "threadx_support.h"

static Turbidity_Sensor *turbidity_self;

// Struct functions
static uSWIFT_return_code_t _turbidity_sensor_self_test(uint16_t *ambient,
                                                        uint16_t *proximity);
static uSWIFT_return_code_t _turbidity_sensor_setup_sensor(void);
static uSWIFT_return_code_t _turbidity_sensor_take_measurement(bool self_test);
static uSWIFT_return_code_t
_turbidity_sensor_get_most_recent_measurement(uint16_t *ambient,
                                              uint16_t *proximity);
static uSWIFT_return_code_t _turbidity_sensor_process_measurements(void);
static uSWIFT_return_code_t
_turbidity_sensor_start_timer(uint16_t timeout_in_minutes);
static uSWIFT_return_code_t _turbidity_sensor_stop_timer(void);
static void _turbidity_sensor_assemble_telemetry_message_element(
    sbd_message_type_53_element *msg);
static void _turbidity_sensor_standby(void);
static void _turbidity_sensor_idle(void);
static void _turbidity_sensor_on(void);
static void _turbidity_sensor_off(void);
// I/O functions for the sensor
static int32_t _turbidity_sensor_i2c_read(void *unused_handle,
                                          uint16_t bus_address,
                                          uint16_t reg_address,
                                          uint8_t *read_data,
                                          uint16_t data_length);
static int32_t _turbidity_sensor_i2c_write(void *unused_handle,
                                           uint16_t bus_address,
                                           uint16_t reg_address,
                                           uint8_t *write_data,
                                           uint16_t data_length);
static void _turbidity_sensor_ms_delay(uint32_t delay);
// Helper functions
static uSWIFT_return_code_t
__turbidity_sensor_get_proximity_reading(uint16_t *reading);
static uSWIFT_return_code_t
__turbidity_sensor_get_ambient_reading(uint16_t *reading);

void turbidity_sensor_init(Turbidity_Sensor *struct_ptr,
                           microSWIFT_configuration *global_config,
                           TX_TIMER *timer, uint16_t *ambient_buffer,
                           uint16_t *proximity_buffer) {
  turbidity_self = struct_ptr;

  turbidity_self->global_config = global_config;

  turbidity_self->timer = timer;

  turbidity_self->pwr_fet.port = TURBIDITY_FET_GPIO_Port;
  turbidity_self->pwr_fet.pin = TURBIDITY_FET_Pin;

  turbidity_self->ambient_series = ambient_buffer;
  turbidity_self->proximity_series = proximity_buffer;

  memset(&(turbidity_self->ambient_averages_series[0]), 0,
         sizeof(turbidity_self->ambient_averages_series));
  memset(&(turbidity_self->proximity_averages_series[0]), 0,
         sizeof(turbidity_self->proximity_averages_series));

  turbidity_self->samples_counter = 0;

  turbidity_self->timer_timeout = false;

  turbidity_self->self_test = _turbidity_sensor_self_test;
  turbidity_self->setup_sensor = _turbidity_sensor_setup_sensor;
  turbidity_self->take_measurement = _turbidity_sensor_take_measurement;
  turbidity_self->get_most_recent_measurement =
      _turbidity_sensor_get_most_recent_measurement;
  turbidity_self->process_measurements = _turbidity_sensor_process_measurements;
  turbidity_self->start_timer = _turbidity_sensor_start_timer;
  turbidity_self->stop_timer = _turbidity_sensor_stop_timer;
  turbidity_self->assemble_telemetry_message_element =
      _turbidity_sensor_assemble_telemetry_message_element;
  turbidity_self->standby = _turbidity_sensor_standby;
  turbidity_self->idle = _turbidity_sensor_idle;
  turbidity_self->on = _turbidity_sensor_on;
  turbidity_self->off = _turbidity_sensor_off;
}

void turbidity_timer_expired(ULONG expiration_input) {
  turbidity_self->timer_timeout = true;
}

bool turbidity_get_timeout_status(void) {
  return turbidity_self->timer_timeout;
}

void turbidity_reset_sample_counter(void) {
  turbidity_self->samples_counter = 0;
}

static uSWIFT_return_code_t _turbidity_sensor_self_test(uint16_t *ambient,
                                                        uint16_t *proximity) {
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;
  uint8_t id = 0;

  if (vcnl4010_register_io_functions(
          &turbidity_self->dev_ctx, NULL, NULL, _turbidity_sensor_i2c_write,
          _turbidity_sensor_i2c_read,
          _turbidity_sensor_ms_delay) != uSWIFT_SUCCESS) {
    return uSWIFT_INITIALIZATION_ERROR;
  }

  ret = vcnl4010_get_id(&turbidity_self->dev_ctx, &id);
  if ((ret != uSWIFT_SUCCESS) || (id != PROD_ID_REV_REG_RESET_VAL)) {
    return uSWIFT_INITIALIZATION_ERROR;
  }

  ret = turbidity_self->setup_sensor();
  if (ret != uSWIFT_SUCCESS) {
    return uSWIFT_INITIALIZATION_ERROR;
  }

  // Throw out the first measurement
  ret = turbidity_self->take_measurement(true);
  ret |= turbidity_self->take_measurement(true);

  if (ret == uSWIFT_SUCCESS) {
    *ambient = turbidity_self->ambient_series[0];
    *proximity = turbidity_self->proximity_series[0];
  }

  return ret;
}

static uSWIFT_return_code_t _turbidity_sensor_setup_sensor(void) {
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;

  ret |= vcnl4010_auto_offset_comp_config(&turbidity_self->dev_ctx, true);

  ret |= vcnl4010_set_led_current(&turbidity_self->dev_ctx, _50_MA);

  return ret;
}

static uSWIFT_return_code_t _turbidity_sensor_take_measurement(bool self_test) {
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;

  ret |= __turbidity_sensor_get_ambient_reading(
      &turbidity_self->ambient_series[turbidity_self->samples_counter]);
  ret |= __turbidity_sensor_get_proximity_reading(
      &turbidity_self->proximity_series[turbidity_self->samples_counter]);

  if (self_test || (ret != uSWIFT_SUCCESS)) {
    goto done;
  }

  turbidity_self->samples_counter++;

  if (turbidity_self->samples_counter == 1) {
    gnss_get_current_lat_lon(&turbidity_self->start_lat,
                             &turbidity_self->start_lon);
    turbidity_self->start_timestamp = get_system_time();
  }

  if (turbidity_self->samples_counter ==
      turbidity_self->global_config->total_turbidity_samples) {
    gnss_get_current_lat_lon(&turbidity_self->end_lat,
                             &turbidity_self->end_lon);
    turbidity_self->stop_timestamp = get_system_time();
    ret = uSWIFT_DONE_SAMPLING;
  }

  if (turbidity_self->samples_counter % 60 == 0) {
    turbidity_self->process_measurements();
  }

done:
  return ret;
}

static uSWIFT_return_code_t
_turbidity_sensor_get_most_recent_measurement(uint16_t *ambient,
                                              uint16_t *proximity) {
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;

  if (turbidity_self->samples_counter == 0) {
    return uSWIFT_NO_SAMPLES_ERROR;
  }

  *ambient =
      turbidity_self->ambient_series[turbidity_self->samples_counter - 1];
  *proximity =
      turbidity_self->proximity_series[turbidity_self->samples_counter - 1];

  return ret;
}

static uSWIFT_return_code_t _turbidity_sensor_process_measurements(void) {
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;
  static uint32_t averages_index = 0;
  uint32_t ambient_accumulator = 0, proximity_accumulator = 0;

  if (turbidity_self->samples_counter < 60) {
    return uSWIFT_NO_SAMPLES_ERROR;
  }

  for (int i = 59; i >= 0; i--) {
    ambient_accumulator +=
        turbidity_self->ambient_series[turbidity_self->samples_counter - i];
    proximity_accumulator +=
        turbidity_self->proximity_series[turbidity_self->samples_counter - i];
  }

  ambient_accumulator /= 60;
  proximity_accumulator /= 60;

  turbidity_self->ambient_averages_series[averages_index] =
      (uint16_t)ambient_accumulator;
  turbidity_self->proximity_averages_series[averages_index] =
      (uint16_t)proximity_accumulator;

  averages_index++;

  return ret;
}

static uSWIFT_return_code_t
_turbidity_sensor_start_timer(uint16_t timeout_in_minutes) {
  ULONG timeout = TX_TIMER_TICKS_PER_SECOND * 60 * timeout_in_minutes;
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;

  if (tx_timer_change(turbidity_self->timer, timeout, 0) != TX_SUCCESS) {
    ret = uSWIFT_TIMER_ERROR;
    return ret;
  }

  if (tx_timer_activate(turbidity_self->timer) != TX_SUCCESS) {
    ret = uSWIFT_TIMER_ERROR;
  }

  return ret;
}

static uSWIFT_return_code_t _turbidity_sensor_stop_timer(void) {
  return (tx_timer_deactivate(turbidity_self->timer) == TX_SUCCESS)
             ? uSWIFT_SUCCESS
             : uSWIFT_TIMER_ERROR;
}

static void _turbidity_sensor_assemble_telemetry_message_element(
    sbd_message_type_53_element *msg) {
  memcpy(&msg->start_lat, &turbidity_self->start_lat, sizeof(int32_t));
  memcpy(&msg->start_lon, &turbidity_self->start_lon, sizeof(int32_t));
  memcpy(&msg->end_lat, &turbidity_self->end_lat, sizeof(int32_t));
  memcpy(&msg->end_lon, &turbidity_self->end_lon, sizeof(int32_t));
  memcpy(&msg->start_timestamp, &turbidity_self->start_timestamp,
         sizeof(int32_t));
  memcpy(&msg->end_timestamp, &turbidity_self->stop_timestamp, sizeof(int32_t));
  memcpy(&msg->backscatter_avgs, &turbidity_self->proximity_averages_series[0],
         sizeof(msg->backscatter_avgs));
  memcpy(&msg->ambient_avgs, &turbidity_self->ambient_averages_series[0],
         sizeof(msg->backscatter_avgs));
}

static void _turbidity_sensor_standby(void) {
  vcnl4010_set_led_current(&turbidity_self->dev_ctx, _50_MA);
}

static void _turbidity_sensor_idle(void) {
  vcnl4010_set_led_current(&turbidity_self->dev_ctx, _0_MA);
  vcnl4010_cont_conv_config(&turbidity_self->dev_ctx, false);
}

static void _turbidity_sensor_on(void) {
  gpio_write_pin(turbidity_self->pwr_fet, GPIO_PIN_SET);
}

static void _turbidity_sensor_off(void) {
  gpio_write_pin(turbidity_self->pwr_fet, GPIO_PIN_RESET);
}

static int32_t _turbidity_sensor_i2c_read(void *unused_handle,
                                          uint16_t bus_address,
                                          uint16_t reg_address,
                                          uint8_t *read_data,
                                          uint16_t data_length) {
  (void)unused_handle;
  return enqueue_i2c_read(bus_address, reg_address, read_data, data_length,
                          TURBIDITY_REQUEST_PROCESSED);
}

static int32_t _turbidity_sensor_i2c_write(void *unused_handle,
                                           uint16_t bus_address,
                                           uint16_t reg_address,
                                           uint8_t *write_data,
                                           uint16_t data_length) {
  (void)unused_handle;
  return enqueue_i2c_write(bus_address, reg_address, write_data, data_length,
                           TURBIDITY_REQUEST_PROCESSED);
}

static void _turbidity_sensor_ms_delay(uint32_t delay) {
  if (delay == 0) {
    tx_thread_relinquish();
  } else {
    tx_thread_sleep(delay);
  }
}

static uSWIFT_return_code_t
__turbidity_sensor_get_proximity_reading(uint16_t *reading) {
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;
  uint32_t ready_counter = 0, ready_timeout = 10;
  uint32_t proximity = 0;
  bool drdy = false;

  ret = vcnl4010_start_prox_conversion(&turbidity_self->dev_ctx);
  if (ret != uSWIFT_SUCCESS) {
    return ret;
  }

  while ((ready_counter++ < ready_timeout) && !drdy) {
    ret = vcnl4010_get_prox_data_ready(&turbidity_self->dev_ctx, &drdy);
    if (ret != uSWIFT_SUCCESS) {
      return ret;
    }

    tx_thread_sleep(1);
  }

  ret = vcnl4010_get_proximity_reading(&turbidity_self->dev_ctx,
                                       (uint16_t *)&proximity);

  *reading = ((uint16_t)(proximity << 8) | (proximity >> 8));

  return ret;
}

static uSWIFT_return_code_t
__turbidity_sensor_get_ambient_reading(uint16_t *reading) {
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;
  uint32_t ready_counter = 0, ready_timeout = 10;
  uint32_t ambient = 0;
  bool drdy = false;

  ret = vcnl4010_start_ambient_conversion(&turbidity_self->dev_ctx);
  if (ret != uSWIFT_SUCCESS) {
    return ret;
  }

  while ((ready_counter++ < ready_timeout) && !drdy) {
    ret = vcnl4010_get_ambient_data_ready(&turbidity_self->dev_ctx, &drdy);
    if (ret != uSWIFT_SUCCESS) {
      return ret;
    }

    tx_thread_sleep(1);
  }

  ret = vcnl4010_get_ambient_reading(&turbidity_self->dev_ctx,
                                     (uint16_t *)&ambient);

  *reading = ((uint16_t)(ambient << 8) | (ambient >> 8));

  return ret;
}
