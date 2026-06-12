/*
 * light_sensor.c
 *
 *  Created on: Aug 21, 2024
 *      Author: philbush
 */

#include "light_sensor.h"
#include "app_threadx.h"
#include "ext_rtc_server.h"
#include "gnss.h"
#include "gpio.h"
#include "i2c.h"
#include "shared_i2c_bus.h"
#include "threadx_support.h"
#include "tx_api.h"

// @formatter:off
static Light_Sensor *light_self;

// Struct functions
static uSWIFT_return_code_t _light_sensor_self_test(void);
static uSWIFT_return_code_t _light_sensor_setup_sensor(void);
static uSWIFT_return_code_t _light_sensor_read_all_channels(void);
static uSWIFT_return_code_t
_light_sensor_start_timer(uint16_t timeout_in_minutes);
static uSWIFT_return_code_t _light_sensor_stop_timer(void);
static uSWIFT_return_code_t _light_sensor_process_measurements(void);
static uSWIFT_return_code_t _light_sensor_get_samples_averages(void);
static void _light_sensor_assemble_telemetry_message_element(
    sbd_message_type_54_element *msg);
static void _light_sensor_get_raw_measurements(light_raw_counts *buffer);
static void _light_sensor_get_basic_counts(light_basic_counts *buffer);
static void
_light_sensor_get_single_measurement(uint16_t *raw_measurement,
                                     uint32_t *basic_count,
                                     light_channel_index_t which_channel);
static void _light_sensor_standby(void);
static void _light_sensor_idle(void);
static void _light_sensor_on(void);
static void _light_sensor_off(void);

// I/O functions for the sensor
static int32_t _light_sensor_i2c_read(void *unused_handle, uint16_t bus_address,
                                      uint16_t reg_address, uint8_t *read_data,
                                      uint16_t data_length);
static int32_t _light_sensor_i2c_write(void *unused_handle,
                                       uint16_t bus_address,
                                       uint16_t reg_address,
                                       uint8_t *write_data,
                                       uint16_t data_length);
static void _light_sensor_ms_delay(uint32_t delay);
// Helper functions
static void __raw_to_basic_counts(void);
static void __get_mins_maxes(void);

// @formatter:on
/**
 * @brief  Initialize the Light_Sensor struct.
 * @param  struct_ptr:= pointer to Light_Sensor struct
 * @param  timer:= pointer to TX_TIMER instance
 * @param  int_pin_sema:= pointer to semaphore used to evaluate falling edges on
 * Int pin
 * @param  light_Sensor_i2c_sema:= semaphore used to determine TxRx completion
 * status from I2C interrupt transactions
 * @retval Void
 */
