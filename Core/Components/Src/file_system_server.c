/*
 * file_system_server.c
 *
 *  Created on: Nov 21, 2024
 *      Author: philbush
 */

#include "file_system_server.h"
#include "app_threadx.h"
#include "logger.h"
#include "string.h"

typedef enum {
  SAVE_LOG_LINE_COMPLETE = (ULONG)(1 << SAVE_LOG_LINE),
  SAVE_GNSS_RAW_COMPLETE = (ULONG)(1 << SAVE_GNSS_RAW),
  SAVE_GNSS_BREADCRUMB_TRACK_COMPLETE =
      (ULONG)(1 << SAVE_GNSS_BREADCRUMB_TRACK),
  SAVE_TEMPERATURE_RAW_COMPLETE = (ULONG)(1 << SAVE_TEMPERATURE_RAW),
  SAVE_CT_RAW_COMPLETE = (ULONG)(1 << SAVE_CT_RAW),
  SAVE_LIGHT_RAW_COMPLETE = (ULONG)(1 << SAVE_LIGHT_RAW),
  SAVE_TURBIDITY_RAW_COMPLETE = (ULONG)(1 << SAVE_TURBIDITY_RAW),
  UPDATE_DATE_TIME_COMPLETE = (ULONG)(1 << UPDATE_DATE_TIME)
} file_system_complete_flags;

static file_system_server file_server_self;

static void __internal_manage_request(file_system_request_message *msg,
                                      ULONG operation_wait_time);

void file_system_server_init(TX_QUEUE *request_queue,
                             TX_EVENT_FLAGS_GROUP *complete_flags,
                             microSWIFT_configuration *global_config) {
  file_server_self.request_queue = request_queue;
  file_server_self.complete_flags = complete_flags;
  file_server_self.global_config = global_config;
  file_server_self.sd_error_status = false;
}

void file_system_server_set_error_status(void) {
  file_server_self.sd_error_status = true;
}

void file_system_server_save_log_line(char *log_line) {
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;
  file_system_request_message queue_msg = {0};

  if (file_server_self.sd_error_status) {
    uart_logger_return_line_buf((log_line_buf *)log_line);
    return;
  }

  queue_msg.request = SAVE_LOG_LINE;
  queue_msg.size = strlen(log_line);
  queue_msg.object_pointer = (void *)log_line;
  queue_msg.complete_flag = SAVE_LOG_LINE_COMPLETE;
  queue_msg.return_code = &ret;

  if (tx_queue_send(file_server_self.request_queue, &queue_msg,
                    FILE_SYSTEM_QUEUE_MAX_WAIT_TICKS) != TX_SUCCESS) {
    *queue_msg.return_code = uSWIFT_MESSAGE_QUEUE_ERROR;
  }
}

uSWIFT_return_code_t file_system_server_save_gnss_raw(GNSS *gnss) {
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;
  file_system_request_message queue_msg = {0};

  queue_msg.request = SAVE_GNSS_RAW;
  queue_msg.size = sizeof(float) *
                   file_server_self.global_config->gnss_samples_per_window * 3;
  queue_msg.object_pointer = (void *)gnss;
  queue_msg.complete_flag = SAVE_GNSS_RAW_COMPLETE;
  queue_msg.return_code = &ret;

  __internal_manage_request(&queue_msg,
                            FILE_SYSTEM_GNSS_VELOCITIES_MAX_WAIT_TICKS);

  return ret;
}

uSWIFT_return_code_t file_system_server_save_gnss_track(GNSS *gnss) {
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;
  file_system_request_message queue_msg = {0};

  queue_msg.request = SAVE_GNSS_BREADCRUMB_TRACK;
  queue_msg.size = sizeof(gnss_track_point) * (gnss->breadcrumb_index + 1);
  queue_msg.object_pointer = (void *)gnss;
  queue_msg.complete_flag = SAVE_GNSS_BREADCRUMB_TRACK_COMPLETE;
  queue_msg.return_code = &ret;

  __internal_manage_request(&queue_msg, FILE_SYSTEM_GNSS_TRACK_MAX_WAIT_TICKS);

  return ret;
}

