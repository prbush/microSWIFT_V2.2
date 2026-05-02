/* USER CODE BEGIN Header */

/**
 ******************************************************************************
 * @file    app_threadx.c
 * @author  MCD Application Team
 * @brief   ThreadX applicative file
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2022 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 ******************************************************************************
 *
 */
// clang-format off
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "app_threadx.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
// clang-format on
#include "NEDWaves/mem_replacements.h"
#include "accelerometer.h"
#include "app_filex.h"
#include "battery.h"
#include "ct_sensor.h"
#include "ext_rtc.h"
#include "ext_rtc_server.h"
#include "gnss.h"
#include "iridium.h"
#include "light_sensor.h"
#include "main.h"
#include "rf_switch.h"
#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"
#include "stdio.h"
#include "stm32u5xx_hal.h"
#include "string.h"
#include "temp_sensor.h"
#include "turbidity_sensor.h"
#include <math.h>

#include "NEDWaves/test_data_includes.h"

#include "adc.h"
#include "configuration.h"
#include "controller.h"
#include "file_system_server.h"
#include "leds.h"
#include "linked_list.h"
#include "logger.h"
#include "persistent_ram.h"
#include "sbd.h"
#include "shared_i2c_bus.h"
#include "testing_hooks.h"
#include "threadx_support.h"
#include "watchdog.h"

// Waves files
#include "NEDWaves/NEDwaves_memlight.h"
#include "NEDWaves/NEDwaves_memlight_emxAPI.h"
#include "NEDWaves/NEDwaves_memlight_terminate.h"
#include "NEDWaves/NEDwaves_memlight_types.h"
#include "NEDWaves/rt_nonfinite.h"
#include "NEDWaves/rtwhalf.h"
// clang-format off
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
// clang-format on

// clang-format off
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// clang-format on

// clang-format off
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
// clang-format on
// clang-format off
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
// clang-format on
// The configuration struct
microSWIFT_configuration configuration;
// The SBD message we'll assemble
sbd_message_type_52 sbd_message;
// Handles for all the STM32 peripherals
Device_Handles device_handles;
// The primary byte pool from which all memory is allocated from
TX_BYTE_POOL *byte_pool;
// Our threads
TX_THREAD control_thread;
TX_THREAD rtc_thread;
TX_THREAD led_thread;
TX_THREAD i2c_bus_thread;
TX_THREAD logger_thread;
TX_THREAD gnss_thread;
TX_THREAD ct_thread;
TX_THREAD temperature_thread;
TX_THREAD light_thread;
TX_THREAD turbidity_thread;
TX_THREAD waves_thread;
TX_THREAD iridium_thread;
TX_THREAD accel_thread;
// clang-format off
Thread_Handles thread_handles =
  {
    &control_thread,
    &rtc_thread,
    &led_thread,
    &i2c_bus_thread,
    &logger_thread,
    &gnss_thread,
    &ct_thread,
    &temperature_thread,
    &light_thread,
    &turbidity_thread,
    &waves_thread,
    &iridium_thread,
    &fx_thread,
    &accel_thread
  };
// clang-format on
// Used to track initialization status of threads and components
TX_EVENT_FLAGS_GROUP initialization_flags;
// We'll use these flags to indicate a thread has completed execution
TX_EVENT_FLAGS_GROUP complete_flags;
// These flags are used to indicate an interrupt event has occurred
TX_EVENT_FLAGS_GROUP irq_flags;
// Flags for errors
TX_EVENT_FLAGS_GROUP error_flags;
// RTC complete flags (for RTC server)
TX_EVENT_FLAGS_GROUP rtc_complete_flags;
// I2C complete flags
TX_EVENT_FLAGS_GROUP i2c_complete_flags;
// Flags for which thread is checkin in with watchdog
TX_EVENT_FLAGS_GROUP watchdog_check_in_flags;
// Timers for threads
TX_TIMER led_duration_timer;
TX_TIMER control_timer;
TX_TIMER gnss_timer;
TX_TIMER ct_timer;
TX_TIMER temperature_timer;
TX_TIMER light_timer;
TX_TIMER turbidity_timer;
TX_TIMER waves_timer;
TX_TIMER iridium_timer;
// Comms buses semaphores !! No GNSS UART sema as it uses event flags instead
TX_SEMAPHORE ext_rtc_spi_sema;
TX_SEMAPHORE core_i2c_sema;
TX_SEMAPHORE iridium_uart_sema;
TX_SEMAPHORE ct_uart_sema;
TX_SEMAPHORE expansion_uart_sema;
// used to tell logger_thread that HAL_UART_Transmit_DMA has finished
TX_SEMAPHORE logger_sema;
// Logger mutex; protects LOG call since all threads have access to it
TX_MUTEX logger_mutex;
// Server/client message queue for RTC (including watchdog function)
TX_QUEUE rtc_messaging_queue;
// Logger message passing queue
TX_QUEUE logger_message_queue;
// Queue for LED thread
TX_QUEUE led_queue;
// Shared I2C bus queue
TX_QUEUE i2c_bus_queue;

// Buffer for the Waves byte pool to allow dynamic memory allocation in a
// bounded fashion
__ALIGN_BEGIN UCHAR waves_byte_pool_buffer[WAVES_MEM_POOL_SIZE]
    __attribute__((section(".ram1"))) __ALIGN_END;
TX_BYTE_POOL waves_byte_pool;

// Logger block pool
__ALIGN_BEGIN UCHAR
    logger_block_buffer[(sizeof(log_line_buf) * LOG_QUEUE_LENGTH) +
                        (LOG_QUEUE_LENGTH * sizeof(void *))]
    __attribute__((section(".ram1"))) __ALIGN_END;

TX_BLOCK_POOL logger_block_pool;

// Light sensor sample buffer
ALIGN_32BYTES(
    light_basic_counts
        light_sensor_sample_buffer[LIGHT_SENSOR_BYTE_POOL_BUFFER_SIZE /
                                   sizeof(light_basic_counts)]
    __attribute__((section(".ram1"))));

// Turbidity sensor sample buffer
ALIGN_32BYTES(uint16_t turbidity_sensor_ambient_buffer
                  [TURBIDITY_SENSOR_SAMPLE_BUFFER_SIZE / sizeof(uint16_t)]
              __attribute__((section(".ram1"))));
ALIGN_32BYTES(uint16_t turbidity_sensor_proximity_buffer
                  [TURBIDITY_SENSOR_SAMPLE_BUFFER_SIZE / sizeof(uint16_t)]
              __attribute__((section(".ram1"))));
// clang-format off
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
// clang-format on

// Threads
static void rtc_thread_entry(ULONG thread_input);
static void logger_thread_entry(ULONG thread_input);
static void control_thread_entry(ULONG thread_input);
static void led_thread_entry(ULONG thread_input);
static void i2c_bus_thread_entry(ULONG thread_input);
static void gnss_thread_entry(ULONG thread_input);
static void waves_thread_entry(ULONG thread_input);
static void iridium_thread_entry(ULONG thread_input);
static void ct_thread_entry(ULONG thread_input);
static void temperature_thread_entry(ULONG thread_input);
static void light_thread_entry(ULONG thread_input);
static void turbidity_thread_entry(ULONG thread_input);
static void accel_thread_entry(ULONG thread_input);

// clang-format off
/* USER CODE END PFP */

/**
  * @brief  Application ThreadX Initialization.
  * @param memory_ptr: memory pointer
  * @retval int
  */