void light_sensor_init(Light_Sensor *struct_ptr,
                       microSWIFT_configuration *global_config,
                       light_basic_counts *samples_series_buffer,
                       TX_TIMER *timer) {
  light_self = struct_ptr;

  light_self->global_config = global_config;
  light_self->timer = timer;

  light_self->pwr_fet.port = LIGHT_FET_GPIO_Port;
  light_self->pwr_fet.pin = LIGHT_FET_Pin;

  light_self->smux_assignment_low_channels.adc_assignments[0] = F1;
  light_self->smux_assignment_low_channels.adc_assignments[1] = F2;
  light_self->smux_assignment_low_channels.adc_assignments[2] = F3;
  light_self->smux_assignment_low_channels.adc_assignments[3] = F4;
  light_self->smux_assignment_low_channels.adc_assignments[4] = F5;
  light_self->smux_assignment_low_channels.adc_assignments[5] = F6;

  light_self->smux_assignment_high_channels.adc_assignments[0] = F7;
  light_self->smux_assignment_high_channels.adc_assignments[1] = F8;
  light_self->smux_assignment_high_channels.adc_assignments[2] = NIR;
  light_self->smux_assignment_high_channels.adc_assignments[3] = CLEAR;
  light_self->smux_assignment_high_channels.adc_assignments[4] = DARK;
  light_self->smux_assignment_high_channels.adc_assignments[5] = ADC_DISABLE;

  memset(&(light_self->raw_counts), 0, sizeof(light_self->raw_counts));
  memset(&(light_self->basic_counts), 0, sizeof(light_self->basic_counts));

  light_self->as7341_current_reg_bank = REG_BANK_UNKNOWN;

  light_self->sensor_gain =
      (as7341_again_t)light_self->global_config->light_sensor_gain;

  light_self->timer_timeout = false;

  light_self->valid_samples = 0;
  light_self->failed_samples = 0;
  light_self->samples_series = samples_series_buffer;
  memset(&(light_self->samples_min), 0xFFFFFFFF, sizeof(light_basic_counts));
  memset(&(light_self->samples_max), 0, sizeof(light_basic_counts));
  memset(&(light_self->samples_averages_accumulator), 0,
         sizeof(light_basic_counts));

  light_self->start_lat = 0;
  light_self->start_lon = 0;
  light_self->end_lat = 0;
  light_self->end_lon = 0;
  light_self->start_timestamp = 0;
  light_self->end_timestamp = 0;

  light_self->self_test = _light_sensor_self_test;
  light_self->setup_sensor = _light_sensor_setup_sensor;
  light_self->read_all_channels = _light_sensor_read_all_channels;
  light_self->start_timer = _light_sensor_start_timer;
  light_self->stop_timer = _light_sensor_stop_timer;
  light_self->process_measurements = _light_sensor_process_measurements;
  light_self->get_samples_averages = _light_sensor_get_samples_averages;
  light_self->assemble_telemetry_message_element =
      _light_sensor_assemble_telemetry_message_element;
  light_self->get_raw_measurements = _light_sensor_get_raw_measurements;
  light_self->get_basic_counts = _light_sensor_get_basic_counts;
  light_self->get_single_measurement = _light_sensor_get_single_measurement;
  light_self->standby = _light_sensor_standby;
  light_self->idle = _light_sensor_idle;
  light_self->on = _light_sensor_on;
  light_self->off = _light_sensor_off;
}

/**
 * @brief  Timer timeout callback
 * @param  expiration_input:= Unused input
 * @retval Void
 */
void light_timer_expired(ULONG expiration_input) {
  (void)expiration_input;
  light_self->timer_timeout = true;
}

/**
 * @brief  Get the timout status.
 * @param  Void
 * @retval Bool representing timeout status (true = timeout occurred)
 */
bool light_get_timeout_status(void) { return light_self->timer_timeout; }

/**
 * @brief  Test the light sensor. This involves checking the ID, setting up the
 * sensor, and taking initial measurements.
 * @param  Void
 * @retval uSWIFT_return_code_t indicating success or specific failure type
 */
static uSWIFT_return_code_t _light_sensor_self_test(void) {
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;
  as7341_auxid_reg_t aux_id;
  as7341_revid_reg_t rev_id;
  uint8_t id;

  // Initialize the I/O interface
  if (as7341_register_io_functions(
          &light_self->dev_ctx, NULL, NULL, _light_sensor_i2c_write,
          _light_sensor_i2c_read, _light_sensor_ms_delay) != AS7341_OK) {
    return uSWIFT_IO_ERROR;
  }

  if (as7341_spectral_meas_config(&light_self->dev_ctx, false) != AS7341_OK) {
    return uSWIFT_IO_ERROR;
  }

  tx_thread_sleep(20);

  if (as7341_power(&light_self->dev_ctx, true) != AS7341_OK) {
    return uSWIFT_IO_ERROR;
  }

  light_self->dev_ctx.bus_read(NULL, AS7341_I2C_ADDR, AUXID_REG_ADDR,
                               (uint8_t *)&aux_id, 1);
  light_self->dev_ctx.bus_read(NULL, AS7341_I2C_ADDR, REVID_REG_ADDR,
                               (uint8_t *)&rev_id, 1);

  // Check the chip ID
  if (as7341_get_id(&light_self->dev_ctx, &id) != AS7341_OK) {
    return uSWIFT_IO_ERROR;
  }

  if ((id != AS7341_ID) || (aux_id.auxid != AS7341_AUXID) ||
      (rev_id.rev_id != AS7341_REVID)) {
    return uSWIFT_IO_ERROR;
  }

  ret = light_self->setup_sensor();
  if (ret != uSWIFT_SUCCESS) {
    return ret;
  }

  // Read all the channels
  ret = light_self->read_all_channels();

  return ret;
}

