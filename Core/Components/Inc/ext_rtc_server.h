/*
 * ext_rtc_api.h
 *
 *  Created on: Aug 6, 2024
 *      Author: philbush
 */

#ifndef COMPONENTS_INC_EXT_RTC_SERVER_H_
#define COMPONENTS_INC_EXT_RTC_SERVER_H_

#include "stddef.h"
#include "time.h"
#include "pcf2131_reg.h"
#include "limits.h"
#include "tx_api.h"
#include "microSWIFT_return_codes.h"

// @formatter:off

#define RTC_QUEUE_LENGTH            16U
#define RTC_QUEUE_MAX_WAIT_TICKS    2U
#define RTC_FLAG_MAX_WAIT_TICKS     10U

// Which request function does a requester want performed
typedef enum
{
  REFRESH_WATCHDOG  = 0,
  GET_TIME          = 1,
  SET_TIME          = 2,
  SET_TIMESTAMP     = 3,
  GET_TIMESTAMP     = 4,
  GET_SYS_TIME      = 5,
  SET_ALARM         = 6,
  CLEAR_FLAG        = 7
} rtc_request_t;

typedef enum
{
  MINUTE_SECOND_FLAG        = 0,
  WATCHDOG_FLAG             = 1,
  ALARM_FLAG                = 2,
  BATTERY_SWITCHOVER_FLAG   = 3,
  BATTERY_STATUS_FLAG       = 4,
  TIMESTAMP1_FLAG           = 5,
  TIMESTAMP2_FLAG           = 6,
  TIMESTAMP3_FLAG           = 7,
  TIMESTAMP4_FLAG           = 8,
  ALL_RTC_FLAGS             = 9
} rtc_flag_t;

// Message struct for GET_TIME, request
typedef struct
{
  struct tm *time_struct;
} rtc_get_time_t;

// Message struct for SET_TIME request
typedef struct
{
  struct tm *time_struct;
} rtc_set_time_t;

// Message struct for SET_TIMESTAMP request
typedef struct
{
  pcf2131_timestamp_t   which_timestamp;
} rtc_set_timestamp_t;

// Message struct for GET_TIMESTAMP request
typedef struct
{
  pcf2131_timestamp_t   which_timestamp;
  struct tm             *timestamp;
} rtc_get_timestamp_t;

// Generic message type for request -- used to create uniform request size
typedef union
{
  rtc_get_time_t        get_set_time;
  rtc_set_timestamp_t   set_timestamp;
  rtc_get_timestamp_t   get_timestamp;
  rtc_alarm_struct      set_alarm;
  uint32_t              clear_flag;
} rtc_data_t;

// Generic message request format
typedef struct
{
  int32_t               request; // Use type rtc_request_t

  // For set requests, this contains input data
  // For get requests, this is written back to
  rtc_data_t            input_output_struct;
  ULONG                 complete_flag;
  uSWIFT_return_code_t  *return_code;
} rtc_request_message;

typedef struct
{
  TX_QUEUE              *request_queue;
  TX_EVENT_FLAGS_GROUP  *complete_flags;
} rtc_server;

// Interface functions
void                 rtc_server_init ( TX_QUEUE *request_queue, TX_EVENT_FLAGS_GROUP *complete_flags );
void                 rtc_server_refresh_watchdog ( void );
uSWIFT_return_code_t rtc_server_get_time ( struct tm *return_time_struct, ULONG complete_flag );
uSWIFT_return_code_t rtc_server_set_time ( struct tm *input_time_struct, ULONG complete_flag );
uSWIFT_return_code_t rtc_server_set_timestamp ( pcf2131_timestamp_t which_timestamp, ULONG complete_flag );
uSWIFT_return_code_t rtc_server_get_timestamp ( pcf2131_timestamp_t which_timestamp, struct tm* return_timestamp,
                                                ULONG complete_flag );
uSWIFT_return_code_t rtc_server_set_alarm ( rtc_alarm_struct alarm_settings, ULONG complete_flag );
uSWIFT_return_code_t rtc_server_clear_flag (rtc_flag_t which_flag, ULONG complete_flag );
// Generic do-all function
uSWIFT_return_code_t rtc_server_process_request ( rtc_request_message *request );

/* Helper functions */
void                 struct_tm_dec_to_bcd ( struct tm *struct_ptr );
void                 struct_tm_bcd_to_dec ( struct tm *struct_ptr );

// @formatter:on

#endif /* COMPONENTS_INC_EXT_RTC_SERVER_H_ */