UINT App_ThreadX_Init(VOID *memory_ptr)
{
  UINT ret = TX_SUCCESS;
  /* USER CODE BEGIN App_ThreadX_MEM_POOL */
  // clang-format on
  (void)byte_pool;
  CHAR *pointer = TX_NULL;
  byte_pool = memory_ptr;

  // ============================== Threads ==============================

  // Allocate stack for the control thread
  ret = tx_byte_allocate(byte_pool, (VOID **)&pointer, XL_STACK, TX_NO_WAIT);
  if (ret != TX_SUCCESS) {
    return ret;
  }

  // Create the control thread. HIGHEST priority level and no preemption
  // possible
  ret =
      tx_thread_create(&control_thread, "control thread", control_thread_entry,
                       0, pointer, XL_STACK, HIGHEST_PRIORITY, HIGHEST_PRIORITY,
                       TX_NO_TIME_SLICE, TX_AUTO_START);
  if (ret != TX_SUCCESS) {
    return ret;
  }

  //
  // Allocate stack for the rtc thread
  ret = tx_byte_allocate(byte_pool, (VOID **)&pointer, M_STACK, TX_NO_WAIT);
  if (ret != TX_SUCCESS) {
    return ret;
  }
  // Create the rtc thread. VERY_HIGH priority level and no preemption possible
  ret = tx_thread_create(&rtc_thread, "rtc thread", rtc_thread_entry, 0,
                         pointer, M_STACK, VERY_HIGH_PRIORITY, HIGHEST_PRIORITY,
                         TX_NO_TIME_SLICE, TX_AUTO_START);
  if (ret != TX_SUCCESS) {
    return ret;
  }

  //
  // Allocate stack for the LED thread
  ret = tx_byte_allocate(byte_pool, (VOID **)&pointer, XXS_STACK, TX_NO_WAIT);
  if (ret != TX_SUCCESS) {
    return ret;
  }
  // Create the LED thread. Low priority priority level and no preemption
  // possible
  ret = tx_thread_create(&led_thread, "LED thread", led_thread_entry, 0,
                         pointer, XXS_STACK, VERY_HIGH_PRIORITY,
                         HIGHEST_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);
  if (ret != TX_SUCCESS) {
    return ret;
  }

  //
  // Allocate stack for the I2C thread
  ret = tx_byte_allocate(byte_pool, (VOID **)&pointer, XS_STACK, TX_NO_WAIT);
  if (ret != TX_SUCCESS) {
    return ret;
  }
  // Create the I2C thread, VERY_HIGH_PRIORITY level and no preemption possible
  ret =
      tx_thread_create(&i2c_bus_thread, "I2C bus thread", i2c_bus_thread_entry,
                       0, pointer, XS_STACK, VERY_HIGH_PRIORITY,
                       HIGHEST_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);
  if (ret != TX_SUCCESS) {
    return ret;
  }

  //
  // Allocate stack for the logger thread
  ret = tx_byte_allocate(byte_pool, (VOID **)&pointer, M_STACK, TX_NO_WAIT);
  if (ret != TX_SUCCESS) {
    return ret;
  }
  // Create the logger thread. Low priority priority level and no preemption
  // possible
  ret = tx_thread_create(&logger_thread, "logger thread", logger_thread_entry,
                         0, pointer, M_STACK, LOW_PRIORITY, HIGHEST_PRIORITY,
                         TX_NO_TIME_SLICE, TX_AUTO_START);
  if (ret != TX_SUCCESS) {
    return ret;
  }

  //
  // Allocate stack for the gnss thread
  ret = tx_byte_allocate(byte_pool, (VOID **)&pointer, XL_STACK, TX_NO_WAIT);
  if (ret != TX_SUCCESS) {
    return ret;
  }
  // Create the gnss thread. MID priority, no preemption-threshold
  ret = tx_thread_create(&gnss_thread, "gnss thread", gnss_thread_entry, 0,
                         pointer, XL_STACK, HIGH_PRIORITY, HIGHEST_PRIORITY,
                         TX_NO_TIME_SLICE, TX_DONT_START);
  if (ret != TX_SUCCESS) {
    return ret;
  }

  //
  // Allocate stack for the CT thread
  ret = tx_byte_allocate(byte_pool, (VOID **)&pointer, S_STACK, TX_NO_WAIT);
  if (ret != TX_SUCCESS) {
    return ret;
  }
  // Create the CT thread. MID priority, no preemption-threshold
  ret = tx_thread_create(&ct_thread, "ct thread", ct_thread_entry, 0, pointer,
                         S_STACK, MID_PRIORITY, HIGHEST_PRIORITY,
                         TX_NO_TIME_SLICE, TX_DONT_START);
  if (ret != TX_SUCCESS) {
    return ret;
  }

  //
  // Allocate stack for the temperature thread
  ret = tx_byte_allocate(byte_pool, (VOID **)&pointer, M_STACK, TX_NO_WAIT);
  if (ret != TX_SUCCESS) {
    return ret;
  }
  // Create the temperature thread. MID priority, no preemption-threshold
  ret = tx_thread_create(&temperature_thread, "temperature thread",
                         temperature_thread_entry, 0, pointer, M_STACK,
                         MID_PRIORITY, HIGHEST_PRIORITY, TX_NO_TIME_SLICE,
                         TX_DONT_START);
  if (ret != TX_SUCCESS) {
    return ret;
  }

  //
  // Allocate stack for the light thread
  ret = tx_byte_allocate(byte_pool, (VOID **)&pointer, XL_STACK, TX_NO_WAIT);
  if (ret != TX_SUCCESS) {
    return ret;
  }
  // Create the light thread. MID priority, no preemption-threshold
  ret = tx_thread_create(&light_thread, "light thread", light_thread_entry, 0,
                         pointer, XL_STACK, MID_PRIORITY, HIGHEST_PRIORITY,
                         TX_NO_TIME_SLICE, TX_DONT_START);
  if (ret != TX_SUCCESS) {
    return ret;
  }

  //
  // Allocate stack for the turbidity thread
  ret = tx_byte_allocate(byte_pool, (VOID **)&pointer, XL_STACK, TX_NO_WAIT);
  if (ret != TX_SUCCESS) {
    return ret;
  }
  // Create the turbidity thread. MID priority, no preemption-threshold
  ret = tx_thread_create(&turbidity_thread, "turbidity thread",
                         turbidity_thread_entry, 0, pointer, XL_STACK,
                         MID_PRIORITY, HIGHEST_PRIORITY, TX_NO_TIME_SLICE,
                         TX_DONT_START);
  if (ret != TX_SUCCESS) {
    return ret;
  }

  //
  // Allocate stack for the waves thread
  ret = tx_byte_allocate(byte_pool, (VOID **)&pointer, XXL_STACK, TX_NO_WAIT);
  if (ret != TX_SUCCESS) {
    return ret;
  }
  // Create the waves thread. Lowest priority, can be preempted by any other
  // thread
  ret = tx_thread_create(&waves_thread, "waves thread", waves_thread_entry, 0,
                         pointer, XXL_STACK, LOWEST_PRIORITY, LOWEST_PRIORITY,
                         TX_NO_TIME_SLICE, TX_DONT_START);
  if (ret != TX_SUCCESS) {
    return ret;
  }

  //
  // Allocate stack for the Iridium thread
  ret = tx_byte_allocate(byte_pool, (VOID **)&pointer, XL_STACK, TX_NO_WAIT);
  if (ret != TX_SUCCESS) {
    return ret;
  }
  // Create the Iridium thread. HIGH priority, no preemption-threshold
  ret =
      tx_thread_create(&iridium_thread, "iridium thread", iridium_thread_entry,
                       0, pointer, XL_STACK, HIGH_PRIORITY, HIGHEST_PRIORITY,
                       TX_NO_TIME_SLICE, TX_DONT_START);
  if (ret != TX_SUCCESS) {
    return ret;
  }

  // Allocate stack for the Accelerometer thread
  ret = tx_byte_allocate(byte_pool, (VOID **)&pointer, S_STACK, TX_NO_WAIT);
  if (ret != TX_SUCCESS) {
    return ret;
  }
  // Create the accelerometer thread. HIGH priority, no preemption-threshold
  ret = tx_thread_create(&accel_thread, "accelerometer thread",
                         accel_thread_entry, 0, pointer, S_STACK, HIGH_PRIORITY,
                         HIGHEST_PRIORITY, TX_NO_TIME_SLICE, TX_DONT_START);
  if (ret != TX_SUCCESS) {
    return ret;
  }

  // ======================== Byte and Block Pools =============================

  // Block pool for the UART logger
  ret = tx_block_pool_create(&logger_block_pool, "UART Logger Block Pool",
                             sizeof(log_line_buf), &logger_block_buffer[0],
                             sizeof(logger_block_buffer));
  if (ret != TX_SUCCESS) {
    return ret;
  }
  /* Note: the Waves thread will create and manage its own block pool. */

  // ============================== Event Flags ==============================

  // For tracking initialization of components and threads
  ret = tx_event_flags_create(&initialization_flags, "init flags");
  if (ret != TX_SUCCESS) {
    return ret;
  }
  //
  // Create the event flags we'll use for tracking thread completion
  ret = tx_event_flags_create(&complete_flags, "completion flags");
  if (ret != TX_SUCCESS) {
    return ret;
  }
  //
  // Create the event flags we'll use for tracking interrupt events
  ret = tx_event_flags_create(&irq_flags, "interrupt flags");
  if (ret != TX_SUCCESS) {
    return ret;
  }
  //
  // Create the error flags we'll use for tracking errors
  ret = tx_event_flags_create(&error_flags, "error flags");
  if (ret != TX_SUCCESS) {
    return ret;
  }
  //
  // Create the rtc complete flags we'll use for tracking rtc function
  // completion
  ret = tx_event_flags_create(&rtc_complete_flags, "RTC complete flags");
  if (ret != TX_SUCCESS) {
    return ret;
  }
  //
  // Create the I2C complete flags we'll use for tracking I2C function
  // completion
  ret = tx_event_flags_create(&i2c_complete_flags, "I2C complete flags");
  if (ret != TX_SUCCESS) {
    return ret;
  }
  //
  // Create watchdog check in flags -- used to track which thread has checked in
  ret = tx_event_flags_create(&watchdog_check_in_flags,
                              "watchdog check-in flags");
  if (ret != TX_SUCCESS) {
    return ret;
  }

  // ============================== Timers ==============================

  ret = tx_timer_create(&led_duration_timer, "LED duration timer",
                        led_timer_expired, 0, 1, 0, TX_NO_ACTIVATE);
  if (ret != TX_SUCCESS) {
    return ret;
  }

  ret = tx_timer_create(&control_timer, "Control thread timer",
                        control_timer_expired, 0, 1, 0, TX_NO_ACTIVATE);
  if (ret != TX_SUCCESS) {
    return ret;
  }

  ret = tx_timer_create(&gnss_timer, "GNSS thread timer", gnss_timer_expired, 0,
                        1, 0, TX_NO_ACTIVATE);
  if (ret != TX_SUCCESS) {
    return ret;
  }

  ret = tx_timer_create(&ct_timer, "CT thread timer", ct_timer_expired, 0, 1, 0,
                        TX_NO_ACTIVATE);
  if (ret != TX_SUCCESS) {
    return ret;
  }

  ret = tx_timer_create(&temperature_timer, "Temperature thread timer",
                        temperature_timer_expired, 0, 1, 0, TX_NO_ACTIVATE);
  if (ret != TX_SUCCESS) {
    return ret;
  }

  ret = tx_timer_create(&light_timer, "Light thread timer", light_timer_expired,
                        0, 1, 0, TX_NO_ACTIVATE);
  if (ret != TX_SUCCESS) {
    return ret;
  }

  ret = tx_timer_create(&turbidity_timer, "Turbidity thread timer",
                        turbidity_timer_expired, 0, 1, 0, TX_NO_ACTIVATE);
  if (ret != TX_SUCCESS) {
    return ret;
  }

  ret = tx_timer_create(&iridium_timer, "Iridium thread timer",
                        iridium_timer_expired, 0, 1, 0, TX_NO_ACTIVATE);
  if (ret != TX_SUCCESS) {
    return ret;
  }

  // ============================== Semaphores ==============================

  // Semaphores to identify comms bus DMA Tx/Rx completion
  ret = tx_semaphore_create(&ext_rtc_spi_sema, "RTC SPI sema", 0);
  if (ret != TX_SUCCESS) {
    return ret;
  }

  ret = tx_semaphore_create(&core_i2c_sema, "Core I2C sema", 0);
  if (ret != TX_SUCCESS) {
    return ret;
  }

  ret = tx_semaphore_create(&iridium_uart_sema, "Iridium UART sema", 0);
  if (ret != TX_SUCCESS) {
    return ret;
  }

  ret = tx_semaphore_create(&ct_uart_sema, "CT UART sema", 0);
  if (ret != TX_SUCCESS) {
    return ret;
  }

  ret = tx_semaphore_create(&expansion_uart_sema, "Expansion UART sema", 0);
  if (ret != TX_SUCCESS) {
    return ret;
  }

  ret = tx_semaphore_create(&logger_sema, "Logger UART sema", 0);
  if (ret != TX_SUCCESS) {
    return ret;
  }

  // ============================== Mutexes ==============================

  ret = tx_mutex_create(&logger_mutex, "UART Logger mutex", TX_NO_INHERIT);
  if (ret != TX_SUCCESS) {
    return ret;
  }

  // ========================== Message Queues ==============================

  // Server message queue for UART Logger
  //
  // Allocate buffer space for the message queue
  ret = tx_byte_allocate(
      byte_pool, (VOID **)&pointer,
      GET_QUEUE_BUFFER_SIZE(sizeof(logger_message), LOG_QUEUE_LENGTH),
      TX_NO_WAIT);
  if (ret != TX_SUCCESS) {
    return ret;
  }

  ret = tx_queue_create(
      &logger_message_queue, "logger msg queue",
      GET_QUEUE_MSG_ELEMENT_SIZE(sizeof(logger_message)), pointer,
      GET_QUEUE_BUFFER_SIZE(sizeof(logger_message), LOG_QUEUE_LENGTH));
  if (ret != TX_SUCCESS) {
    return ret;
  }

  // Server message queue for RTC (including watchdog function)
  ret = tx_byte_allocate(
      byte_pool, (VOID **)&pointer,
      GET_QUEUE_BUFFER_SIZE(sizeof(rtc_request_message), RTC_QUEUE_LENGTH),
      TX_NO_WAIT);
  if (ret != TX_SUCCESS) {
    return ret;
  }

  ret = tx_queue_create(
      &rtc_messaging_queue, "RTC msg queue",
      GET_QUEUE_MSG_ELEMENT_SIZE(sizeof(rtc_request_message)), pointer,
      GET_QUEUE_BUFFER_SIZE(sizeof(rtc_request_message), RTC_QUEUE_LENGTH));
  if (ret != TX_SUCCESS) {
    return ret;
  }

  ret = tx_byte_allocate(
      byte_pool, (VOID **)&pointer,
      GET_QUEUE_BUFFER_SIZE(sizeof(led_message), LED_MESSAGE_QUEUE_LENGTH),
      TX_NO_WAIT);
  if (ret != TX_SUCCESS) {
    return ret;
  }

  ret = tx_queue_create(
      &led_queue, "LED queue", GET_QUEUE_MSG_ELEMENT_SIZE(sizeof(led_message)),
      pointer,
      GET_QUEUE_BUFFER_SIZE(sizeof(led_message), LED_MESSAGE_QUEUE_LENGTH));
  if (ret != TX_SUCCESS) {
    return ret;
  }

  ret = tx_byte_allocate(
      byte_pool, (VOID **)&pointer,
      GET_QUEUE_BUFFER_SIZE(sizeof(i2c_queue_message), I2C_QUEUE_LENGTH),
      TX_NO_WAIT);
  if (ret != TX_SUCCESS) {
    return ret;
  }

  ret = tx_queue_create(
      &i2c_bus_queue, "I2C Bus queue",
      GET_QUEUE_MSG_ELEMENT_SIZE(sizeof(i2c_queue_message)), pointer,
      GET_QUEUE_BUFFER_SIZE(sizeof(i2c_queue_message), I2C_QUEUE_LENGTH));
  if (ret != TX_SUCCESS) {
    return ret;
  }

  // ========================== Misc init ==============================

  device_handles.core_spi_handle = &hspi1;
  device_handles.expansion_spi_handle = &hspi2;
  device_handles.core_i2c_handle = &hi2c2;
  device_handles.iridium_uart_handle = &huart4;
  device_handles.gnss_uart_handle = &hlpuart1;
  device_handles.ct_uart_handle = &huart1;
  device_handles.logger_uart_handle = &huart3;
  device_handles.expansion_uart_handle = &huart2;
  device_handles.ext_psram_handle = &hospi1;
  device_handles.battery_adc = &hadc1;
  device_handles.gnss_uart_tx_dma_handle = &handle_GPDMA1_Channel9;
  device_handles.gnss_uart_rx_dma_handle = &handle_GPDMA1_Channel8;
  device_handles.iridium_uart_tx_dma_handle = &handle_GPDMA1_Channel7;
  device_handles.iridium_uart_rx_dma_handle = &handle_GPDMA1_Channel6;
  device_handles.ct_uart_tx_dma_handle = &handle_GPDMA1_Channel1;
  device_handles.ct_uart_rx_dma_handle = &handle_GPDMA1_Channel0;
  device_handles.logger_uart_tx_dma_handle = &handle_GPDMA1_Channel5;
  device_handles.logger_uart_rx_dma_handle = &handle_GPDMA1_Channel4;
  device_handles.expansion_uart_tx_dma_handle = &handle_GPDMA1_Channel3;
  device_handles.expansion_uart_rx_dma_handle = &handle_GPDMA1_Channel2;

  persistent_ram_get_device_config(&configuration);
  // TODO: Revert this once I've added it to the configuration!
  // configuration.accelerometer_enabled = true;

  // clang-format off
  /* USER CODE END App_ThreadX_MEM_POOL */
  /* USER CODE BEGIN App_ThreadX_Init */
  // clang-format on
  //
  // Run tests if needed
  if (tests.threadx_init_test != NULL) {
    tests.threadx_init_test(NULL);
  }

  // clang-format off
  /* USER CODE END App_ThreadX_Init */

  return ret;
}

  /**
  * @brief  Function that implements the kernel's initialization.
  * @param  None
  * @retval None
  */