/**
 * @brief  Apply the specific configuration to the sensor
 * @param  Void
 * @retval uSWIFT_return_code_t indicating success or specific failure type
 */
static uSWIFT_return_code_t _light_sensor_setup_sensor(void) {
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;
  as7341_az_config_reg_t az_config;

  // Make sure we're starting with SP_EN bit cleared
  if (as7341_spectral_meas_config(&light_self->dev_ctx, false) != AS7341_OK) {
    return uSWIFT_IO_ERROR;
  }

  // Set ASTEP and ATIME to obtain the integration time from the following
  // formula : t_int = (ATMIE + 1) X (ASTEP + 1) * 2.78us We want an integration
  // time of 182ms, so we'll set ATIME = 0, ASTEP = 65534
  ret = as7341_set_astep(&light_self->dev_ctx, 65534);
  ret |= as7341_set_atime(&light_self->dev_ctx, 0);
  if (ret != uSWIFT_SUCCESS) {
    return ret;
  }

  // Need WTIME to be longer than integration time: integration time = 182ms,
  // WTIME = 2.78ms * (n+1) = 2.78*68 = 189ms
  ret = as7341_set_wait_time(&light_self->dev_ctx, 67);
  ret |= as7341_wait_config(&light_self->dev_ctx, true);
  if (ret != uSWIFT_SUCCESS) {
    return ret;
  }

  // Set the gain
  ret = as7341_set_again(&light_self->dev_ctx, light_self->sensor_gain);
  if (ret != uSWIFT_SUCCESS) {
    return ret;
  }

  // Setup auto-zero to occur every cycle
  az_config.az_nth_iteration.nth_iteration = 1;
  ret =
      as7341_auto_zero_config(&light_self->dev_ctx, az_config.az_nth_iteration);
  if (ret != uSWIFT_SUCCESS) {
    return ret;
  }

  // Set integration mode to SPM (non-synchronized)
  ret = as7341_set_integration_mode(&light_self->dev_ctx, SPM_MODE);
  if (ret != uSWIFT_SUCCESS) {
    return ret;
  }

  // Set low power idle mode
  ret = as7341_low_power_config(&light_self->dev_ctx, true);

  return ret;
}

/**
 * @brief  Read all channels, data will be stored in light_self->raw_counts and
 * light_self->basic_counts.
 * @param  Void
 * @retval uSWIFT_return_code_t indicating success or specific failure type
 */
