/*
 * thread_functions.c
 *
 *  Created on: Aug 20, 2024
 *      Author: philbush
 */

#include "threadx_support.h"
#include "app_threadx.h"
#include "file_system.h"
#include "file_system_server.h"
#include "logger.h"
#include "sdmmc.h"
#include "watchdog.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static time_t sys_time = 0;

bool gnss_apply_config(GNSS *gnss) {
  uSWIFT_return_code_t gnss_return_code = uSWIFT_SUCCESS;
  ULONG start_time = tx_time_get(), max_time = (TX_TIMER_TICKS_PER_SECOND * 58);
  uint32_t fail_counter = 0;

  while ((tx_time_get() - start_time) < max_time) {
    gnss_return_code = gnss->config();

    if (gnss_return_code == uSWIFT_SUCCESS) {
      break;
    }

    if (fail_counter++ % 3 == 0) {
      gnss->off();
      tx_thread_sleep(25 + (rand() % 75));
      gnss->on();
      tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / 2);
    }

    tx_thread_sleep(rand() % (TX_TIMER_TICKS_PER_SECOND / 2));
  }

  return (gnss_return_code == uSWIFT_SUCCESS);
}

bool ct_self_test(CT *ct, bool add_warmup_time, ct_sample *self_test_readings) {
  int32_t fail_counter = 0, max_retries = 25;
  uSWIFT_return_code_t ct_return_code;

  while (fail_counter < max_retries) {
    ct_return_code = ct->self_test(add_warmup_time, self_test_readings);

    if (ct_return_code == uSWIFT_SUCCESS) {
      break;
    }

    ct->off();
    tx_thread_sleep(13);
    ct->on();
    tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / 10);

    fail_counter++;
  }

  return (ct_return_code == uSWIFT_SUCCESS);
}

bool temperature_self_test(Temperature *temperature, float *self_test_temp) {
  int32_t fail_counter = 0, max_retries = 10;
  uSWIFT_return_code_t temp_return_code;

  while (fail_counter < max_retries) {
    temp_return_code = temperature->self_test(self_test_temp);

    if (temp_return_code == uSWIFT_SUCCESS) {
      break;
    }

    tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / 10);

    fail_counter++;
  }

  return (temp_return_code == uSWIFT_SUCCESS);
}

bool turbidity_self_test(Turbidity_Sensor *obs, uint16_t *ambient,
                         uint16_t *proximity) {
  int32_t fail_counter = 0, max_retries = 10;
  uSWIFT_return_code_t ret;

  while (fail_counter < max_retries) {
    ret = obs->self_test(ambient, proximity);

    if (ret == uSWIFT_SUCCESS) {
      break;
    }

    tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / 10);

    fail_counter++;
  }

  return (ret == uSWIFT_SUCCESS);
}

bool light_self_test(Light_Sensor *light) {
  int32_t fail_counter = 0, max_retries = 10;
  uSWIFT_return_code_t light_return_code;

  while (fail_counter < max_retries) {
    light_return_code = light->self_test();

    if (light_return_code == uSWIFT_SUCCESS) {
      break;
    }

    tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / 10);

    fail_counter++;
  }

  return (light_return_code == uSWIFT_SUCCESS);
}

bool iridium_apply_config(Iridium *iridium) {
  int32_t fail_counter = 0, max_retries = 10;
  uSWIFT_return_code_t iridium_return_code;

  if (!iridium_self_test(iridium)) {
    return false;
  }

  // Send the configuration settings to the modem
  fail_counter = 0;
  while (fail_counter < max_retries) {
    iridium_return_code = iridium->config();

    if (iridium_return_code == uSWIFT_SUCCESS) {
      break;
    }

    iridium->cycle_power();
    fail_counter++;
  }

  return (iridium_return_code == uSWIFT_SUCCESS);
}

bool iridium_self_test(Iridium *iridium) {
  int32_t fail_counter = 0, max_retries = 10;
  uSWIFT_return_code_t iridium_return_code;

  while (fail_counter < max_retries) {
    iridium_return_code = iridium->self_test();

    if (iridium_return_code == uSWIFT_SUCCESS) {
      break;
    }

    iridium->cycle_power();
    fail_counter++;
  }

  return (iridium_return_code == uSWIFT_SUCCESS);
}