void MX_ThreadX_Init(void)
{
  /* USER CODE BEGIN Before_Kernel_Start */
  // clang-format on

  // clang-format off
  /* USER CODE END Before_Kernel_Start */

  tx_kernel_enter();

  /* USER CODE BEGIN Kernel_Start_Error */
  // clang-format on

  // clang-format off
  /* USER CODE END Kernel_Start_Error */
}

/* USER CODE BEGIN 1 */
// clang-format on

/*
 * @brief  RTC thread. Manages all RTC operations.
 *
 * @note   This is a server style thread to manage shared hardware. Multiple
 * threads require access to the RTC, so all operations happen in this thread.
 *
 *         Threads call functions defined in ext_rtc_server.h which place
 * requests on the rtc queue which are then handled here.
 *
 * @param  ULONG thread_input - unused
 * @retval void
 */
static uint32_t watchdog_refresh_counter = 0; // debugging
static void rtc_thread_entry(ULONG thread_input) {
  UNUSED(thread_input);
  Ext_RTC rtc;
  TX_THREAD *this_thread = tx_thread_identify();
  uSWIFT_return_code_t ret;
  UINT tx_ret;
  rtc_request_message req;

  ret = ext_rtc_init(&rtc, device_handles.core_spi_handle, &ext_rtc_spi_sema);

  rtc_server_init(&rtc_messaging_queue, &rtc_complete_flags);

  // Setup the RTC, this will also enable the watchdog, setting a refresh period
  // as defined by WATCHDOG_PERIOD in ext_rtc.h
  ret |= rtc.setup_rtc();

  if (ret != uSWIFT_SUCCESS) {
    // Gotta wait for the logger to fully initialize
    tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);
    rtc_error_out(this_thread, "RTC failed to initialize.");
  }

  (void)tx_event_flags_set(&initialization_flags, RTC_INIT_SUCCESS, TX_OR);

  LOG("RTC Initialization successful.");

  while (1) {
    // See if we have any requests on the queue
    tx_ret = tx_queue_receive(&rtc_messaging_queue, &req, TX_WAIT_FOREVER);
    if (tx_ret == TX_SUCCESS) {
      switch (req.request) {
      case REFRESH_WATCHDOG:
        watchdog_refresh_counter++;
        ret = rtc.refresh_watchdog();
        break;

      case GET_TIME:
        ret =
            rtc.get_date_time(req.input_output_struct.get_set_time.time_struct);
        break;

      case SET_TIME:

        ret =
            rtc.set_date_time(req.input_output_struct.get_set_time.time_struct);
        break;

      case SET_TIMESTAMP:
        ret = rtc.set_timestamp(
            req.input_output_struct.set_timestamp.which_timestamp);
        break;

      case GET_TIMESTAMP:
        ret = rtc.get_timestamp(
            req.input_output_struct.get_timestamp.which_timestamp,
            req.input_output_struct.get_timestamp.timestamp);
        break;

      case SET_ALARM:
        ret = rtc.set_alarm(req.input_output_struct.set_alarm);
        break;

      case CLEAR_FLAG:
        ret = rtc.clear_flag(req.input_output_struct.clear_flag);
        break;

      default:
        ret = uSWIFT_PARAMETERS_INVALID;
        break;
      }

      if (ret != uSWIFT_SUCCESS) {
        rtc_error_out(this_thread,
                      "RTC failed to service request %d, returning code %d.",
                      (int)req.request, (int)ret);
      }

      if (req.return_code != NULL) {
        *req.return_code = ret;
      }
      (void)tx_event_flags_set(&rtc_complete_flags, req.complete_flag, TX_OR);
    }
  }
}

/*
 * @brief  LED thread entry
 *         Manages LED status
 * @param  ULONG thread_input - unused
 * @retval void
 */
static void led_thread_entry(ULONG thread_input) {
  UINT tx_ret;
  led_message msg;
  LEDs leds;

  leds_init(&leds, &led_duration_timer, &led_queue);

  while (1) {
    tx_ret = tx_queue_receive(&led_queue, &msg, TX_WAIT_FOREVER);
    if (tx_ret == TX_SUCCESS) {
      leds.play_sequence(msg.sequence, msg.duration_sec);
    }
  }
}

/*
 * @brief  I2C Bus thread entry
 *         Manages the shared I2C bus
 * @param  ULONG thread_input - unused
 * @retval void
 */