static uSWIFT_return_code_t _light_sensor_read_all_channels(void) {
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;
  bool data_ready = false;
  bool first_wait = false;
  uint32_t wait_period = 0;
  uint32_t timeout = TX_TIMER_TICKS_PER_SECOND / 4;
  uint32_t start_time = 0;
  uint32_t time_now = 0;

  // Set the SMUX to the lower channels
  if (as7341_config_smux(&light_self->dev_ctx,
                         &(light_self->smux_assignment_low_channels)) !=
      AS7341_OK) {
    ret = uSWIFT_IO_ERROR;
    goto failed;
  }

  // Enable spectral measurements
  if (as7341_spectral_meas_config(&light_self->dev_ctx, true) != AS7341_OK) {
    ret = uSWIFT_IO_ERROR;
    goto failed;
  }

  start_time = tx_time_get();
  time_now = start_time;

  while (!data_ready && ((time_now - start_time) < timeout)) {
    if (first_wait) {
      wait_period = 175;
      first_wait = false;
    } else {
      wait_period = 2;
    }

    tx_thread_sleep(wait_period);

    if (as7341_get_data_ready(&light_self->dev_ctx, &data_ready) != AS7341_OK) {
      ret = uSWIFT_IO_ERROR;
      goto failed;
    }

    time_now = tx_time_get();
  }

  if ((time_now - start_time) >= timeout) {
    ret = uSWIFT_TIMEOUT;
    goto failed;
  }

  if (as7341_get_all_channel_data(&light_self->dev_ctx,
                                  ((as7341_all_channel_data_struct *)&light_self
                                       ->raw_counts.f1_chan)) != AS7341_OK) {
    ret = uSWIFT_IO_ERROR;
    goto failed;
  }

  if (as7341_spectral_meas_config(&light_self->dev_ctx, false) != AS7341_OK) {
    ret = uSWIFT_IO_ERROR;
    goto failed;
  }

  // Set the SMUX to the upper channels and repeat process
  if (as7341_config_smux(&light_self->dev_ctx,
                         &(light_self->smux_assignment_high_channels)) !=
      AS7341_OK) {
    ret = uSWIFT_IO_ERROR;
    goto failed;
  }

  if (as7341_spectral_meas_config(&light_self->dev_ctx, true) != AS7341_OK) {
    ret = uSWIFT_IO_ERROR;
    goto failed;
  }

  data_ready = false;
  first_wait = false;
  start_time = tx_time_get();
  time_now = start_time;

  while (!data_ready && ((time_now - start_time) < timeout)) {
    if (first_wait) {
      wait_period = 175;
      first_wait = false;
    } else {
      wait_period = 2;
    }

    tx_thread_sleep(wait_period);

    if (as7341_get_data_ready(&light_self->dev_ctx, &data_ready) != AS7341_OK) {
      return uSWIFT_IO_ERROR;
    }
  }

  if ((time_now - start_time) >= timeout) {
    ret = uSWIFT_TIMEOUT;
    goto failed;
  }

  if (as7341_get_all_channel_data(&light_self->dev_ctx,
                                  ((as7341_all_channel_data_struct *)&light_self
                                       ->raw_counts.f7_chan)) != AS7341_OK) {
    ret = uSWIFT_IO_ERROR;
    goto failed;
  }

  if (as7341_spectral_meas_config(&light_self->dev_ctx, false) != AS7341_OK) {
    // NOTE(LEL): This error flag is ignored, but I think it's OK because both
    //   self_test and read_all_samples start by trying to set this flag, and
    //   if it fails then, the functions will report failure. I think this is
    //   probably set here as a good idea, but not required.
    ret = uSWIFT_IO_ERROR;
  }

  // Convert everything to basic counts
  __raw_to_basic_counts();

  return uSWIFT_SUCCESS;

failed:
  (void)as7341_spectral_meas_config(&light_self->dev_ctx, false);
  return ret;
}

/**
 * @brief  Start the thread timer.
 * @param  timeout_in_minutes:= timeout, in minutes
 * @retval uSWIFT_return_code_t indicating success or specific failure type
 */
static uSWIFT_return_code_t
_light_sensor_start_timer(uint16_t timeout_in_minutes) {
  ULONG timeout = TX_TIMER_TICKS_PER_SECOND * 60 * timeout_in_minutes;
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;

  if (tx_timer_change(light_self->timer, timeout, 0) != TX_SUCCESS) {
    ret = uSWIFT_TIMER_ERROR;
    return ret;
  }

  if (tx_timer_activate(light_self->timer) != TX_SUCCESS) {
    ret = uSWIFT_TIMER_ERROR;
  }

  return ret;
}

