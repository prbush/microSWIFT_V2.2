/*
 * logger.h
 *
 *  Created on: Aug 20, 2024
 *      Author: philbush
 */

#ifndef COMPONENTS_INC_LOGGER_H_
#define COMPONENTS_INC_LOGGER_H_

#include "usart.h"
#include "tx_api.h"
#include "stdarg.h"
#include "stdbool.h"
#include "gpio.h"
#include "time.h"

#define LOG_QUEUE_LENGTH 32

#define MUTEX_LOCK_TICKS 5

#define LOGGER_MAX_TICKS_TO_TX_MSG 50 // ~17ms for buffer size 256 bytes @ 115200 baud, we'll add a few ms

// This macro is what all threads will call to log information
#define LOG(fmt, ...) (uart_log(fmt, ##__VA_ARGS__))

typedef struct
{
  char line_buf[320];
} log_line_buf;

typedef struct
{
  log_line_buf *str_buf;
  size_t strlen;
} logger_message;

typedef struct
{
  // Block pool for the char buffers
  TX_BLOCK_POOL *block_pool;
  // Message queue
  TX_QUEUE *msg_que;
  // Mutex
  TX_MUTEX *lock;
  // UART handle
  UART_HandleTypeDef *uart;
  void (*send_log_line) ( log_line_buf *buf, size_t strlen );
} uart_logger;

void uart_logger_init ( uart_logger *logger, TX_BLOCK_POOL *block_pool, TX_QUEUE *msg_que,
                        TX_MUTEX *mutex, UART_HandleTypeDef *uart_handle );
void uart_log ( const char *fmt, ... );
void uart_logger_return_line_buf ( log_line_buf *buffer );

#endif /* COMPONENTS_INC_LOGGER_H_ */