void gnss_error_out(GNSS *gnss, ULONG error_flag, TX_THREAD *gnss_thread,
                    const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char tmp_fmt[128];

  gnss->off();
  gnss->stop_timer();
  gnss_deinit();

  vsnprintf(&tmp_fmt[0], sizeof(tmp_fmt), fmt, args);
  va_end(args);
  LOG(&(tmp_fmt[0]));

  (void)tx_event_flags_set(&error_flags, error_flag, TX_OR);

  watchdog_deregister_thread(GNSS_THREAD);

  tx_thread_suspend(gnss_thread);
}

void ct_error_out(CT *ct, ULONG error_flag, TX_THREAD *ct_thread,
                  const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char tmp_fmt[128];

  ct->off();
  ct->stop_timer();
  ct_deinit();

  vsnprintf(&tmp_fmt[0], sizeof(tmp_fmt), fmt, args);
  va_end(args);
  LOG(&(tmp_fmt[0]));

  (void)tx_event_flags_set(&error_flags, error_flag, TX_OR);

  watchdog_deregister_thread(CT_THREAD);

  tx_thread_suspend(ct_thread);
}

void temperature_error_out(Temperature *temperature, ULONG error_flag,
                           TX_THREAD *temperature_thread, const char *fmt,
                           ...) {
  va_list args;
  va_start(args, fmt);
  char tmp_fmt[128];

  temperature->stop_timer();

  vsnprintf(&tmp_fmt[0], sizeof(tmp_fmt), fmt, args);
  va_end(args);
  LOG(&(tmp_fmt[0]));

  (void)tx_event_flags_set(&error_flags, error_flag, TX_OR);

  watchdog_deregister_thread(TEMPERATURE_THREAD);

  tx_thread_suspend(temperature_thread);
}

void light_error_out(Light_Sensor *light, ULONG error_flag,
                     TX_THREAD *light_thread, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char tmp_fmt[128];

  light->idle();
  light->stop_timer();

  vsnprintf(&tmp_fmt[0], sizeof(tmp_fmt), fmt, args);
  va_end(args);
  LOG(&(tmp_fmt[0]));

  (void)tx_event_flags_set(&error_flags, error_flag, TX_OR);

  watchdog_deregister_thread(LIGHT_THREAD);

  tx_thread_suspend(light_thread);
}

void turbidity_error_out(Turbidity_Sensor *turbidity, ULONG error_flag,
                         TX_THREAD *turbidity_thread, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char tmp_fmt[128];

  turbidity->idle();
  turbidity->stop_timer();

  vsnprintf(&tmp_fmt[0], sizeof(tmp_fmt), fmt, args);
  va_end(args);
  LOG(&(tmp_fmt[0]));

  (void)tx_event_flags_set(&error_flags, error_flag, TX_OR);

  watchdog_deregister_thread(TURBIDITY_THREAD);

  tx_thread_suspend(turbidity_thread);
}

void accel_error_out(Accelerometer *accel, ULONG error_flag,
                     TX_THREAD *accel_thread, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char tmp_fmt[128];

  accel->uart_deinit();
  accel->power_off();

  vsnprintf(&tmp_fmt[0], sizeof(tmp_fmt), fmt, args);
  va_end(args);
  LOG(&(tmp_fmt[0]));

  (void)tx_event_flags_set(&error_flags, error_flag, TX_OR);
  // NOTE: If we start using a watchdog with the accel thread, would need to
  // call watchdog_deregister_thread(accel_thread) here.

  // NOTE: The other *_error_out functions call "suspend" but these
  // threads are not set up to be resumed after an error; everything will be
  // reset at the next duty cycle.
  tx_thread_terminate(accel_thread);
}

void waves_error_out(ULONG error_flag, TX_THREAD *waves_thread, const char *fmt,
                     ...) {
  va_list args;
  va_start(args, fmt);
  char tmp_fmt[128];

  vsnprintf(&tmp_fmt[0], sizeof(tmp_fmt), fmt, args);
  va_end(args);
  LOG(&(tmp_fmt[0]));

  (void)tx_event_flags_set(&error_flags, error_flag, TX_OR);

  watchdog_deregister_thread(WAVES_THREAD);

  tx_thread_suspend(waves_thread);
}