/**
 * @brief  Stop the thread timer.
 * @param  void
 * @retval uSWIFT_return_code_t indicating success or specific failure type
 */
static uSWIFT_return_code_t _light_sensor_stop_timer(void) {
  return (tx_timer_deactivate(light_self->timer) == TX_SUCCESS)
             ? uSWIFT_SUCCESS
             : uSWIFT_TIMER_ERROR;
}

/**
 * @brief  Process the last round of measurements
 * @param  void
 * @retval uSWIFT_return_code_t indicating success or specific failure type
 */
static uSWIFT_return_code_t _light_sensor_process_measurements(void) {
  // Store the most recent samples in the time series; this buffer
  // is saved to disk at the end of the sampling period.
  memcpy(&(light_self->samples_series[light_self->valid_samples]),
         &light_self->basic_counts, sizeof(light_basic_counts));
  light_self->valid_samples++;

  // Update min/max for each channel
  __get_mins_maxes();

  // update the accumulator.
  uint32_t *basic_count_ptr = &light_self->basic_counts.f1_chan;
  uint32_t *averages_ptr = &light_self->samples_averages_accumulator.f1_chan;
  for (int i = 0; i < 12; i++, basic_count_ptr++, averages_ptr++) {
    *averages_ptr += *basic_count_ptr;
  }

  if (light_self->valid_samples == 1) {
    gnss_get_current_lat_lon(&light_self->start_lat, &light_self->start_lon);
    light_self->start_timestamp = get_system_time();
  } else {
    // Fill in timestamp + lat/lon data every time, because if the last
    // sampling attempt fails this function won't be called. If we only
    // fill these values in when sampling is complete, there's the potential
    // that they will be from significantly later than the last successful
    // sample.
    gnss_get_current_lat_lon(&light_self->end_lat, &light_self->end_lon);
    light_self->end_timestamp = get_system_time();
  }

  return uSWIFT_SUCCESS;
}

/**
 * @brief  Average the samples.
 * @param  void
 * @retval uSWIFT_return_code_t indicating success or specific failure type
 */
static uSWIFT_return_code_t _light_sensor_get_samples_averages(void) {
  uint32_t *averages_ptr = &light_self->samples_averages_accumulator.f1_chan;

  if (light_self->valid_samples == 0) {
    return uSWIFT_PARAMETERS_INVALID;
  }

  for (int i = 0; i < 12; i++, averages_ptr++) {
    *averages_ptr /= light_self->valid_samples;
  }

  return uSWIFT_SUCCESS;
}

static void _light_sensor_assemble_telemetry_message_element(
    sbd_message_type_54_element *msg) {
  // Packed elements necessitate using memcpy, lest a crash
  memcpy(&msg->start_lat, &light_self->start_lat, sizeof(int32_t));
  memcpy(&msg->start_lon, &light_self->start_lon, sizeof(int32_t));
  memcpy(&msg->end_lat, &light_self->end_lat, sizeof(int32_t));
  memcpy(&msg->end_lon, &light_self->end_lon, sizeof(int32_t));
  memcpy(&msg->start_timestamp, &light_self->start_timestamp, sizeof(uint32_t));
  memcpy(&msg->end_timestamp, &light_self->end_timestamp, sizeof(uint32_t));
  memcpy(&msg->valid_samples, &light_self->valid_samples, sizeof(uint16_t));
  memcpy(&msg->failed_samples, &light_self->failed_samples, sizeof(uint16_t));
  memcpy(&msg->max_reading_clear, &light_self->samples_max.clear_chan,
         sizeof(uint16_t));
  memcpy(&msg->min_reading_clear, &light_self->samples_min.clear_chan,
         sizeof(uint16_t));
  memcpy(&msg->avg_clear, &light_self->samples_averages_accumulator.clear_chan,
         sizeof(uint16_t));
  memcpy(&msg->avg_f1, &light_self->samples_averages_accumulator.f1_chan,
         sizeof(uint16_t));
  memcpy(&msg->avg_f2, &light_self->samples_averages_accumulator.f2_chan,
         sizeof(uint16_t));
  memcpy(&msg->avg_f3, &light_self->samples_averages_accumulator.f3_chan,
         sizeof(uint16_t));
  memcpy(&msg->avg_f4, &light_self->samples_averages_accumulator.f4_chan,
         sizeof(uint16_t));
  memcpy(&msg->avg_f5, &light_self->samples_averages_accumulator.f5_chan,
         sizeof(uint16_t));
  memcpy(&msg->avg_f6, &light_self->samples_averages_accumulator.f6_chan,
         sizeof(uint16_t));
  memcpy(&msg->avg_f7, &light_self->samples_averages_accumulator.f7_chan,
         sizeof(uint16_t));
  memcpy(&msg->avg_f8, &light_self->samples_averages_accumulator.f8_chan,
         sizeof(uint16_t));
  memcpy(&msg->avg_dark, &light_self->samples_averages_accumulator.dark_chan,
         sizeof(uint16_t));
  memcpy(&msg->avg_nir, &light_self->samples_averages_accumulator.nir_chan,
         sizeof(uint16_t));
}