uSWIFT_return_code_t
file_system_server_save_temperature_raw(Temperature *temp) {
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;
  file_system_request_message queue_msg = {0};

  queue_msg.request = SAVE_TEMPERATURE_RAW;
  queue_msg.size = temp->samples_counter * sizeof(float);
  queue_msg.object_pointer = (void *)temp;
  queue_msg.complete_flag = SAVE_TEMPERATURE_RAW_COMPLETE;
  queue_msg.return_code = &ret;

  __internal_manage_request(&queue_msg, FILE_SYSTEM_TEMP_RAW_MAX_WAIT_TICKS);

  return ret;
}

uSWIFT_return_code_t file_system_server_save_ct_raw(CT *ct) {
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;
  file_system_request_message queue_msg = {0};

  queue_msg.request = SAVE_CT_RAW;
  queue_msg.size = ct->total_samples * sizeof(ct_sample);
  queue_msg.object_pointer = (void *)ct;
  queue_msg.complete_flag = SAVE_CT_RAW_COMPLETE;
  queue_msg.return_code = &ret;

  __internal_manage_request(&queue_msg, FILE_SYSTEM_CT_RAW_MAX_WAIT_TICKS);

  return ret;
}

uSWIFT_return_code_t file_system_server_save_light_raw(Light_Sensor *light) {
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;
  file_system_request_message queue_msg = {0};

  queue_msg.request = SAVE_LIGHT_RAW;
  queue_msg.size =
      light->valid_samples * (sizeof(light_basic_counts) - sizeof(uint32_t));
  queue_msg.object_pointer = (void *)light;
  queue_msg.complete_flag = SAVE_LIGHT_RAW_COMPLETE;
  queue_msg.return_code = &ret;

  __internal_manage_request(&queue_msg, FILE_SYSTEM_LIGHT_RAW_MAX_WAIT_TICKS);

  return ret;
}

uSWIFT_return_code_t
file_system_server_save_turbidity_raw(Turbidity_Sensor *obs) {
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;
  file_system_request_message queue_msg = {0};

  queue_msg.request = SAVE_TURBIDITY_RAW;
  queue_msg.size = obs->samples_counter * (sizeof(uint16_t) * 2U);
  queue_msg.object_pointer = (void *)obs;
  queue_msg.complete_flag = SAVE_TURBIDITY_RAW_COMPLETE;
  queue_msg.return_code = &ret;

  __internal_manage_request(&queue_msg,
                            FILE_SYSTEM_TURBIDITY_RAW_MAX_WAIT_TICKS);

  return ret;
}

uSWIFT_return_code_t file_system_server_update_date_time(void) {
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;
  file_system_request_message queue_msg = {0};

  queue_msg.request = UPDATE_DATE_TIME;
  queue_msg.size = 0;
  queue_msg.object_pointer = NULL;
  queue_msg.complete_flag = UPDATE_DATE_TIME_COMPLETE;
  queue_msg.return_code = &ret;

  // Can't wait for this to complete or the system will be in an endless reset
  // loop when the RTC time is set.
  __internal_manage_request(&queue_msg, 0U);

  return ret;
}

static void __internal_manage_request(file_system_request_message *msg,
                                      ULONG operation_wait_time) {
  ULONG event_flags;
  *msg->return_code = uSWIFT_SUCCESS;

  if (tx_queue_send(file_server_self.request_queue, msg,
                    FILE_SYSTEM_QUEUE_MAX_WAIT_TICKS) != TX_SUCCESS) {
    *msg->return_code = uSWIFT_MESSAGE_QUEUE_ERROR;
    return;
  }

  if (tx_event_flags_get(file_server_self.complete_flags, msg->complete_flag,
                         TX_OR_CLEAR, &event_flags,
                         operation_wait_time) != TX_SUCCESS) {
    *msg->return_code = uSWIFT_TIMEOUT;
  }
}