void iridium_error_out(Iridium *iridium, ULONG error_flag,
                       TX_THREAD *iridium_thread, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char tmp_fmt[128];

  iridium->sleep();
  iridium->off();
  iridium->stop_timer();
  iridium_deinit();

  vsnprintf(&tmp_fmt[0], sizeof(tmp_fmt), fmt, args);
  va_end(args);
  LOG(&(tmp_fmt[0]));

  (void)tx_event_flags_set(&error_flags, error_flag, TX_OR);

  watchdog_deregister_thread(IRIDIUM_THREAD);

  tx_thread_suspend(iridium_thread);
}

void rtc_error_out(TX_THREAD *rtc_thread, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char tmp_fmt[128];

  vsnprintf(&tmp_fmt[0], sizeof(tmp_fmt), fmt, args);
  va_end(args);
  LOG(&(tmp_fmt[0]));

  (void)tx_event_flags_set(&error_flags, RTC_ERROR, TX_OR);

  tx_thread_suspend(rtc_thread);
}

void i2c_error_out(TX_THREAD *i2c_thread, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char tmp_fmt[128];

  vsnprintf(&tmp_fmt[0], sizeof(tmp_fmt), fmt, args);
  va_end(args);
  LOG(&(tmp_fmt[0]));

  (void)tx_event_flags_set(&error_flags, CORE_I2C_BUS_ERROR, TX_OR);

  tx_thread_suspend(i2c_thread);
}

void filex_error_out(TX_THREAD *filex_thread, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char tmp_fmt[128];

  file_system_deinit();

  file_system_server_set_error_status();

  vsnprintf(&tmp_fmt[0], sizeof(tmp_fmt), fmt, args);
  va_end(args);
  LOG(&(tmp_fmt[0]));

  (void)tx_event_flags_set(&error_flags, FILE_SYSTEM_ERROR, TX_OR);

  tx_thread_suspend(filex_thread);
}

ULONG get_gnss_sample_window_timeout(microSWIFT_configuration *config) {
  ULONG sample_window_timeout =
      (((config->gnss_samples_per_window / config->gnss_sampling_rate) / 60) +
       GNSS_WINDOW_BUFFER_TIME);

  return sample_window_timeout;
}

void set_system_time(time_t timestamp) { sys_time = timestamp; }

time_t get_system_time(void) { return sys_time; }

void increment_system_time(void) { sys_time++; }