static void _light_sensor_get_raw_measurements(light_raw_counts *buffer) {
  memcpy(buffer, &(light_self->raw_counts), sizeof(light_raw_counts));
}

static void _light_sensor_get_basic_counts(light_basic_counts *buffer) {
  memcpy(buffer, &(light_self->basic_counts), sizeof(light_basic_counts));
}

static void
_light_sensor_get_single_measurement(uint16_t *raw_measurement,
                                     uint32_t *basic_count,
                                     light_channel_index_t which_channel) {
  uint16_t *raw = &light_self->raw_counts.f1_chan;
  uint32_t *basic = &light_self->basic_counts.f1_chan;

  if ((which_channel < 0) || (which_channel > DARK_CHANNEL)) {
    *raw_measurement = 0;
    *basic_count = 0;
    return;
  }

  *raw_measurement = *(raw + which_channel);
  *basic_count = *(basic + which_channel);
}

static void _light_sensor_standby(void) {
  as7341_power(&light_self->dev_ctx, true);
}

static void _light_sensor_idle(void) {
  as7341_spectral_meas_config(&light_self->dev_ctx, false);
  as7341_power(&light_self->dev_ctx, false);
}

static void _light_sensor_on(void) {
  gpio_write_pin(light_self->pwr_fet, GPIO_PIN_SET);
}

static void _light_sensor_off(void) {
  gpio_write_pin(light_self->pwr_fet, GPIO_PIN_RESET);
}

static int32_t _light_sensor_i2c_read(void *unused_handle, uint16_t bus_address,
                                      uint16_t reg_address, uint8_t *read_data,
                                      uint16_t data_length) {
  (void)unused_handle;
  int32_t ret = AS7341_OK;

  // Why could they possibly need to put a bit in there for different register
  // banks? This could so easily be handled in the asic
  if (((reg_address >= 0x64) && (reg_address < 0x80)) &&
      (reg_address != CFG0_REG_ADDR) &&
      ((light_self->as7341_current_reg_bank == REG_BANK_80_PLUS) ||
       (light_self->as7341_current_reg_bank == REG_BANK_UNKNOWN))) {
    if (as7341_set_register_bank(&light_self->dev_ctx, REG_BANK_60_74) !=
        AS7341_OK) {
      light_self->as7341_current_reg_bank = REG_BANK_UNKNOWN;
      return AS7341_ERROR;
    }

    light_self->as7341_current_reg_bank = REG_BANK_60_74;
  } else if (((reg_address >= 0x80) || (reg_address < 0x64)) &&
             (reg_address != CFG0_REG_ADDR) &&
             ((light_self->as7341_current_reg_bank == REG_BANK_60_74) ||
              (light_self->as7341_current_reg_bank == REG_BANK_UNKNOWN))) {
    if (as7341_set_register_bank(&light_self->dev_ctx, REG_BANK_80_PLUS) !=
        AS7341_OK) {
      light_self->as7341_current_reg_bank = REG_BANK_UNKNOWN;
      return AS7341_ERROR;
    }

    light_self->as7341_current_reg_bank = REG_BANK_80_PLUS;
  }

  ret = enqueue_i2c_read(bus_address, reg_address, read_data, data_length,
                         LIGHT_REQUEST_PROCESSED);

  return ret;
}