static void i2c_bus_thread_entry(ULONG thread_input) {
  TX_THREAD *this_thread = tx_thread_identify();
  Shared_I2C_Bus shared_bus;
  i2c_queue_message incoming_msg = {0};
  uSWIFT_return_code_t ret;
  UINT tx_ret;

  if (!shared_i2c_init(&shared_bus, &hi2c2, &core_i2c_sema)) {
    i2c_error_out(this_thread, "Core I2C Bus failed to initialize.");
  }

  shared_i2c_server_init(&i2c_bus_queue, &i2c_complete_flags);

  while (1) {
    // See if we have any requests on the queue
    tx_ret = tx_queue_receive(&i2c_bus_queue, &incoming_msg, TX_WAIT_FOREVER);
    if (tx_ret == TX_SUCCESS) {
      switch (incoming_msg.operation_type) {
      case I2C_READ:
        ret = shared_bus.read(incoming_msg.dev_addr, incoming_msg.dev_reg,
                              incoming_msg.input_output_buffer,
                              incoming_msg.data_len);
        break;

      case I2C_WRITE:
        ret = shared_bus.write(incoming_msg.dev_addr, incoming_msg.dev_reg,
                               incoming_msg.input_output_buffer,
                               incoming_msg.data_len);
        break;

      default:
        ret = uSWIFT_PARAMETERS_INVALID;
        break;
      }

      if (ret != uSWIFT_SUCCESS) {
        // TODO: There are much cleaner ways to do this int -> string lookup.
        char request_type[16];
        switch (incoming_msg.operation_type) {
        case I2C_READ:
          snprintf(request_type, 16, "READ");
          break;
        case I2C_WRITE:
          snprintf(request_type, 16, "WRITE");
          break;
        default:
          snprintf(request_type, 16, "unknown");
          break;
        }

        char failure_mode[16];
        switch (ret) {
        case uSWIFT_MESSAGE_QUEUE_ERROR:
          snprintf(failure_mode, 16, "QUEUE ERROR");
          break;
        case uSWIFT_TIMEOUT:
          snprintf(failure_mode, 16, "TIMEOUT");
          break;
        case uSWIFT_IO_ERROR:
          snprintf(failure_mode, 16, "IO ERROR");
          break;
        case uSWIFT_I2C_BUSY_ERROR:
          snprintf(failure_mode, 16, "I2C BUSY");
          break;
        case uSWIFT_I2C_SEMAPHORE_ERROR:
          snprintf(failure_mode, 16, "I2C SEMAPHORE");
          break;
        case uSWIFT_I2C_WRITE_ERROR:
          snprintf(failure_mode, 16, "I2C WRITE ERROR");
          break;
        case uSWIFT_I2C_READ_ERROR:
          snprintf(failure_mode, 16, "I2C READ ERROR");
          break;
        default:
          // this is a 32-bit enum; not sure why I got a compiler warning for
          // using %d
          snprintf(failure_mode, 16, "unknown: %lld", ret);
          break;
        }

        LOG("I2C Bus failed to service %s request: %s.", request_type,
            failure_mode);
      }

      if (incoming_msg.return_code != NULL) {
        *incoming_msg.return_code = ret;
      }

      (void)tx_event_flags_set(&i2c_complete_flags, incoming_msg.complete_flag,
                               TX_OR);
    }
  }
}

/*
 * @brief  Logger thread entry
 *         Logs all system messages to Virtual Com Port (VCP). Logs are also
 * passed down to the file system to be written to the SD card.
 *         !! Logging return codes are not checked and any function is allowed
 * to fail silently, such that the logging functionality cannot crash the system
 * or create a bottleneck. !!
 * @param  ULONG thread_input - unused
 * @retval void
 */
static void logger_thread_entry(ULONG thread_input) {
  UNUSED(thread_input);
  logger_message msg;
  UINT tx_ret;
  uart_logger logger = {0};

  uart_logger_init(&logger, &logger_block_pool, &logger_message_queue,
                   &logger_mutex, device_handles.logger_uart_handle);

  while (1) {

    tx_ret = tx_queue_receive(&logger_message_queue, &msg, TX_WAIT_FOREVER);
    if (tx_ret == TX_SUCCESS) {
      logger.send_log_line(&(msg.str_buf[0]), msg.strlen);
      // Need to wait until the transmission is complete before grabbing another
      // message
      (void)tx_semaphore_get(&logger_sema, TX_WAIT_FOREVER);
      // Pass the buffer down to the file system for saving to SD card
      file_system_server_save_log_line((char *)&(msg.str_buf[0]));
    }
  }
}

/*
 * @brief  Control thread entry
 *         Primary control thread, manages all other threads and meta state.
 * @param  ULONG thread_input - unused
 * @retval void
 */
static void control_thread_entry(ULONG thread_input) {
  UNUSED(thread_input);
  Control control = {0};
  struct watchdog_t watchdog = {0};
  bool first_window = is_first_sample_window(), heartbeat_started = false;
  uint32_t watchdog_counter = 0,
           sample_window = persistent_ram_get_sample_window_counter();
  ULONG start_time = 0;
  microSWIFT_firmware_version_t version = {0};

  persistent_ram_get_firmware_version(&version);

  controller_init(&control, &configuration, &thread_handles, &error_flags,
                  &initialization_flags, &irq_flags, &complete_flags,
                  &control_timer, device_handles.battery_adc, &sbd_message);

  LOG("\r\n\r\nHello World!\r\nmicroSWIFT %lu.\r\nFirmware major version %hu, "
      "minor version %hu.\r\nSample window %lu",
      configuration.tracking_number, version.major_rev, version.minor_rev,
      sample_window);

  // Run tests if needed
  if (tests.control_test != NULL) {
    tests.control_test(&control);
  }

  if (watchdog_init(&watchdog, &watchdog_check_in_flags) != WATCHDOG_OK) {
    Error_Handler();
  }

  watchdog_check_in(CONTROL_THREAD);

  // Run the self test
  if (!control.startup_procedure()) {
    if (first_window) {

      led_light_sequence(TEST_FAILED_LED_SEQUENCE, LED_SEQUENCE_FOREVER);
      tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND * 30);

      control.shutdown_procedure();

      // NOTE(LEL): I don't think this ever gets called, because
      // shutdown_procedure sets an alarm and puts the processor to sleep.
      Error_Handler();
    }
  }

  watchdog_check_in(CONTROL_THREAD);

  if (first_window) {
    led_light_sequence(TEST_PASSED_LED_SEQUENCE, 10);
  } else {
    led_light_sequence(HEARTBEAT_SEQUENCE, LED_SEQUENCE_FOREVER);
    heartbeat_started = true;
  }

  start_time = tx_time_get();

  while (1) {
    if (++watchdog_counter % (TX_TIMER_TICKS_PER_SECOND / 10) == 0) {
      watchdog_check_in(CONTROL_THREAD);
    }

    control.monitor_and_handle_errors();
    control.manage_state();

    // Make sure we haven't timeout for the full duty cycle
    if (control_get_timeout_status()) {
      LOG("Duty cycle timeout. Starting shutdown sequence.");
      tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);
      control.shutdown_procedure();
    }

    // Once the initial light sequence has finished, start the heartbeat
    // sequence
    if (((tx_time_get() - start_time) > (TX_TIMER_TICKS_PER_SECOND * 10)) &&
        !heartbeat_started) {
      led_light_sequence(HEARTBEAT_SEQUENCE, LED_SEQUENCE_FOREVER);
      heartbeat_started = true;
    }

    tx_thread_sleep(10);
  }
}

/*
 * @brief  gnss_thread_entry
 *         Thread that governs the GNSS processing. Note that actual message
 * processing happens in interrupt context, so this thread is just acting as the
 * traffic cop.
 *
 * @param  ULONG thread_input - unused
 * @retval void
 */