uint32_t get_next_telemetry_message(uint8_t **msg_buffer,
                                    microSWIFT_configuration *config) {
  uint32_t num_waves_msgs =
      persistent_ram_get_num_msgs_enqueued(WAVES_TELEMETRY);
  uint32_t num_turbidity_msgs =
      persistent_ram_get_num_msgs_enqueued(TURBIDITY_TELEMETRY);
  uint32_t num_light_msgs =
      persistent_ram_get_num_msgs_enqueued(LIGHT_TELEMETRY);
  uint32_t num_accelerometer_msgs =
      persistent_ram_get_num_msgs_enqueued(ACCELEROMETER_TELEMETRY);
  uint32_t msg_type = NO_MESSAGE;
  char legacy_number_7 = '7';
  uint8_t type = 0;
  uint8_t port = 0;
  uint16_t size = 0;

  // If there was an OTA update and the ack message has not yet been sent, then
  // we will send that first.
  if (persistent_ram_get_ota_update_status() &&
      !persistent_ram_get_ota_ack_status()) {
    persistent_ram_get_ota_ack_msg((sbd_message_type_99 *)*msg_buffer);
    return OTA_ACK_MESSAGE;
  }

  // Put simply, we're going to grab from whichever message queue has the most
  // elements, considering which sensors are enabled. In case of a tie, Waves
  // telemetry messages have priority as they produce a full SBD message every
  // sample window.

  if ((num_waves_msgs == 0) && (num_turbidity_msgs == 0) &&
      (num_light_msgs == 0) && (num_accelerometer_msgs == 0)) {
    *msg_buffer = NULL;
    return NO_MESSAGE;
  }

  // First priority is light telemetry as it contains the most elements
  if ((num_light_msgs >= num_waves_msgs) &&
      (num_light_msgs >= num_turbidity_msgs) &&
      (num_light_msgs >= num_accelerometer_msgs)) {
    *msg_buffer =
        persistent_ram_get_prioritized_unsent_message(LIGHT_TELEMETRY);
    msg_type = LIGHT_TELEMETRY;
  }
  // Second priority is turbidity as there are fewer per message than light and
  // more than waves
  else if ((num_turbidity_msgs >= num_waves_msgs) &&
           (num_turbidity_msgs >= num_accelerometer_msgs)) {
    *msg_buffer =
        persistent_ram_get_prioritized_unsent_message(TURBIDITY_TELEMETRY);
    msg_type = TURBIDITY_TELEMETRY;
  } else if ((num_waves_msgs >= num_accelerometer_msgs)) {
    *msg_buffer =
        persistent_ram_get_prioritized_unsent_message(WAVES_TELEMETRY);
    msg_type = WAVES_TELEMETRY;
  }
  // Lowest priority is Accelerometer
  // QUESTION(lindzey): do we want to differentiate between
  //    wake-on-shake and duty-cycle messages?
  else {
    *msg_buffer =
        persistent_ram_get_prioritized_unsent_message(ACCELEROMETER_TELEMETRY);
    msg_type = ACCELEROMETER_TELEMETRY;
  }

  if (*msg_buffer == NULL) {
    msg_type = NO_MESSAGE;
  }

  // Fill in remaining bytes
  switch (msg_type) {
  case WAVES_TELEMETRY:
    // Message is already filled out in Iridium thread, nothing to add
    break;

  case LIGHT_TELEMETRY:

    type = 54;
    size = sizeof(sbd_message_type_54);

    memcpy(&(((sbd_message_type_54 *)*msg_buffer)->legacy_number_7),
           &legacy_number_7, sizeof(char));
    memcpy(&(((sbd_message_type_54 *)*msg_buffer)->type), &type,
           sizeof(uint8_t));
    memcpy(&(((sbd_message_type_54 *)*msg_buffer)->port), &port,
           sizeof(uint8_t));
    memcpy(&(((sbd_message_type_54 *)*msg_buffer)->size), &size,
           sizeof(uint16_t));
    break;

  case TURBIDITY_TELEMETRY:

    type = 53;
    size = sizeof(sbd_message_type_53);

    memcpy(&(((sbd_message_type_53 *)*msg_buffer)->legacy_number_7),
           &legacy_number_7, sizeof(char));
    memcpy(&(((sbd_message_type_53 *)*msg_buffer)->type), &type,
           sizeof(uint8_t));
    memcpy(&(((sbd_message_type_53 *)*msg_buffer)->port), &port,
           sizeof(uint8_t));
    memcpy(&(((sbd_message_type_53 *)*msg_buffer)->size), &size,
           sizeof(uint16_t));
    memcpy(&(((sbd_message_type_53 *)*msg_buffer)->serial_number),
           &config->turbidity_serial_number, sizeof(uint16_t));
    break;

  case ACCELEROMETER_TELEMETRY:
    // Message is already filled out in Iridium thread, nothing to add
    break;

  default:

    msg_type = NO_MESSAGE;
    break;
  }

  return msg_type;
}

bool is_first_sample_window(void) {
  return (persistent_ram_get_sample_window_counter() == 1);
}

ULONG get_current_flags(TX_EVENT_FLAGS_GROUP *event_flags) {
  ULONG current_flags = 0;
  (void)tx_event_flags_get(event_flags, ALL_EVENT_FLAGS, TX_OR, &current_flags,
                           TX_NO_WAIT);
  return current_flags;
}

void clear_event_flags(TX_EVENT_FLAGS_GROUP *event_flags) {
  ULONG current_flags = 0;
  (void)tx_event_flags_get(event_flags, ALL_EVENT_FLAGS, TX_OR_CLEAR,
                           &current_flags, TX_NO_WAIT);
}

ULONG get_timer_remaining_ticks(TX_TIMER *timer) {

  ULONG remaining_ticks;

  if (tx_timer_info_get(timer, TX_NULL, TX_NULL, &remaining_ticks, TX_NULL,
                        TX_NULL) != TX_SUCCESS) {
    return 0;
  }

  return remaining_ticks;
}

uint32_t ticks_from_milliseconds(uint32_t milliseconds) {
  if (milliseconds == 0) {
    return 0;
  }

  return ((uint32_t)(ceil(((float)milliseconds) /
                          ((1.0f / TX_TIMER_TICKS_PER_SECOND) * 1000.0f))));
}
