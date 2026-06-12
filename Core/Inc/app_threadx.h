/* USER CODE BEGIN Header */
// clang-format on
/**
 ******************************************************************************
 * @file    app_threadx.h
 * @author  MCD Application Team
 * @brief   ThreadX applicative header file
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
 */
// clang-format off
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __APP_THREADX_H
#define __APP_THREADX_H
#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "tx_api.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
// clang-format on
#if SD_CARD_ENABLED
#include "sdmmc.h"
#endif

#include "i2c.h"
#include "octospi.h"
#include "spi.h"
#include "threadx_support.h"
#include "usart.h"

// clang-format off
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
// clang-format on

#define SOFT_START_DELAY 50U
#define ONE_SECOND 1000U

// @formatter:off
// clang-format off
// Flag groups look better right-justified, so don't format
enum thread_priorities {
  HIGHEST_PRIORITY     = 1,
  VERY_HIGH_PRIORITY   = 2,
  HIGH_PRIORITY        = 3,
  MID_PRIORITY         = 4,
  LOW_PRIORITY         = 5,
  VERY_LOW_PRIORITY    = 6,
  LOWEST_PRIORITY      = 7
};

enum stack_sizes {
  XXL_STACK     = 16384,
  XL_STACK      = 8192,
  L_STACK       = 6144,
  M_STACK       = 4096,
  S_STACK       = 2048,
  XS_STACK      = 1024,
  XXS_STACK     = 512
};

enum initialization_flags
{
  RTC_INIT_SUCCESS              = ((ULONG) 1 << 0),
  GNSS_INIT_SUCCESS             = ((ULONG) 1 << 1),
  CT_INIT_SUCCESS               = ((ULONG) 1 << 2),
  TEMPERATURE_INIT_SUCCESS      = ((ULONG) 1 << 3),
  TURBIDITY_INIT_SUCCESS        = ((ULONG) 1 << 4),
  LIGHT_INIT_SUCCESS            = ((ULONG) 1 << 5),
  ACCELEROMETER_INIT_SUCCESS    = ((ULONG) 1 << 6),
  WAVES_THREAD_INIT_SUCCESS     = ((ULONG) 1 << 7),
  IRIDIUM_INIT_SUCCESS          = ((ULONG) 1 << 8),
  SYSTEM_INIT_COMPLETE          = ((ULONG) 1 << 9)
};

enum rtc_server_complete_flags
{
  CONTROL_REQUEST_COMPLETE          = ((ULONG) 1 << 0),
  GNSS_REQUEST_COMPLETE             = ((ULONG) 1 << 1),
  CT_REQUEST_COMPLETE               = ((ULONG) 1 << 2),
  TEMPERATURE_REQUEST_COMPLETE      = ((ULONG) 1 << 3),
  LIGHT_REQUEST_COMPLETE            = ((ULONG) 1 << 4),
  TURBIDITY_REQUEST_COMPLETE        = ((ULONG) 1 << 5),
  ACCELEROMETER_REQUEST_COMPLETE    = ((ULONG) 1 << 6),
  WAVES_REQUEST_COMPLETE            = ((ULONG) 1 << 7),
  IRIDIUM_REQUEST_COMPLETE          = ((ULONG) 1 << 8),
};

enum thread_complete_flags
{
  GNSS_THREAD_COMPLETED_SUCCESSFULLY            = ((ULONG) 1 << 0),
  GNSS_THREAD_COMPLETED_WITH_ERRORS             = ((ULONG) 1 << 1),
  GNSS_SAMPLING_STARTED_FIX_GOOD                = ((ULONG) 1 << 2),
  GNSS_TWO_MINS_OUT_FROM_COMPLETION             = ((ULONG) 1 << 3),
  CT_THREAD_COMPLETED_SUCCESSFULLY              = ((ULONG) 1 << 4),
  CT_THREAD_COMPLETED_WITH_ERRORS               = ((ULONG) 1 << 5),
  TEMPERATURE_THREAD_COMPLETED_SUCCESSFULLY     = ((ULONG) 1 << 6),
  TEMPERATURE_THREAD_COMPLETED_WITH_ERRORS      = ((ULONG) 1 << 7),
  TURBIDITY_THREAD_COMPLETED_SUCCESSFULLY       = ((ULONG) 1 << 8),
  TURBIDITY_THREAD_COMPLETED_WITH_ERRORS        = ((ULONG) 1 << 9),
  LIGHT_THREAD_COMPLETED_SUCCESSFULLY           = ((ULONG) 1 << 10),
  LIGHT_THREAD_COMPLETED_WITH_ERRORS            = ((ULONG) 1 << 11),
  ACCELEROMETER_THREAD_COMPLETED_SUCCESSFULLY   = ((ULONG) 1 << 12),
  ACCELEROMETER_THREAD_COMPLETED_WITH_ERRORS    = ((ULONG) 1 << 13),
  WAVES_THREAD_COMPLETED_SUCCESSFULLY           = ((ULONG) 1 << 14),
  WAVES_THREAD_COMPLETED_WITH_ERRORS            = ((ULONG) 1 << 15),
  IRIDIUM_THREAD_COMPLETED_SUCCESSFULLY         = ((ULONG) 1 << 16),
  IRIDIUM_THREAD_COMPLETED_WITH_ERRORS          = ((ULONG) 1 << 17),
};