static void gnss_thread_entry(ULONG thread_input) {
  UNUSED(thread_input);
  TX_THREAD *this_thread = tx_thread_identify();
  GNSS gnss = {0};
  float *north = NULL, *east = NULL, *down = NULL;
  uSWIFT_return_code_t gnss_return_code;
  int number_of_no_sample_errors = 0;
  float last_lat = 0;
  float last_lon = 0;
  UINT tx_return;
  ULONG actual_flags;
  int timer_ticks_to_get_message =
      round(((float)TX_TIMER_TICKS_PER_SECOND /
             (float)configuration.gnss_sampling_rate) +
            50);
  uint32_t two_mins_remaining_sample_count =
      abs((2 * 60 * configuration.gnss_sampling_rate) -
          configuration.gnss_samples_per_window);
  bool two_mins_out_msg_sent = false, start_flag_sent = false;
  uint32_t total_samples = 0;

  tx_thread_sleep(10);

  if (lpuart1_init() != UART_OK) {
    gnss_error_out(&gnss, GNSS_INIT_FAILED, this_thread,
                   "GNSS UART port failed to initialize.");
  }

  // Make sure the waves thread has initialized properly before proceeding
  while (1) {
    if (waves_memory_get_raw_data_pointers(&north, &east, &down)) {
      break;
    }
    tx_thread_sleep(1);
  }

  // Init and turn on
  // ** NOTE: RF switch is managed by control thread
  gnss_init(&gnss, &configuration, device_handles.gnss_uart_handle,
            device_handles.gnss_uart_tx_dma_handle,
            device_handles.gnss_uart_rx_dma_handle, &irq_flags, &error_flags,
            &gnss_timer, north, east, down);

  gnss.on();

  tx_thread_sleep(SOFT_START_DELAY);

  // Run tests if needed
  if (tests.gnss_thread_test != NULL) {
    tests.gnss_thread_test(&gnss);
  }

  // Apply configuration through UBX_VALSET messages
  if (!gnss_apply_config(&gnss)) {
    gnss_error_out(&gnss, GNSS_CONFIGURATION_FAILED, this_thread,
                   "GNSS failed to initialize.");
  }

  // Report init success
  (void)tx_event_flags_set(&initialization_flags, GNSS_INIT_SUCCESS, TX_OR);
  LOG("GNSS initialization successful.");

  // This thread has successfully initialized, it should now be checking in with
  // the watchdog
  watchdog_register_thread(GNSS_THREAD);
  watchdog_check_in(GNSS_THREAD);

  // Start the timer for resolution stages
  gnss.start_timer(configuration.gnss_max_acquisition_wait_time);

  // Frame sync and switch to DMA circular mode
  if (gnss.sync_and_start_reception() != uSWIFT_SUCCESS) {
    // If we were unable to get good GNSS reception and start the DMA transfer
    // loop, then shutdown
    gnss_error_out(&gnss, GNSS_FRAME_SYNC_FAILED, this_thread,
                   "GNSS frame sync failed.");
  }

  // We are now running in DMA circular mode
  LOG("GNSS successfully switched to circular DMA mode.");

  // Process messages until we have resolved time
  // **NOTE: RTC will be set when time is resolved
  while (!(gnss.all_resolution_stages_complete ||
           gnss_get_timer_timeout_status())) {
    if (gnss.total_samples % 50 == 0) {
      watchdog_check_in(GNSS_THREAD);
    }

    tx_return = tx_event_flags_get(
        &irq_flags, (GNSS_MSG_RECEIVED | GNSS_MSG_INCOMPLETE), TX_OR_CLEAR,
        &actual_flags, timer_ticks_to_get_message);
    // Full message came through
    if ((tx_return == TX_SUCCESS) && !(actual_flags & GNSS_MSG_INCOMPLETE)) {
      gnss.process_message();
    }
  }

  // Failed to resolve time within alloted period
  if (gnss_get_timer_timeout_status()) {
    gnss_error_out(
        &gnss, GNSS_RESOLUTION_ERROR, this_thread,
        "GNSS failed to get a fix within alloted time of %d minutes.",
        configuration.gnss_max_acquisition_wait_time);
  }

  // Start the sample window timer
  gnss.stop_timer();
  gnss.start_timer(get_gnss_sample_window_timeout(&configuration));

  // Process messages until complete
  while (!gnss_get_sample_window_complete()) {
    total_samples = gnss_get_samples_processed();

    if (total_samples % (configuration.gnss_sampling_rate * 10) == 0) {
      watchdog_check_in(GNSS_THREAD);
    }

    if ((total_samples > 0) && (!start_flag_sent)) {

      (void)tx_event_flags_set(&complete_flags, GNSS_SAMPLING_STARTED_FIX_GOOD,
                               TX_OR);
      start_flag_sent = true;
    }

    tx_return = tx_event_flags_get(
        &irq_flags, (GNSS_MSG_RECEIVED | GNSS_MSG_INCOMPLETE), TX_OR_CLEAR,
        &actual_flags, timer_ticks_to_get_message);

    // Full message came through
    if ((tx_return == TX_SUCCESS) && !(actual_flags & GNSS_MSG_INCOMPLETE)) {
      gnss.process_message();
      number_of_no_sample_errors = 0;
    }
    // Message was dropped or incomplete -- replace with running average
    // velocities
    else if ((tx_return == TX_NO_EVENTS) ||
             (actual_flags & GNSS_MSG_INCOMPLETE)) {
      gnss_return_code = gnss.get_running_average_velocities();

      if (gnss_return_code == uSWIFT_NO_SAMPLES_ERROR) {
#warning                                                                       \
    "Likely won't use DMA Receive to Idle, rework this to reflect. Can keep GNSS_TOO_MANY_PARTIAL_MSGS,\
          but might have to check in GNSS for this. Make sure to get the instance above as well during resolution."
        // If we get a full minute worth of dropped or incomplete messages, fail
        // out
        if (++number_of_no_sample_errors ==
            configuration.gnss_sampling_rate * 60) {
          gnss_error_out(
              &gnss, GNSS_TOO_MANY_PARTIAL_MSGS, this_thread,
              "GNSS received too many partial or dropped messages: %d",
              number_of_no_sample_errors);
        }
      }

    }
    // Any other return code indicates something got corrupted. Let's hope this
    // works...
    else {
      gnss_error_out(&gnss, MEMORY_CORRUPTION_ERROR, this_thread,
                     "Memory corruption detected in GNSS thread.");
    }

    // If this evaluates to true, something hung up with GNSS sampling and we
    // were not able to get all required samples in the alloted time.
    if (gnss_get_timer_timeout_status()) {
      gnss_error_out(&gnss, GNSS_SAMPLE_WINDOW_TIMEOUT, this_thread,
                     "GNSS sample window timed out after %d minutes.",
                     get_gnss_sample_window_timeout(&configuration));
    }

    // Check if we are two mins out
    if (!two_mins_out_msg_sent &&
        (gnss_get_samples_processed() >= two_mins_remaining_sample_count)) {
      (void)tx_event_flags_set(&complete_flags,
                               GNSS_TWO_MINS_OUT_FROM_COMPLETION, TX_OR);
      two_mins_out_msg_sent = true;
    }
  }

  LOG("GNSS sample window completed.");

  watchdog_check_in(GNSS_THREAD);

  // Stop the timer, turn off sensor
  gnss.stop_timer();
  gnss.off();

  // Fill in the location fields in the SBD message
  gnss.get_location(&last_lat, &last_lon);
  memcpy(&sbd_message.Lat, &last_lat, sizeof(float));
  memcpy(&sbd_message.Lon, &last_lon, sizeof(float));

  // We were using the "port" field to encode how many samples were averaged
  // divided by 10, but now we are using it to store the firmware version, done
  // in Iridium thread. Leaving this in case we decide to use it again.
  //  sbd_port = ((gnss.total_samples_averaged / 10) >= 255) ?
  //      255 : (gnss.total_samples_averaged / 10);
  //  memcpy (&sbd_message.port, &sbd_port, sizeof(uint8_t));

  // Deinit -- this will shut down UART port and DMA channels
  gnss_deinit();

  watchdog_check_in(GNSS_THREAD);

  (void)file_system_server_save_gnss_raw(&gnss);
  (void)file_system_server_save_gnss_track(&gnss);

  watchdog_check_in(GNSS_THREAD);
  watchdog_deregister_thread(GNSS_THREAD);

  // The logger gets weird if there is no break here...
  tx_thread_sleep(LOGGER_MAX_TICKS_TO_TX_MSG);

  (void)tx_event_flags_set(&complete_flags, GNSS_THREAD_COMPLETED_SUCCESSFULLY,
                           TX_OR);
  tx_thread_terminate(this_thread);
}

/*
 * @brief  ct_thread_entry
 *         This thread will handle the CT sensor, capture readings, and getting
 * averages..
 *
 * @param  ULONG thread_input - unused
 * @retval void
 */
static void ct_thread_entry(ULONG thread_input) {
  UNUSED(thread_input);
  TX_THREAD *this_thread = tx_thread_identify();
  CT ct = {0};
  ct_sample ct_readings = {0};
  uSWIFT_return_code_t ct_return_code;
  uint32_t ct_parsing_error_counter = 0;
  real16_T half_salinity, half_temp;
  int32_t ct_thread_timeout = 3; // mins

  tx_thread_sleep(10);

  if (usart1_init() != UART_OK) {
    ct_error_out(&ct, CT_INIT_FAILED, this_thread,
                 "CT UART port failed to initialize.");
  }

  // Set the mean salinity and temp values to error values in the event the
  // sensor fails
  half_salinity.bitPattern = TELEMETRY_FIELD_ERROR_CODE;
  half_temp.bitPattern = TELEMETRY_FIELD_ERROR_CODE;

  memcpy(&sbd_message.mean_salinity, &half_salinity, sizeof(real16_T));
  memcpy(&sbd_message.mean_temp, &half_temp, sizeof(real16_T));

  ct_init(&ct, &configuration, device_handles.ct_uart_handle,
          device_handles.ct_uart_tx_dma_handle,
          device_handles.ct_uart_rx_dma_handle, &ct_uart_sema, &error_flags,
          &ct_timer);

  ct.on();
  tx_thread_sleep(SOFT_START_DELAY);
  //
  // Run tests if needed
  if (tests.ct_thread_test != NULL) {
    tests.ct_thread_test(&ct);
  }

  if (!ct_self_test(&ct, false, &ct_readings)) {
    ct_error_out(&ct, CT_INIT_FAILED, this_thread, "CT self test failed.");
  }

  LOG("CT initialization complete. Temp = %3f, Salinity = %3f",
      ct_readings.temp, ct_readings.salinity);
  (void)tx_event_flags_set(&initialization_flags, CT_INIT_SUCCESS, TX_OR);

  // Control will resume when ready
  ct.off();
  tx_thread_suspend(this_thread);

  // Control thread resumes this thread
  ct.on();
  ct.start_timer(ct_thread_timeout);
  watchdog_register_thread(CT_THREAD);
  watchdog_check_in(CT_THREAD);

  tx_thread_sleep(SOFT_START_DELAY);

  // Turn on the CT sensor, warm it up, and frame sync
  if (!ct_self_test(&ct, true, &ct_readings)) {
    ct_error_out(&ct, CT_INIT_FAILED, this_thread, "CT self test failed.");
  }

  LOG("CT sample window started.");

  // Take our samples
  while (1) {
    watchdog_check_in(CT_THREAD);

    ct_return_code = ct.parse_sample();

    if (ct_return_code == uSWIFT_PROCESSING_ERROR) {
      ct_parsing_error_counter++;
    }

    if ((ct_parsing_error_counter >= 10) ||
        (ct_return_code == uSWIFT_IO_ERROR)) {
      // If there are too many parsing errors or a UART error occurs, stop
      // trying
      ct_error_out(&ct, CT_SAMPLING_ERROR, this_thread,
                   "CT sensor too many parsing errors, shutting down.");
    }

    // If this evaluates to true, something hung up with CT sampling and we were
    // not able to get all required samples in the alloted time.
    if (ct_get_timeout_status()) {
      ct_error_out(&ct, CT_SAMPLE_WINDOW_TIMEOUT, this_thread,
                   "CT sample window timed out after %d minutes.",
                   ct_thread_timeout);
    }

    if (ct_return_code == uSWIFT_DONE_SAMPLING) {
      break;
    }
  }

  LOG("CT sample window completed.");

  watchdog_check_in(CT_THREAD);

  // Turn off the CT sensor
  ct.stop_timer();
  ct.off();

  // Deinit UART and DMA to prevent spurious interrupts
  ct_deinit();

  // Got our samples, now average them
  ct_return_code = ct.get_averages(&ct_readings);
  // Make sure something didn't go terribly wrong
  if (ct_return_code == uSWIFT_NO_SAMPLES_ERROR) {
    ct_error_out(&ct, CT_SAMPLING_ERROR, this_thread,
                 "CT sensor did not collect enough samples.");
  }

  // Now set the mean salinity and temp values to the real ones
  half_salinity = doubleToHalf(ct_readings.salinity);
  half_temp = doubleToHalf(ct_readings.temp);

  memcpy(&sbd_message.mean_salinity, &half_salinity, sizeof(real16_T));
  memcpy(&sbd_message.mean_temp, &half_temp, sizeof(real16_T));

  watchdog_check_in(CT_THREAD);

  (void)file_system_server_save_ct_raw(&ct);

  watchdog_check_in(CT_THREAD);
  watchdog_deregister_thread(CT_THREAD);

  // The logger gets weird if there is no break here...
  tx_thread_sleep(LOGGER_MAX_TICKS_TO_TX_MSG);

  (void)tx_event_flags_set(&complete_flags, CT_THREAD_COMPLETED_SUCCESSFULLY,
                           TX_OR);
  tx_thread_terminate(this_thread);
}

/*
 * @brief  temperature_thread_entry
 *         This thread will handle the temperature sensor, capture readings, and
 * getting averages.
 *
 * @param  ULONG thread_input - unused
 * @retval void
 */
