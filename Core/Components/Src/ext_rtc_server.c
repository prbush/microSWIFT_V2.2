/*
 * ext_rtc_api.c
 *
 *  Created on: Aug 9, 2024
 *      Author: philbush
 */
// clang-format off

#include <ext_rtc_server.h>

static rtc_server rtc_server_self;

void rtc_server_init ( TX_QUEUE *request_queue, TX_EVENT_FLAGS_GROUP *complete_flags )
{
  rtc_server_self.request_queue = request_queue;
  rtc_server_self.complete_flags = complete_flags;
}

void rtc_server_refresh_watchdog ( void )
{
  rtc_request_message queue_msg =
    { 0 };

  queue_msg.request = REFRESH_WATCHDOG;
  queue_msg.complete_flag = 0;
  queue_msg.return_code = NULL;

  (void) tx_queue_send (rtc_server_self.request_queue, &queue_msg,
  RTC_QUEUE_MAX_WAIT_TICKS);
}

uSWIFT_return_code_t rtc_server_get_time ( struct tm *return_time_struct, ULONG complete_flag )
{
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;
  rtc_request_message queue_msg =
    { 0 };
  ULONG event_flags;

  queue_msg.request = GET_TIME;
  queue_msg.input_output_struct.get_set_time.time_struct = return_time_struct;
  queue_msg.complete_flag = complete_flag;
  queue_msg.return_code = &ret;

  if ( tx_queue_send (rtc_server_self.request_queue, &queue_msg,
  RTC_QUEUE_MAX_WAIT_TICKS)
       != TX_SUCCESS )
  {
    return uSWIFT_MESSAGE_QUEUE_ERROR;
  }

  if ( tx_event_flags_get (rtc_server_self.complete_flags, complete_flag, TX_OR_CLEAR, &event_flags,
  RTC_FLAG_MAX_WAIT_TICKS)
       != TX_SUCCESS )
  {
    return uSWIFT_TIMEOUT;
  }

  return ret;
}

uSWIFT_return_code_t rtc_server_set_time ( struct tm *input_time_struct, ULONG complete_flag )
{
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;
  rtc_request_message queue_msg =
    { 0 };
  ULONG event_flags;

  queue_msg.request = SET_TIME;
  queue_msg.input_output_struct.get_set_time.time_struct = input_time_struct;
  queue_msg.complete_flag = complete_flag;
  queue_msg.return_code = &ret;

  if ( tx_queue_send (rtc_server_self.request_queue, &queue_msg,
  RTC_QUEUE_MAX_WAIT_TICKS)
       != TX_SUCCESS )
  {
    return uSWIFT_MESSAGE_QUEUE_ERROR;
  }

  if ( tx_event_flags_get (rtc_server_self.complete_flags, complete_flag, TX_OR_CLEAR, &event_flags,
  RTC_FLAG_MAX_WAIT_TICKS)
       != TX_SUCCESS )
  {
    return uSWIFT_TIMEOUT;
  }

  return ret;
}

uSWIFT_return_code_t rtc_server_set_timestamp ( pcf2131_timestamp_t which_timestamp,
                                                ULONG complete_flag )
{
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;
  rtc_request_message queue_msg =
    { 0 };
  ULONG event_flags;

  queue_msg.request = SET_TIMESTAMP;
  queue_msg.input_output_struct.set_timestamp.which_timestamp = which_timestamp;
  queue_msg.complete_flag = complete_flag;
  queue_msg.return_code = &ret;

  if ( tx_queue_send (rtc_server_self.request_queue, &queue_msg,
  RTC_QUEUE_MAX_WAIT_TICKS)
       != TX_SUCCESS )
  {
    return uSWIFT_MESSAGE_QUEUE_ERROR;
  }

  if ( tx_event_flags_get (rtc_server_self.complete_flags, complete_flag, TX_OR_CLEAR, &event_flags,
  RTC_FLAG_MAX_WAIT_TICKS)
       != TX_SUCCESS )
  {
    return uSWIFT_TIMEOUT;
  }

  return ret;
}

uSWIFT_return_code_t rtc_server_get_timestamp ( pcf2131_timestamp_t which_timestamp,
                                                struct tm *return_timestamp, ULONG complete_flag )
{
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;
  rtc_request_message queue_msg =
    { 0 };
  ULONG event_flags;

  queue_msg.request = GET_TIMESTAMP;
  queue_msg.input_output_struct.get_timestamp.which_timestamp = which_timestamp;
  queue_msg.input_output_struct.get_timestamp.timestamp = return_timestamp;
  queue_msg.complete_flag = complete_flag;
  queue_msg.return_code = &ret;

  if ( tx_queue_send (rtc_server_self.request_queue, &queue_msg,
  RTC_QUEUE_MAX_WAIT_TICKS)
       != TX_SUCCESS )
  {
    return uSWIFT_MESSAGE_QUEUE_ERROR;
  }