enum interrupt_flags
{
  // GNSS DMA flags (others use semaphores)
  GNSS_CONFIG_RECVD             = ((ULONG) 1 << 0),
  GNSS_TX_COMPLETE              = ((ULONG) 1 << 1),
  GNSS_MSG_RECEIVED             = ((ULONG) 1 << 2),
  GNSS_MSG_INCOMPLETE           = ((ULONG) 1 << 3),
  // Signal the ADC conversion has been completed
  BATTERY_CONVERSION_COMPLETE   = ((ULONG) 1 << 4)
};

typedef enum error_flags
{
  // Thread/ sensor errors
  GNSS_INIT_FAILED                      = ((ULONG) 1 << 0),
  GNSS_CONFIGURATION_FAILED             = ((ULONG) 1 << 1),
  GNSS_RESOLUTION_ERROR                 = ((ULONG) 1 << 2),
  GNSS_TOO_MANY_PARTIAL_MSGS            = ((ULONG) 1 << 3),
  GNSS_SAMPLE_WINDOW_TIMEOUT            = ((ULONG) 1 << 4),
  GNSS_FRAME_SYNC_FAILED                = ((ULONG) 1 << 5),
  CT_INIT_FAILED                        = ((ULONG) 1 << 6),
  CT_SAMPLING_ERROR                     = ((ULONG) 1 << 7),
  CT_SAMPLE_WINDOW_TIMEOUT              = ((ULONG) 1 << 8),
  TEMPERATURE_INIT_FAILED               = ((ULONG) 1 << 9),
  TEMPERATURE_SAMPLING_ERROR            = ((ULONG) 1 << 10),
  TEMPERATURE_SAMPLE_WINDOW_TIMEOUT     = ((ULONG) 1 << 11),
  LIGHT_INIT_FAILED                     = ((ULONG) 1 << 12),
  LIGHT_SAMPLING_ERROR                  = ((ULONG) 1 << 13),
  LIGHT_SAMPLE_WINDOW_TIMEOUT           = ((ULONG) 1 << 14),
  TURBIDITY_INIT_FAILED                 = ((ULONG) 1 << 15),
  TURBIDITY_SAMPLING_ERROR              = ((ULONG) 1 << 16),
  TURBIDITY_SAMPLE_WINDOW_TIMEOUT       = ((ULONG) 1 << 17),
  IRIDIUM_INIT_ERROR                    = ((ULONG) 1 << 18),
  IRIDIUM_UART_COMMS_ERROR              = ((ULONG) 1 << 19),
  FILE_SYSTEM_ERROR                     = ((ULONG) 1 << 20),
  RTC_ERROR                             = ((ULONG) 1 << 21),
  WAVES_INIT_FAILED                     = ((ULONG) 1 << 22),
  CORE_I2C_BUS_ERROR                    = ((ULONG) 1 << 23),
  NED_WAVES_RAN_OUT_OF_MEM              = ((ULONG) 1 << 24),
  FPU_EXCEPTION_OCCURRED                = ((ULONG) 1 << 25),
  // NB: we're really close to running out of space here.
  // I can shove a few accelerometer flags here, but the *next*
  // new payload will be an issue. Consider making it generic,
  // so the other new threads can also use it? !
  ACCELEROMETER_INIT_FAILED             = ((ULONG) 1 << 26),
  ACCELEROMETER_SAMPLING_ERROR          = ((ULONG) 1 << 27),
  // Misc errors
  WATCHDOG_RESET                        = ((ULONG) 1 << 29),
  SOFTWARE_RESET                        = ((ULONG) 1 << 30),
  MEMORY_CORRUPTION_ERROR               = ((ULONG) 1 << 31)
} error_flags_t;

// Event group flags for signaling completion (RTC thread, I2C thread, etc)
typedef enum
{
  CONTROL_THREAD_REQUEST_PROCESSED      = ((ULONG) 1 << 1),
  GNSS_REQUEST_PROCESSED                = ((ULONG) 1 << 2),
  CT_REQUEST_PROCESSED                  = ((ULONG) 1 << 3),
  TEMPERATURE_REQUEST_PROCESSED         = ((ULONG) 1 << 4),
  LIGHT_REQUEST_PROCESSED               = ((ULONG) 1 << 5),
  TURBIDITY_REQUEST_PROCESSED           = ((ULONG) 1 << 6),
  IRIDIUM_REQUEST_PROCESSED             = ((ULONG) 1 << 7),
  LOGGER_REQUEST_PROCESSED              = ((ULONG) 1 << 8),
  PERSISTENT_RAM_REQUEST_PROCESSED      = ((ULONG) 1 << 9),
  FILE_SYSTEM_REQUEST_PROCESSES         = ((ULONG) 1 << 10)
} process_complete_flags_t;
// clang-format on