static void temperature_thread_entry(ULONG thread_input) {
  UNUSED(thread_input);
  TX_THREAD *this_thread = tx_thread_identify();
  Temperature temperature = {0};
  uSWIFT_return_code_t temp_return_code = uSWIFT_SUCCESS;
  float self_test_reading = 0.0f, sampling_reading = 0.0f;
  real16_T half_temp = {0};
  int32_t temperature_thread_timeout = 2; // minutes
  int32_t fail_counter = 0, max_retries = 10;

  tx_thread_sleep(10);

  // Set the mean salinity and temp values to error values in the event the
  // sensor fails
  half_temp.bitPattern = TEMPERATURE_VALUES_ERROR_CODE;

  memcpy(&sbd_message.mean_temp, &half_temp, sizeof(real16_T));

  temperature_init(&temperature, &configuration, &error_flags,
                   &temperature_timer, true);

  temperature.on();
  tx_thread_sleep(SOFT_START_DELAY);
  //
  // Run tests if needed
  if (tests.temperature_thread_test != NULL) {
    tests.temperature_thread_test(NULL);
  }

  if (!temperature_self_test(&temperature, &self_test_reading)) {
    temperature_error_out(&temperature, TEMPERATURE_INIT_FAILED, this_thread,
                          "Temperature self test failed.");
  }

  LOG("Temperature initialization complete. Temp = %.1f degC, %.1f degF.",
      self_test_reading, ((self_test_reading * 9) / 5) + 32);
  (void)tx_event_flags_set(&initialization_flags, TEMPERATURE_INIT_SUCCESS,
                           TX_OR);

  // No need to turn off temperature sensor, it automatically enters standby
  // mode when not actively taking a measurement and the FET dissipates far more
  // power than the sensor in standby mode.
  temperature.off();
  tx_thread_suspend(this_thread);

  // Control thread resumes this thread
  temperature.start_timer(temperature_thread_timeout);
  watchdog_register_thread(TEMPERATURE_THREAD);
  watchdog_check_in(TEMPERATURE_THREAD);

  temperature.on();
  tx_thread_sleep(SOFT_START_DELAY);

  LOG("Temperature sample window started.");

  while (fail_counter < max_retries) {
    watchdog_check_in(TEMPERATURE_THREAD);

    temp_return_code = temperature.take_samples(&sampling_reading);

    if (temp_return_code == uSWIFT_SUCCESS) {
      break;
    }

    if (temperature_get_timeout_status()) {
      temperature_error_out(
          &temperature, TEMPERATURE_SAMPLE_WINDOW_TIMEOUT, this_thread,
          "Temperature sample window timed out after %d minutes.",
          temperature_thread_timeout);
    }

    fail_counter++;
  }

  if (fail_counter == max_retries) {
    temperature_error_out(&temperature, TEMPERATURE_SAMPLING_ERROR, this_thread,
                          "Unable to get readings from temperature sensor "
                          "after %d failed attempts.",
                          max_retries);
  }

  LOG("Temperature sample window completed.");

  half_temp = floatToHalf(sampling_reading);

  temperature.stop_timer();
  temperature.off();

  memcpy(&sbd_message.mean_temp, &half_temp, sizeof(real16_T));

  watchdog_check_in(TEMPERATURE_THREAD);

  (void)file_system_server_save_temperature_raw(&temperature);

  watchdog_check_in(TEMPERATURE_THREAD);
  watchdog_deregister_thread(TEMPERATURE_THREAD);

  // The logger gets weird if there is no break here...
  tx_thread_sleep(LOGGER_MAX_TICKS_TO_TX_MSG);

  (void)tx_event_flags_set(&complete_flags,
                           TEMPERATURE_THREAD_COMPLETED_SUCCESSFULLY, TX_OR);
  tx_thread_terminate(this_thread);
}

/*
 * @brief  light_thread_entry
 *         This thread will manage the Light sensor
 *
 * @param  ULONG thread_input - unused
 * @retval void
 */
static void light_thread_entry(ULONG thread_input) {
  UNUSED(thread_input);
  TX_THREAD *this_thread = tx_thread_identify();
  uSWIFT_return_code_t light_ret;
  Light_Sensor light = {0};
  sbd_message_type_54_element sbd_msg_element = {0};
  light_raw_counts raw_counts;
  int32_t light_thread_timeout =
      get_gnss_sample_window_timeout(&configuration); // Same timeout as GNSS
  ULONG sample_start_time = 0;
  ULONG thread_sleep_time = 0;

  tx_thread_sleep(10);

  light_sensor_init(&light, &configuration, &(light_sensor_sample_buffer[0]),
                    &light_timer);

  //
  // Run tests if needed
  if (tests.light_thread_test != NULL) {
    tests.light_thread_test(NULL);
  }

  light.on();
  tx_thread_sleep(SOFT_START_DELAY);

  if (!light_self_test(&light)) {
    light_error_out(&light, LIGHT_INIT_FAILED, this_thread,
                    "Light sensor self test failed.");
  }

  light.get_raw_measurements(&raw_counts);

  LOG("Light sensor initialization complete. Raw count:\r\n"
      "F1 = %hu, F2 = %hu, F3 = %hu, F4 = %hu, F5 = %hu, F6 = %hu, "
      "F7 = %hu, F8 = %hu, NIR = %hu, Clear = %hu, Dark = %hu",
      raw_counts.f1_chan, raw_counts.f2_chan, raw_counts.f3_chan,
      raw_counts.f4_chan, raw_counts.f5_chan, raw_counts.f6_chan,
      raw_counts.f7_chan, raw_counts.f8_chan, raw_counts.nir_chan,
      raw_counts.clear_chan, raw_counts.dark_chan);

  (void)tx_event_flags_set(&initialization_flags, LIGHT_INIT_SUCCESS, TX_OR);

  light.idle();
  light.off();
  tx_thread_suspend(this_thread);

  // Control thread resumes this thread
  light.start_timer(light_thread_timeout);
  watchdog_register_thread(LIGHT_THREAD);
  watchdog_check_in(LIGHT_THREAD);

  light.on();
  tx_thread_sleep(SOFT_START_DELAY);

  if (!light_self_test(&light)) {
    light_error_out(&light, LIGHT_INIT_FAILED, this_thread,
                    "Light sensor failed prior to sampling.");
  }

  LOG("Light sample window started.");

  // Take our samples
  while (1) {

    if (light_get_timeout_status()) {
      light_error_out(&light, LIGHT_SAMPLE_WINDOW_TIMEOUT, this_thread,
                      "Light sample window timed out after %d minutes.",
                      light_thread_timeout);
    }

    sample_start_time = tx_time_get();

    watchdog_check_in(LIGHT_THREAD);

    light_ret = light.read_all_channels();

    if (uSWIFT_SUCCESS == light_ret) {
      light_ret = light.process_measurements();
    } else {
      light.failed_samples++;
    }

    int num_samples = light.valid_samples + light.failed_samples;
    if (num_samples == light.global_config->total_light_samples) {
      light.get_samples_averages();
      break;
    }

    thread_sleep_time =
        ((sample_start_time + (TX_TIMER_TICKS_PER_SECOND * 2)) - tx_time_get());

    // Check for wrap around
    if (thread_sleep_time > (TX_TIMER_TICKS_PER_SECOND * 2)) {
      thread_sleep_time = 0;
    }

    // Run at 0.5Hz
    tx_thread_sleep(thread_sleep_time);
  }

  LOG("Light sample window completed. %d / %d samples succeeded.",
      light.valid_samples, light.valid_samples + light.failed_samples);

  light.idle();
  light.off();

  light.assemble_telemetry_message_element(&sbd_msg_element);
  persistent_ram_save_message(LIGHT_TELEMETRY, (uint8_t *)&sbd_msg_element);

  watchdog_check_in(LIGHT_THREAD);

  (void)file_system_server_save_light_raw(&light);

  watchdog_check_in(LIGHT_THREAD);
  watchdog_deregister_thread(LIGHT_THREAD);

  // The logger gets weird if there is no break here...
  tx_thread_sleep(LOGGER_MAX_TICKS_TO_TX_MSG);

  (void)tx_event_flags_set(&complete_flags, LIGHT_THREAD_COMPLETED_SUCCESSFULLY,
                           TX_OR);
  tx_thread_terminate(this_thread);
}

/*
 * @brief  turbidity_thread_entry
 *         This thread will manage the turbidity sensor
 *
 * @param  ULONG thread_input - unused
 * @retval void
 */
static void turbidity_thread_entry(ULONG thread_input) {
  UNUSED(thread_input);
  TX_THREAD *this_thread = tx_thread_identify();
  Turbidity_Sensor obs = {0};
  uint16_t amb, prox;
  uSWIFT_return_code_t ret;
  sbd_message_type_53_element sbd_msg_element = {0};
  int32_t turbidity_thread_timeout =
      get_gnss_sample_window_timeout(&configuration); // Same timeout as GNSS

  tx_thread_sleep(10);

  turbidity_sensor_init(&obs, &configuration, &turbidity_timer,
                        &turbidity_sensor_ambient_buffer[0],
                        &turbidity_sensor_proximity_buffer[0]);

  obs.on();

  tx_thread_sleep(SOFT_START_DELAY);

  //
  // Run tests if needed
  if (tests.turbidity_thread_test != NULL) {
    tests.turbidity_thread_test(NULL);
  }

  if (!turbidity_self_test(&obs, &amb, &prox)) {
    turbidity_error_out(&obs, TURBIDITY_INIT_FAILED, this_thread,
                        "Turbidity sensor self test failed.");
  }

  LOG("Turbidity sensor initialization complete. Ambient raw counts: %d, "
      "Proximity raw counts: %d.",
      amb, prox);

  (void)tx_event_flags_set(&initialization_flags, TURBIDITY_INIT_SUCCESS,
                           TX_OR);

  obs.idle();
  obs.off();

  tx_thread_suspend(this_thread);

  // Control thread resumes this thread
  obs.start_timer(turbidity_thread_timeout);
  watchdog_register_thread(TURBIDITY_THREAD);
  watchdog_check_in(TURBIDITY_THREAD);

  obs.on();
  tx_thread_sleep(SOFT_START_DELAY);

  if (!turbidity_self_test(&obs, &amb, &prox)) {
    turbidity_error_out(&obs, TURBIDITY_INIT_FAILED, this_thread,
                        "Turbidity sensor self failed prior to sampling.");
  }

  turbidity_reset_sample_counter();

  LOG("Turbidity sample window started.");

  // Take our samples
  while (1) {

    watchdog_check_in(TURBIDITY_THREAD);

    ret = obs.take_measurement(false);

    if (ret == uSWIFT_DONE_SAMPLING) {
      break;
    } else if (ret != uSWIFT_SUCCESS) {
      turbidity_error_out(&obs, TURBIDITY_SAMPLING_ERROR, this_thread,
                          "Error occurred when reading turbidity sensor.");
    }

    if (turbidity_get_timeout_status()) {
      turbidity_error_out(&obs, TURBIDITY_SAMPLING_ERROR, this_thread,
                          "Turbidity sample window timed out after %d minutes.",
                          turbidity_thread_timeout);
    }

    // Sleep the rest of the second away
    tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND -
                    (tx_time_get() % TX_TIMER_TICKS_PER_SECOND));
  }

  LOG("Turbidity sample window completed.");

  obs.off();

  obs.assemble_telemetry_message_element(&sbd_msg_element);
  persistent_ram_save_message(TURBIDITY_TELEMETRY, (uint8_t *)&sbd_msg_element);

  watchdog_check_in(TURBIDITY_THREAD);

  (void)file_system_server_save_turbidity_raw(&obs);

  watchdog_check_in(TURBIDITY_THREAD);
  watchdog_deregister_thread(TURBIDITY_THREAD);

  // The logger gets weird if there is no break here...
  tx_thread_sleep(LOGGER_MAX_TICKS_TO_TX_MSG);

  (void)tx_event_flags_set(&complete_flags,
                           TURBIDITY_THREAD_COMPLETED_SUCCESSFULLY, TX_OR);
  tx_thread_terminate(this_thread);
}

