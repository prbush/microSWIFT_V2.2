/*
 * logger.c
 *
 *  Created on: Aug 20, 2024
 *      Author: philbush
 */

#include <ext_rtc_server.h>
#include "logger.h"
#include "usart.h"
#include "tx_api.h"
#include "stdarg.h"
#include "time.h"
#include "string.h"
#include "stdio.h"
#include "threadx_support.h"

static uart_logger *logger_self;

static bool initialized = false;

static UINT _logger_get_buffer ( log_line_buf **line_buf );
static void _logger_send_log_line ( log_line_buf *buf, size_t strlen );

void uart_logger_init ( uart_logger *logger, TX_BLOCK_POOL *block_pool, TX_QUEUE *msg_que,
                        TX_MUTEX *mutex, UART_HandleTypeDef *uart_handle )
{
  logger_self = logger;

  logger_self->block_pool = block_pool;
  logger_self->msg_que = msg_que;
  logger_self->lock = mutex;
  logger_self->uart = uart_handle;
  logger_self->send_log_line = _logger_send_log_line;

  (void) usart3_init ();

  initialized = true;
}

void uart_log ( const char *fmt, ... )
{
  uint32_t init_wait_counter = 0;
  log_line_buf *log_buf = TX_NULL;
  logger_message msg =
    { 0 };
  int32_t bytes_remaining = sizeof(log_line_buf), str_len = 0;
  va_list args;
  va_start(args, fmt);
  UINT tx_ret;
  time_t sys_timestamp = get_system_time ();
  struct tm sys_time =
    { 0 };

  while ( !initialized )
  {
    tx_thread_sleep (1);
    if ( ++init_wait_counter > 10 )
    {
      return;
    }
  }

  // All threads have access to this function concurrently, need mutual exclusion
  if ( tx_mutex_get (logger_self->lock, MUTEX_LOCK_TICKS) != TX_SUCCESS )
  {
    return;
  }

  tx_ret = _logger_get_buffer (&log_buf);

  if ( tx_ret != TX_SUCCESS )
  {
    va_end(args);
    (void) tx_mutex_put (logger_self->lock);
    return;
  }

  // Put a timestamp in first
  sys_time = *gmtime (&sys_timestamp);
  str_len = strftime (&(log_buf->line_buf[0]), bytes_remaining, "%x %X: ", &sys_time);
  bytes_remaining -= str_len;

  str_len = vsnprintf (&(log_buf->line_buf[sizeof(log_line_buf) - bytes_remaining]),
                       bytes_remaining, fmt, args);

  va_end(args);

  bytes_remaining -= str_len;

  if ( bytes_remaining >= 2 )
  {
    bytes_remaining -= 2;
    // Put a break line at the end
    strncat (&log_buf->line_buf[0], "\r\n", bytes_remaining);
  }

  msg.str_buf = log_buf;
  msg.strlen = sizeof(log_line_buf) - bytes_remaining;

  // Make sure its a valid size
  if ( msg.strlen < 0 )
  {
    (void) tx_mutex_put (logger_self->lock);
    return;
  }

  (void) tx_queue_send (logger_self->msg_que, (VOID*) &msg, TX_NO_WAIT);

  (void) tx_mutex_put (logger_self->lock);
}

static UINT _logger_get_buffer ( log_line_buf **line_buf )
{
  return tx_block_allocate (logger_self->block_pool, (VOID**) line_buf, TX_NO_WAIT);
}

static void _logger_send_log_line ( log_line_buf *buf, size_t strlen )
{
  (void) HAL_UART_Transmit_DMA (logger_self->uart, (uint8_t*) &(buf->line_buf[0]), strlen);
}

void uart_logger_return_line_buf ( log_line_buf *buffer )
{
  memset (buffer, 0, sizeof(log_line_buf));
  (void) tx_block_release ((VOID*) buffer);
}