typedef struct {
  // Core
#if SD_CARD_ENABLED
  SD_HandleTypeDef *sd_handle;
#endif
  SPI_HandleTypeDef *core_spi_handle;
  SPI_HandleTypeDef *expansion_spi_handle;
  I2C_HandleTypeDef *core_i2c_handle;
  UART_HandleTypeDef *iridium_uart_handle;
  UART_HandleTypeDef *gnss_uart_handle;
  UART_HandleTypeDef *ct_uart_handle;
  UART_HandleTypeDef *logger_uart_handle;
  UART_HandleTypeDef *expansion_uart_handle;
  OSPI_HandleTypeDef *ext_psram_handle;
  ADC_HandleTypeDef *battery_adc;
  // DMA handles
  DMA_HandleTypeDef *gnss_uart_tx_dma_handle;
  DMA_HandleTypeDef *gnss_uart_rx_dma_handle;
  DMA_HandleTypeDef *iridium_uart_tx_dma_handle;
  DMA_HandleTypeDef *iridium_uart_rx_dma_handle;
  DMA_HandleTypeDef *ct_uart_tx_dma_handle;
  DMA_HandleTypeDef *ct_uart_rx_dma_handle;
  DMA_HandleTypeDef *logger_uart_tx_dma_handle;
  DMA_HandleTypeDef *logger_uart_rx_dma_handle;
  DMA_HandleTypeDef *expansion_uart_tx_dma_handle;
  DMA_HandleTypeDef *expansion_uart_rx_dma_handle;
} Device_Handles;

typedef struct {
  TX_THREAD *control_thread;
  TX_THREAD *rtc_thread;
  TX_THREAD *led_thread;
  TX_THREAD *i2c_bus_thread;
  TX_THREAD *logger_thread;
  TX_THREAD *gnss_thread;
  TX_THREAD *ct_thread;
  TX_THREAD *temperature_thread;
  TX_THREAD *light_thread;
  TX_THREAD *turbidity_thread;
  TX_THREAD *waves_thread;
  TX_THREAD *iridium_thread;
  TX_THREAD *filex_thread;
  TX_THREAD *accel_thread;
} Thread_Handles;

extern microSWIFT_configuration configuration;

extern Thread_Handles thread_handles;
extern Device_Handles device_handles;

extern TX_SEMAPHORE ext_rtc_spi_sema;
extern TX_SEMAPHORE core_i2c_sema;
extern TX_SEMAPHORE iridium_uart_sema;
extern TX_SEMAPHORE ct_uart_sema;
extern TX_SEMAPHORE expansion_uart_sema;

extern TX_SEMAPHORE logger_sema;

extern TX_EVENT_FLAGS_GROUP initialization_flags;
extern TX_EVENT_FLAGS_GROUP irq_flags;
extern TX_EVENT_FLAGS_GROUP error_flags;

// Exposed so we can kill it in ISR if it takes too long to run
extern TX_THREAD waves_thread;
// clang-format off
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
// clang-format on
#define WAVES_MEM_POOL_SIZE 659456
#define EXPANSION_QUEUE_MSG_SIZE 64
#define EXPANSION_QUEUE_LENGTH 4
// clang-format off
/* USER CODE END EC */

/* Private defines -----------------------------------------------------------*/

/* USER CODE BEGIN PD */
// clang-format on

// clang-format off
/* USER CODE END PD */

/* Main thread defines -------------------------------------------------------*/
/* USER CODE BEGIN MTD */
// clang-format on

// clang-format off
/* USER CODE END MTD */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
// clang-format on

#define MS_PER_SECOND 1000

// The max times we'll try to get a single peripheral up before sending reset
// vector
#define MAX_SELF_TEST_RETRIES 10
// The maximum amount of time (in milliseconds) a sample window could take
#define MAX_ALLOWABLE_WINDOW_TIME_IN_MINUTES 61
// clang-format off
/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
UINT App_ThreadX_Init(VOID *memory_ptr);
void MX_ThreadX_Init(void);

/* USER CODE BEGIN EFP */
// clang-format on

// clang-format off
/* USER CODE END EFP */

/* USER CODE BEGIN 1 */
// clang-format on
// @formatter:on
// clang-format off
/* USER CODE END 1 */

#ifdef __cplusplus
}
#endif
#endif /* __APP_THREADX_H */