/*
 * @brief  waves_thread_entry
 *         This thread will run the NEDWaves algorithm.
 *
 * @param  ULONG thread_input - unused
 * @retval void
 */
static void waves_thread_entry(ULONG thread_input) {
  //@formatter:off
  UNUSED(thread_input);
  TX_THREAD *this_thread = &waves_thread;
  NEDWaves_memory waves_mem = {0};
  // Function return parameters
  real16_T E[42] = {0};
  real16_T Dp = {0};
  real16_T Hs = {0};
  real16_T Tp = {0};
  real16_T b_fmax = {0};
  real16_T b_fmin = {0};
  signed char a1[42] = {0};
  signed char a2[42] = {0};
  signed char b1[42] = {0};
  signed char b2[42] = {0};
  unsigned char check[42] = {0};
  //@formatter:on

  tx_thread_sleep(1);

  if (!waves_memory_pool_init(&waves_mem, &configuration,
                              &(waves_byte_pool_buffer[0]),
                              WAVES_MEM_POOL_SIZE)) {
    waves_error_out(WAVES_INIT_FAILED, this_thread,
                    "NED Waves memory pool failed to initialize.");
  }

  (void)tx_event_flags_set(&initialization_flags, WAVES_THREAD_INIT_SUCCESS,
                           TX_OR);
  LOG("NED Waves initialization successful.");

  // Set the SBD fields to error values in the event GNSS has a failure
  Dp.bitPattern = TELEMETRY_FIELD_ERROR_CODE;
  Hs.bitPattern = TELEMETRY_FIELD_ERROR_CODE;
  Tp.bitPattern = TELEMETRY_FIELD_ERROR_CODE;
  b_fmax.bitPattern = TELEMETRY_FIELD_ERROR_CODE;
  b_fmin.bitPattern = TELEMETRY_FIELD_ERROR_CODE;

  memcpy(&sbd_message.Hs, &Hs, sizeof(real16_T));
  memcpy(&sbd_message.Tp, &Tp, sizeof(real16_T));
  memcpy(&sbd_message.Dp, &Dp, sizeof(real16_T));
  memcpy(&sbd_message.f_min, &b_fmin, sizeof(real16_T));
  memcpy(&sbd_message.f_max, &b_fmax, sizeof(real16_T));

  tx_thread_suspend(this_thread);

  // Control thread resumes this thread
  watchdog_register_thread(WAVES_THREAD);
  watchdog_check_in(WAVES_THREAD);

  // Run tests if needed
  if (tests.waves_thread_test != NULL) {
    tests.waves_thread_test(&waves_mem);
  }

  watchdog_check_in(WAVES_THREAD);

  LOG("Running NEDWaves.");

  // Call the entry-point 'NEDwaves_memlight'.
  NEDwaves_memlight(waves_mem.north, waves_mem.east, waves_mem.down,
                    gnss_get_sample_window_frequency(), &Hs, &Tp, &Dp, E,
                    &b_fmin, &b_fmax, a1, b1, a2, b2, check);

  emxDestroyArray_real32_T(waves_mem.north);
  emxDestroyArray_real32_T(waves_mem.east);
  emxDestroyArray_real32_T(waves_mem.down);

  // Done with dynamic memory requirements for NEDWaves, delete the memory pool
  (void)waves_memory_pool_delete();

  LOG("NEDWaves complete.");

  memcpy(&sbd_message.Hs, &Hs, sizeof(real16_T));
  memcpy(&sbd_message.Tp, &Tp, sizeof(real16_T));
  memcpy(&sbd_message.Dp, &Dp, sizeof(real16_T));
  memcpy(&(sbd_message.E_array[0]), &(E[0]), 42 * sizeof(real16_T));
  memcpy(&sbd_message.f_min, &b_fmin, sizeof(real16_T));
  memcpy(&sbd_message.f_max, &b_fmax, sizeof(real16_T));
  memcpy(&(sbd_message.a1_array[0]), &(a1[0]), 42 * sizeof(signed char));
  memcpy(&(sbd_message.b1_array[0]), &(b1[0]), 42 * sizeof(signed char));
  memcpy(&(sbd_message.a2_array[0]), &(a2[0]), 42 * sizeof(signed char));
  memcpy(&(sbd_message.b2_array[0]), &(b2[0]), 42 * sizeof(signed char));
  memcpy(&(sbd_message.cf_array[0]), &(check[0]), 42 * sizeof(unsigned char));

  watchdog_check_in(WAVES_THREAD);
  watchdog_deregister_thread(WAVES_THREAD);

  // The logger gets weird if there is no break here...
  tx_thread_sleep(LOGGER_MAX_TICKS_TO_TX_MSG);

  (void)tx_event_flags_set(&complete_flags, WAVES_THREAD_COMPLETED_SUCCESSFULLY,
                           TX_OR);
  tx_thread_terminate(this_thread);
}

/*
 * @brief  iridium_thread_entry
 *         This thread will handle message sending via Iridium modem.
 *
 * @param  ULONG thread_input - unused
 * @retval void
 */
