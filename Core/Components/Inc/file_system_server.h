/*
 * file_system_server.h
 *
 *  Created on: Nov 21, 2024
 *      Author: philbush
 */

#ifndef COMPONENTS_INC_FILE_SYSTEM_SERVER_H_
#define COMPONENTS_INC_FILE_SYSTEM_SERVER_H_

#include "ct_sensor.h"
#include "fx_api.h"
#include "gnss.h"
#include "light_sensor.h"
#include "microSWIFT_return_codes.h"
#include "stddef.h"
#include "temp_sensor.h"
#include "turbidity_sensor.h"
#include "tx_api.h"

// @formatter:off

#define FILE_SYSTEM_QUEUE_LENGTH 32U
#define FILE_SYSTEM_QUEUE_MAX_WAIT_TICKS 5U
// Each file system operation has a different complexity and time requirement
#define FILE_SYSTEM_LOG_LINE_MAX_WAIT_TICKS (TX_TIMER_TICKS_PER_SECOND * 5U)
#define FILE_SYSTEM_GNSS_VELOCITIES_MAX_WAIT_TICKS                             \
  (TX_TIMER_TICKS_PER_SECOND * 20U)
#define FILE_SYSTEM_GNSS_TRACK_MAX_WAIT_TICKS (TX_TIMER_TICKS_PER_SECOND * 30U)
#define FILE_SYSTEM_TEMP_RAW_MAX_WAIT_TICKS (TX_TIMER_TICKS_PER_SECOND * 5U)
#define FILE_SYSTEM_CT_RAW_MAX_WAIT_TICKS (TX_TIMER_TICKS_PER_SECOND * 5U)
#define FILE_SYSTEM_LIGHT_RAW_MAX_WAIT_TICKS (TX_TIMER_TICKS_PER_SECOND * 10U)
#define FILE_SYSTEM_TURBIDITY_RAW_MAX_WAIT_TICKS                               \
  (TX_TIMER_TICKS_PER_SECOND * 10U)

// clang-format off
// Define request types
typedef enum
{
  SAVE_LOG_LINE                 = 0,
  SAVE_GNSS_RAW                 = 1,
  SAVE_GNSS_BREADCRUMB_TRACK    = 2,
  SAVE_TEMPERATURE_RAW          = 3,
  SAVE_CT_RAW                   = 4,
  SAVE_LIGHT_RAW                = 5,
  SAVE_TURBIDITY_RAW            = 6,
  UPDATE_DATE_TIME              = 7
} file_system_request_t;
// clang-format on

typedef struct {
  int32_t request; // use type file_system_request_t
  uint32_t size;
  void *object_pointer;
  ULONG complete_flag;
  uSWIFT_return_code_t *return_code;
} file_system_request_message;

typedef struct {
  TX_QUEUE *request_queue;
  TX_EVENT_FLAGS_GROUP *complete_flags;
  microSWIFT_configuration *global_config;
  bool sd_error_status;
} file_system_server;

void file_system_server_init(TX_QUEUE *request_queue,
                             TX_EVENT_FLAGS_GROUP *complete_flags,
                             microSWIFT_configuration *global_config);
void file_system_server_set_error_status(void);
void file_system_server_save_log_line(char *log_line);
uSWIFT_return_code_t file_system_server_save_gnss_raw(GNSS *gnss);
uSWIFT_return_code_t file_system_server_save_gnss_track(GNSS *gnss);
uSWIFT_return_code_t file_system_server_save_temperature_raw(Temperature *temp);
uSWIFT_return_code_t file_system_server_save_ct_raw(CT *ct);
uSWIFT_return_code_t file_system_server_save_light_raw(Light_Sensor *light);
uSWIFT_return_code_t
file_system_server_save_turbidity_raw(Turbidity_Sensor *obs);
uSWIFT_return_code_t file_system_server_update_date_time(void);

#endif /* COMPONENTS_INC_FILE_SYSTEM_SERVER_H_ */