  if ( tx_event_flags_get (rtc_server_self.complete_flags, complete_flag, TX_OR_CLEAR, &event_flags,
  RTC_FLAG_MAX_WAIT_TICKS)
       != TX_SUCCESS )
  {
    return uSWIFT_TIMEOUT;
  }

  return ret;
}

uSWIFT_return_code_t rtc_server_set_alarm ( rtc_alarm_struct alarm_settings, ULONG complete_flag )
{
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;
  rtc_request_message queue_msg =
    { 0 };
  ULONG event_flags;

  queue_msg.request = SET_ALARM;
  queue_msg.input_output_struct.set_alarm = alarm_settings;
  queue_msg.complete_flag = complete_flag;
  queue_msg.return_code = &ret;

  if ( tx_queue_send (rtc_server_self.request_queue, &queue_msg,
  RTC_QUEUE_MAX_WAIT_TICKS)
       != TX_SUCCESS )
  {
    return uSWIFT_MESSAGE_QUEUE_ERROR;
  }

  if ( tx_event_flags_get (rtc_server_self.complete_flags, complete_flag, TX_OR_CLEAR, &event_flags,
  RTC_FLAG_MAX_WAIT_TICKS)
       != TX_SUCCESS )
  {
    return uSWIFT_TIMEOUT;
  }

  return ret;
}

uSWIFT_return_code_t rtc_server_clear_flag ( rtc_flag_t which_flag, ULONG complete_flag )
{
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;
  rtc_request_message queue_msg =
    { 0 };
  ULONG event_flags;

  queue_msg.request = CLEAR_FLAG;
  queue_msg.input_output_struct.clear_flag = which_flag;
  queue_msg.complete_flag = complete_flag;
  queue_msg.return_code = &ret;

  if ( tx_queue_send (rtc_server_self.request_queue, &queue_msg,
  RTC_QUEUE_MAX_WAIT_TICKS)
       != TX_SUCCESS )
  {
    return uSWIFT_MESSAGE_QUEUE_ERROR;
  }

  if ( tx_event_flags_get (rtc_server_self.complete_flags, complete_flag, TX_OR_CLEAR, &event_flags,
  RTC_FLAG_MAX_WAIT_TICKS)
       != TX_SUCCESS )
  {
    return uSWIFT_TIMEOUT;
  }

  return ret;
}

// Requires request message to be filled out
uSWIFT_return_code_t rtc_server_process_request ( rtc_request_message *request )
{
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;
  ULONG event_flags;

  if ( tx_queue_send (rtc_server_self.request_queue, request,
  RTC_QUEUE_MAX_WAIT_TICKS)
       != TX_SUCCESS )
  {
    return uSWIFT_MESSAGE_QUEUE_ERROR;
  }

  if ( tx_event_flags_get (rtc_server_self.complete_flags, request->complete_flag,
  TX_OR_CLEAR,
                           &event_flags,
                           RTC_FLAG_MAX_WAIT_TICKS)
       != TX_SUCCESS )
  {
    return uSWIFT_TIMEOUT;
  }

  return ret;
}

void struct_tm_dec_to_bcd ( struct tm *struct_ptr )
{
  // Account for leap seconds
  struct_ptr->tm_sec = (struct_ptr->tm_sec > 60) ?
      BCD_ERROR : DEC_TO_BCD(struct_ptr->tm_sec)
  ;
  struct_ptr->tm_min = (struct_ptr->tm_min > 59) ?
      BCD_ERROR : DEC_TO_BCD(struct_ptr->tm_min)
  ;
  struct_ptr->tm_hour = (struct_ptr->tm_hour > 23) ?
      BCD_ERROR : DEC_TO_BCD(struct_ptr->tm_hour)
  ;
  struct_ptr->tm_mday = (struct_ptr->tm_mday > 31) ?
      BCD_ERROR : DEC_TO_BCD(struct_ptr->tm_mday)
  ;
  struct_ptr->tm_mon = (struct_ptr->tm_mon > 12) ?
      BCD_ERROR : DEC_TO_BCD(struct_ptr->tm_mon)
  ;
  struct_ptr->tm_year = (struct_ptr->tm_year - 2000 > 99) ?
      BCD_ERROR : DEC_TO_BCD(struct_ptr->tm_year - 2000)
  ;
}

void struct_tm_bcd_to_dec ( struct tm *struct_ptr )
{
  struct_ptr->tm_sec = BCD_TO_DEC_SINGLE(struct_ptr->tm_sec);
  struct_ptr->tm_min = BCD_TO_DEC_SINGLE(struct_ptr->tm_min);
  struct_ptr->tm_hour = BCD_TO_DEC_SINGLE(struct_ptr->tm_hour);
  struct_ptr->tm_mday = BCD_TO_DEC_SINGLE(struct_ptr->tm_mday);
  struct_ptr->tm_mon = BCD_TO_DEC_SINGLE(struct_ptr->tm_mon);
}