static int32_t _light_sensor_i2c_write(void *unused_handle,
                                       uint16_t bus_address,
                                       uint16_t reg_address,
                                       uint8_t *write_data,
                                       uint16_t data_length) {
  (void)unused_handle;
  int32_t ret = AS7341_OK;

  // Why could they possibly need to put a bit in there for different register
  // banks? This could so easily be handled in the asic
  if (((reg_address >= 0x64) && (reg_address < 0x80)) &&
      (reg_address != CFG0_REG_ADDR) &&
      ((light_self->as7341_current_reg_bank == REG_BANK_80_PLUS) ||
       (light_self->as7341_current_reg_bank == REG_BANK_UNKNOWN))) {
    if (as7341_set_register_bank(&light_self->dev_ctx, REG_BANK_60_74) !=
        AS7341_OK) {
      light_self->as7341_current_reg_bank = REG_BANK_UNKNOWN;
      return AS7341_ERROR;
    }

    light_self->as7341_current_reg_bank = REG_BANK_60_74;
  } else if (((reg_address >= 0x80) || (reg_address < 0x64)) &&
             (reg_address != CFG0_REG_ADDR) &&
             ((light_self->as7341_current_reg_bank == REG_BANK_60_74) ||
              (light_self->as7341_current_reg_bank == REG_BANK_UNKNOWN))) {
    if (as7341_set_register_bank(&light_self->dev_ctx, REG_BANK_80_PLUS) !=
        AS7341_OK) {
      light_self->as7341_current_reg_bank = REG_BANK_UNKNOWN;
      return AS7341_ERROR;
    }

    light_self->as7341_current_reg_bank = REG_BANK_80_PLUS;
  }

  ret = enqueue_i2c_write(bus_address, reg_address, write_data, data_length,
                          LIGHT_REQUEST_PROCESSED);

  return ret;
}

static void _light_sensor_ms_delay(uint32_t delay)

{
  if (delay == 0) {
    tx_thread_relinquish();
  } else {
    tx_thread_sleep(delay);
  }
}

static void __raw_to_basic_counts(void) {
  uint16_t *raw_cnt_ptr = &(light_self->raw_counts.f1_chan);
  uint32_t *basic_cnt_ptr = &(light_self->basic_counts.f1_chan);

  if (light_self->sensor_gain == GAIN_0_5X) {
    for (int i = 0; i < 12; i++, raw_cnt_ptr++, basic_cnt_ptr++) {
      *basic_cnt_ptr = *raw_cnt_ptr >> 1;
    }
  } else {
    for (int i = 0; i < 12; i++, raw_cnt_ptr++, basic_cnt_ptr++) {
      *basic_cnt_ptr = *raw_cnt_ptr << (light_self->sensor_gain - 1);
    }
  }
}

static void __get_mins_maxes(void) {
  uint32_t *basic_count, *min, *max;
  basic_count = &light_self->basic_counts.f1_chan;
  min = &light_self->samples_min.f1_chan;
  max = &light_self->samples_max.f1_chan;

  for (int i = 0; i < 12; i++, basic_count++, min++, max++) {
    if (*basic_count < *min) {
      *min = *basic_count;
    }

    if (*basic_count > *max) {
      *max = *basic_count;
    }
  }
}
