/*
 * thread_functions.h
 *
 *  Created on: Aug 20, 2024
 *      Author: philbush
 */

#ifndef INC_THREADX_SUPPORT_H_
#define INC_THREADX_SUPPORT_H_

#include "accelerometer.h"
#include "ct_sensor.h"
#include "gnss.h"
#include "iridium.h"
#include "light_sensor.h"
#include "main.h"
#include "math.h"
#include "persistent_ram.h"
#include "sbd.h"
#include "stdarg.h"
#include "stdbool.h"
#include "temp_sensor.h"
#include "turbidity_sensor.h"
#include "tx_api.h"

#define GET_QUEUE_MSG_ELEMENT_SIZE(msg_element_size)                           \
  ((uint32_t)ceil(((double)msg_element_size) / ((double)sizeof(uint32_t))))
#define GET_QUEUE_BUFFER_SIZE(msg_element_size, queue_element_length)          \
  (GET_QUEUE_MSG_ELEMENT_SIZE(msg_element_size) * sizeof(uint32_t) *           \
   queue_element_length)

#define ALL_EVENT_FLAGS (0xFFFFFFFF)
#define NO_ERROR_FLAG 0U

bool gnss_apply_config(GNSS *gnss);
bool ct_self_test(CT *ct, bool add_warmup_time, ct_sample *self_test_readings);
bool temperature_self_test(Temperature *temperature, float *self_test_temp);
bool turbidity_self_test(Turbidity_Sensor *obs, uint16_t *ambient,
                         uint16_t *proximity);
bool light_self_test(Light_Sensor *light);
bool iridium_apply_config(Iridium *iridium);
bool iridium_self_test(Iridium *iridium);

void gnss_error_out(GNSS *gnss, ULONG error_flag, TX_THREAD *gnss_thread,
                    const char *fmt, ...);
void ct_error_out(CT *ct, ULONG error_flag, TX_THREAD *ct_thread,
                  const char *fmt, ...);
void temperature_error_out(Temperature *temperature, ULONG error_flag,
                           TX_THREAD *temperature_thread, const char *fmt, ...);
void light_error_out(Light_Sensor *light, ULONG error_flag,
                     TX_THREAD *light_thread, const char *fmt, ...);
void turbidity_error_out(Turbidity_Sensor *turbidity, ULONG error_flag,
                         TX_THREAD *turbidity_thread, const char *fmt, ...);
void accel_error_out(Accelerometer *accel, ULONG error_flag,
                     TX_THREAD *accel_thread, const char *fmt, ...);
void waves_error_out(ULONG error_flag, TX_THREAD *waves_thread, const char *fmt,
                     ...);
void iridium_error_out(Iridium *iridium, ULONG error_flag,
                       TX_THREAD *iridium_thread, const char *fmt, ...);
void rtc_error_out(TX_THREAD *rtc_thread, const char *fmt, ...);
void i2c_error_out(TX_THREAD *i2c_thread, const char *fmt, ...);
void filex_error_out(TX_THREAD *filex_thread, const char *fmt, ...);

ULONG get_gnss_acquisition_timeout(microSWIFT_configuration *config);
ULONG get_gnss_sample_window_timeout(microSWIFT_configuration *config);

void set_system_time(time_t timestamp);
time_t get_system_time(void);
void increment_system_time(void);

uint32_t get_next_telemetry_message(uint8_t **msg_buffer,
                                    microSWIFT_configuration *config);

bool is_first_sample_window(void);

ULONG get_current_flags(TX_EVENT_FLAGS_GROUP *event_flags);
void clear_event_flags(TX_EVENT_FLAGS_GROUP *event_flags);

ULONG get_timer_remaining_ticks(TX_TIMER *timer);

uint32_t ticks_from_milliseconds(uint32_t milliseconds);

#endif /* INC_THREADX_SUPPORT_H_ */