static void iridium_thread_entry(ULONG thread_input) {
  UNUSED(thread_input);
  TX_THREAD *this_thread = &iridium_thread;
  Iridium iridium = {0};
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;
  int32_t iridium_thread_timeout = configuration.iridium_max_transmit_time;
  char ascii_7 = '7';
  uint8_t sbd_type = 52;
  uint16_t sbd_size = 331;
  // We're using the Type 52 field "port" to hold firmware version
  microSWIFT_firmware_version_t sbd_port = {0};
  float sbd_timestamp = 0;
  uint32_t error_bits = 0;
  struct tm time_struct = {0};
  time_t time_now = 0;
  uint8_t msg_buffer[IRIDIUM_SBD_MAX_LENGTH + IRIDIUM_CHECKSUM_LENGTH] = {0};
  uint8_t *msg_ptr = (uint8_t *)&sbd_message;
  bool current_message_sent = false;
  uint32_t next_message_type;
  uint32_t next_message_size = 0;
  char *log_str = NULL;
  ULONG start_time = 0;

  tx_thread_sleep(10);

  if (uart4_init() != UART_OK) {
    iridium_error_out(&iridium, IRIDIUM_INIT_ERROR, this_thread,
                      "Iridium UART port failed to initialize.");
  }

  iridium_init(&iridium, &configuration, device_handles.iridium_uart_handle,
               &iridium_uart_sema, device_handles.iridium_uart_tx_dma_handle,
               device_handles.iridium_uart_rx_dma_handle, &iridium_timer,
               &error_flags);

  iridium.on();
  iridium.wake();
  start_time = tx_time_get();
  iridium.charge_caps(IRIDIUM_TOP_UP_CAP_CHARGE_TIME);

  if (!iridium_apply_config(&iridium)) {
    iridium_error_out(&iridium, IRIDIUM_INIT_ERROR, this_thread,
                      "Iridium modem failed to initialize.");
  }

  LOG("Iridium modem initialized successfully.");
  (void)tx_event_flags_set(&initialization_flags, IRIDIUM_INIT_SUCCESS, TX_OR);

  //
  // Run tests if needed
  if (tests.iridium_thread_test != NULL) {
    tests.iridium_thread_test(&iridium);
  }

  // Finish charging the caps
  ULONG end_time = tx_time_get();
  if (end_time < start_time) {
    // timer wrapped around!
  } else {
    ULONG elapsed_time = end_time - start_time;
    LOG("Iridium initialization took %ul ticks; need %ul to fully charge.",
        elapsed_time, IRIDIUM_INITIAL_CAP_CHARGE_TIME);
    if (elapsed_time < IRIDIUM_INITIAL_CAP_CHARGE_TIME) {
      ULONG ticks_remaining = IRIDIUM_INITIAL_CAP_CHARGE_TIME - elapsed_time;
      tx_thread_sleep(ticks_remaining);
    }
  }

  iridium.sleep();

  tx_thread_suspend(this_thread);

  // Control thread resumes this thread
  watchdog_register_thread(IRIDIUM_THREAD);
  watchdog_check_in(IRIDIUM_THREAD);

  iridium.wake();
  iridium.charge_caps(IRIDIUM_TOP_UP_CAP_CHARGE_TIME);

  iridium.start_timer(iridium_thread_timeout);

  // finish filling out the SBD message
  rtc_server_get_time(&time_struct, IRIDIUM_REQUEST_COMPLETE);
  time_now = mktime(&time_struct);
  sbd_timestamp = (float)time_now;
  persistent_ram_get_firmware_version(&sbd_port);
  error_bits = control_get_accumulated_error_flags();
  memcpy(&sbd_message.legacy_number_7, &ascii_7, sizeof(char));
  memcpy(&sbd_message.type, &sbd_type, sizeof(uint8_t));
  memcpy(&sbd_message.size, &sbd_size, sizeof(uint16_t));
  memcpy(&sbd_message.port, (uint8_t *)&sbd_port, sizeof(uint8_t));
  memcpy(&sbd_message.timestamp, &sbd_timestamp, sizeof(float));
  memcpy(&sbd_message.error_bits, &error_bits, sizeof(uint32_t));

  msg_ptr = (uint8_t *)&sbd_message;

  if (!iridium_self_test(&iridium)) {
    // Need to save the message
    error_bits = get_current_flags(&error_flags);
    error_bits |= IRIDIUM_INIT_ERROR;
    persistent_ram_save_message(WAVES_TELEMETRY, (uint8_t *)&sbd_message);
    iridium_error_out(&iridium, IRIDIUM_UART_COMMS_ERROR, this_thread,
                      "Iridium modem UART communication error.");
  }

  // copy over the message into the buffer
  memcpy(&(msg_buffer[0]), &sbd_message, sizeof(sbd_message_type_52));

  // Send the current message followed by any cached messages, until time runs
  // out
  while (!iridium_get_timeout_status()) {

    watchdog_check_in(IRIDIUM_THREAD);

    // Report how many messages of each type have been queued.
    uint32_t num_waves_msgs =
        persistent_ram_get_num_msgs_enqueued(WAVES_TELEMETRY);
    uint32_t num_turbidity_msgs =
        persistent_ram_get_num_msgs_enqueued(TURBIDITY_TELEMETRY);
    uint32_t num_light_msgs =
        persistent_ram_get_num_msgs_enqueued(LIGHT_TELEMETRY);
    uint32_t num_accel_msgs =
        persistent_ram_get_num_msgs_enqueued(ACCELEROMETER_TELEMETRY);
    LOG("Iridium trying to transmit! Queue lengths: waves = %lu, accel = %lu, "
        "turbidity = %lu, light = %lu",
        num_waves_msgs, num_accel_msgs, num_turbidity_msgs, num_light_msgs);

    if (!current_message_sent) {
      LOG("Attempting transmission of NEDWaves telemetry...");
      ret = iridium.transmit_message(&(msg_buffer[0]),
                                     sizeof(sbd_message_type_52));
      if (ret == uSWIFT_SUCCESS) {
        current_message_sent = true;
      } else if (ret == uSWIFT_TIMEOUT) {
        break;
      }

      if (iridium_get_configuration_received_status()) {
        (void)iridium.receive_configuration();
      }

      continue;
    }

    next_message_type = get_next_telemetry_message(&msg_ptr, &configuration);
    if (next_message_type == NO_MESSAGE) {
      LOG("No cached messages ready to transmit.");
      break;
    }

    switch (next_message_type) {
    case WAVES_TELEMETRY:
      next_message_size = sizeof(sbd_message_type_52);
      log_str = "NEDWaves telemetry";
      break;
    case TURBIDITY_TELEMETRY:
      next_message_size = sizeof(sbd_message_type_53);
      log_str = "OBS telemetry";
      break;
    case LIGHT_TELEMETRY:
      next_message_size = sizeof(sbd_message_type_54);
      log_str = "Light telemetry";
      break;
    case ACCELEROMETER_TELEMETRY:
      next_message_size = sizeof(sbd_message_type_55);
      log_str = "Accelerometer telemetry";
      break;
    case OTA_ACK_MESSAGE:
      next_message_size = sizeof(sbd_message_type_99);
      log_str = "OTA config acknowledgment";
      break;

    default:
      next_message_size = 0;
      log_str = "Unknown";
    }

    if (next_message_size > 0) {
      LOG("Attempting transmission of %s...", log_str);

      if (iridium.transmit_message(msg_ptr, next_message_size) ==
          uSWIFT_SUCCESS) {
        if (iridium_get_configuration_received_status()) {
          // NOTE(LEL): This will keep trying on successive message
          // transmissions if the first one fails, since the flag won't be
          // reset. Is that what we want? I doubt the modem will still
          // be holding on to the message if we request it then the receive DMA
          // fails...
          (void)iridium.receive_configuration();
        }

        persistent_ram_delete_message_element(next_message_type, msg_ptr);
      }
    }
  }

  watchdog_check_in(IRIDIUM_THREAD);

  tx_thread_sleep(LOGGER_MAX_TICKS_TO_TX_MSG);

  if (!current_message_sent) {
    // Update the error bits
    error_bits = control_get_accumulated_error_flags();
    memcpy(&sbd_message.error_bits, &error_bits, sizeof(uint32_t));
    // Save the message
    persistent_ram_save_message(WAVES_TELEMETRY, msg_ptr);

    tx_thread_sleep(10);
  }

  // Turn off the modem
  iridium.stop_timer();
  iridium.sleep();

  iridium_deinit();

  watchdog_check_in(IRIDIUM_THREAD);
  watchdog_deregister_thread(IRIDIUM_THREAD);

  // The logger gets weird if there is no break here...
  tx_thread_sleep(LOGGER_MAX_TICKS_TO_TX_MSG);

  (void)tx_event_flags_set(&complete_flags,
                           IRIDIUM_THREAD_COMPLETED_SUCCESSFULLY, TX_OR);
  tx_thread_terminate(this_thread);
}

static void accel_thread_entry(ULONG thread_input) {
  UNUSED(thread_input);

  TX_THREAD *this_thread = &accel_thread;

  // Calling "LOG" here causes repeated restarts of the program.
  // https://github.com/SASlabgroup/microSWIFT_V2.2_Firmware/issues/1
  // LOG("accel_thread_entry");
  // Calling it after a sleep is fine
  // tx_thread_sleep(100);
  // LOG("accel_thread_entry");

  Accelerometer accel = {0};
  accelerometer_init(&accel, device_handles.expansion_uart_handle,
                     &expansion_uart_sema);

  accel.power_on();
  int32_t ret;
  ret = usart2_init();
  if (UART_OK != ret) {
    accel_error_out(&accel, ACCELEROMETER_INIT_FAILED, this_thread,
                    "Accel UART port failed to initialize");
  }

  tx_thread_sleep(3 * TX_TIMER_TICKS_PER_SECOND);

  // Additional sleep to attach debugger.
  // tx_thread_sleep(30 * TX_TIMER_TICKS_PER_SECOND);

  accel_self_test_result_t self_test_result;
  ret = accel.self_test(&self_test_result);
  if (uSWIFT_SUCCESS != ret) {
    accel_error_out(&accel, ACCELEROMETER_INIT_FAILED, this_thread,
                    "Accelerometer self test failed with error: (%d)",
                    (int)ret);

  } else {
    LOG("Accelerometer self test succeeded. timestamp = %d, X = %0.2f, Y = "
        "%0.2f, Z = %0.2f (g)",
        self_test_result.timestamp, self_test_result.x_g, self_test_result.y_g,
        self_test_result.z_g);
    (void)tx_event_flags_set(&initialization_flags, ACCELEROMETER_INIT_SUCCESS,
                             TX_OR);
  }

  tx_thread_sleep(10 * TX_TIMER_TICKS_PER_SECOND);

  ret = usart2_deinit();
  accel.power_off();

  // Suspend for now, will be woken up when GNSS has initialized
  tx_thread_suspend(this_thread);

  tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / 10);

  LOG("Resuming accelerometer thread");
  accel.power_on();
  ret = usart2_init();
  if (UART_OK != ret) {
    accel_error_out(&accel, ACCELEROMETER_INIT_FAILED, this_thread,
                    "Accel UART port failed to initialize");
  }

  // TODO: Initialize time on accel board / confirm UART comms

  tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND); // Time for board to wake back up.

  accel.start_sampling();
  uint32_t timestamp = (uint32_t)get_system_time();

  LOG("Accelerometer sample window started.");
  sbd_message_type_55 accel_msg = {0};

  // TODO: Set timeout for overall thread:
  //   e.g.   iridium.start_timer(iridium_thread_timeout);

  // Read bytes; package up and add to iridium queue.
  ret = accel.parse_waves(&accel_msg);
  if (uSWIFT_SUCCESS != ret) {
    accel_error_out(&accel, ACCELEROMETER_SAMPLING_ERROR, this_thread,
                    "Acceleration-based waves returned with error code: %d",
                    (int)ret);
  }

  LOG("Accelerometer-based waves computations completed.");
  accel.uart_deinit();
  accel.power_off();

  char ascii_7 = '7';
  uint8_t accel_type = 55;
  int32_t lat = 0;
  int32_t lon = 0;
  // NOTE(LEL): It looks like there's a call in gnss.c that does this division
  // for us? So should we be using get_location(...) instead?
  gnss_get_current_lat_lon(&lat, &lon);
  float msg_lat, msg_lon;
  msg_lat = (float)lat / LAT_LON_CONVERSION_FACTOR;
  msg_lon = (float)lon / LAT_LON_CONVERSION_FACTOR;

  memcpy(&accel_msg.legacy_number_7, &ascii_7, sizeof(uint8_t));
  memcpy(&accel_msg.type, &accel_type, sizeof(uint8_t));
  memcpy(&accel_msg.timestamp, &timestamp, sizeof(uint32_t));
  memcpy(&accel_msg.latitude, &msg_lat, sizeof(float));
  memcpy(&accel_msg.longitude, &msg_lon, sizeof(float));

  LOG("Received accelerometer message:");
  LOG("....X min/mean/max: %0.4f / %0.4f / %0.4f",
      halfToFloat(accel_msg.min_x_accel), halfToFloat(accel_msg.mean_x_accel),
      halfToFloat(accel_msg.max_x_accel));
  LOG("....Y min/mean/max: %0.4f / %0.4f / %0.4f",
      halfToFloat(accel_msg.min_y_accel), halfToFloat(accel_msg.mean_y_accel),
      halfToFloat(accel_msg.max_y_accel));
  LOG("....Z min/mean/max: %0.4f / %0.4f / %0.4f",
      halfToFloat(accel_msg.min_z_accel), halfToFloat(accel_msg.mean_z_accel),
      halfToFloat(accel_msg.max_z_accel));
  LOG("Lat = %0.2f, Lon = %0.2f", accel_msg.latitude, accel_msg.longitude);

  persistent_ram_save_message(ACCELEROMETER_TELEMETRY, (uint8_t *)&accel_msg);

  // TODO: Add saving the data. I'm not yet sure whether we need a whole
  //       struct to hold the accelerometer-related variables ...
  //    Of course, this woudldn't be true raw data, just the chunk
  //    that is sent
  // (void)file_system_server_save_accelerometer_raw(&accel);

  tx_thread_sleep(LOGGER_MAX_TICKS_TO_TX_MSG);

  (void)tx_event_flags_set(&complete_flags,
                           ACCELEROMETER_THREAD_COMPLETED_SUCCESSFULLY, TX_OR);
  tx_thread_terminate(this_thread);
}
// clang-format off
/* USER CODE END 1 */
