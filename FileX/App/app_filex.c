/* USER CODE BEGIN Header */
// clang-format on
/**
 ******************************************************************************
 * @file    app_filex.c
 * @author  MCD Application Team
 * @brief   FileX applicative file
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
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

/* Includes ------------------------------------------------------------------*/
#include "app_filex.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
// clang-format on
#include "app_threadx.h"
#include "file_system.h"
#include "file_system_server.h"
#include "logger.h"
#include "sdmmc.h"

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
/* Main file system thread  */
TX_THREAD fx_thread;
/* Buffer for FileX FX_MEDIA sector cache. */
ALIGN_32BYTES(uint32_t fx_sd_media_memory[FX_STM32_SD_DEFAULT_SECTOR_SIZE /
                                          sizeof(uint32_t)]);
/* Define FileX global media data structure.  */
FX_MEDIA sd_card;
// Queue for the file system server
TX_QUEUE file_system_messaging_queue;
// File system complete flags (for file system server)
TX_EVENT_FLAGS_GROUP file_system_complete_flags;
// timeout timer for file system
TX_TIMER file_system_timer;

// clang-format off
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/

/* USER CODE BEGIN PFP */
// clang-format on
/* Main thread entry function.  */
void fx_thread_entry(ULONG thread_input);
// clang-format off
/* USER CODE END PFP */

/**
  * @brief  Application FileX Initialization.
  * @param memory_ptr: memory pointer
  * @retval int
*/
UINT MX_FileX_Init(VOID *memory_ptr)
{
  UINT ret = FX_SUCCESS;
  TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL*)memory_ptr;

  /* USER CODE BEGIN MX_FileX_MEM_POOL */
  // clang-format on
  VOID *pointer;
  // clang-format off
  /* USER CODE END MX_FileX_MEM_POOL */

/* USER CODE BEGIN MX_FileX_Init */
  // clang-format on

  // Allocate stack for the file system thread
  ret = tx_byte_allocate(byte_pool, &pointer, XL_STACK, TX_NO_WAIT);
  if (ret != FX_SUCCESS) {
    return TX_POOL_ERROR;
  }

  // Create the file system thread. LOWEST_PRIORITY priority level and no
  // preemption possible
  ret = tx_thread_create(&fx_thread, "FileX Thread", fx_thread_entry, 0,
                         pointer, XL_STACK, LOW_PRIORITY, HIGHEST_PRIORITY,
                         TX_NO_TIME_SLICE, TX_AUTO_START);
  if (ret != FX_SUCCESS) {
    return TX_THREAD_ERROR;
  }

  // Message queue for file system server
  ret = tx_byte_allocate(byte_pool, (VOID **)&pointer,
                         sizeof(file_system_request_message) *
                             FILE_SYSTEM_QUEUE_LENGTH,
                         TX_NO_WAIT);
  if (ret != TX_SUCCESS) {
    return ret;
  }

  ret = tx_queue_create(
      &file_system_messaging_queue, "file system queue",
      sizeof(file_system_request_message) / sizeof(uint32_t), pointer,
      sizeof(file_system_request_message) * FILE_SYSTEM_QUEUE_LENGTH);
  if (ret != TX_SUCCESS) {
    return ret;
  }

  // Create the file system complete flags we'll use for tracking file system
  // function completion
  ret = tx_event_flags_create(&file_system_complete_flags,
                              "File system complete flags");
  if (ret != TX_SUCCESS) {
    return ret;
  }

  // Create the file system timeout timer
  ret = tx_timer_create(&file_system_timer, "File system timeout timer",
                        file_system_timer_expired, 0, 1, 0, TX_NO_ACTIVATE);
  if (ret != TX_SUCCESS) {
    return ret;
  }
  // clang-format off
/* USER CODE END MX_FileX_Init */

/* Initialize FileX.  */
  fx_system_initialize();

/* USER CODE BEGIN MX_FileX_Init 1*/
  // clang-format on
  file_system_server_init(&file_system_messaging_queue,
                          &file_system_complete_flags, &configuration);
  // clang-format off
/* USER CODE END MX_FileX_Init 1*/

  return ret;
}

/* USER CODE BEGIN 1 */
// clang-format on
void fx_thread_entry(ULONG thread_input) {
  TX_THREAD *this_thread = tx_thread_identify();
  File_System_SD_Card file_system = {0};
  file_system_request_message msg = {0};
  UINT tx_ret, num_errors = 0;
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;

  file_system_init(&file_system, &(fx_sd_media_memory[0]), &sd_card,
                   &configuration);

  if (file_system.initialize_card() != uSWIFT_SUCCESS) {
    filex_error_out(this_thread,
                    "SD card failed to initialize. File system unavailable.");
  }

  LOG("SD card peripheral initialization successful.");

  while (1) {
    tx_ret =
        tx_queue_receive(&file_system_messaging_queue, &msg, TX_WAIT_FOREVER);
    if (tx_ret == TX_SUCCESS) {
      switch (msg.request) {
      case SAVE_LOG_LINE:
        ret = file_system.save_log_line(msg.object_pointer, msg.size);
        uart_logger_return_line_buf((log_line_buf *)msg.object_pointer);
        break;

      case SAVE_GNSS_RAW:
        ret = file_system.save_gnss_velocities(msg.object_pointer);
        break;

      case SAVE_GNSS_BREADCRUMB_TRACK:
        ret = file_system.save_gnss_breadcrumb_track(msg.object_pointer);
        break;

      case SAVE_TEMPERATURE_RAW:
        ret = file_system.save_temperature_raw(msg.object_pointer);
        break;

      case SAVE_CT_RAW:
        ret = file_system.save_ct_raw(msg.object_pointer);
        break;

      case SAVE_LIGHT_RAW:
        ret = file_system.save_light_raw(msg.object_pointer);
        break;

      case SAVE_TURBIDITY_RAW:
        ret = file_system.save_turbidity_raw(msg.object_pointer);
        break;

      case UPDATE_DATE_TIME:
        ret = file_system.set_date_time();
        break;

      default:
        ret = uSWIFT_PARAMETERS_INVALID;
        break;
      }

      if (ret != uSWIFT_SUCCESS) {
        if (++num_errors >= 5) {
          filex_error_out(
              this_thread,
              "File system failed to service request %d, returning code %d.",
              (int)msg.request, (int)ret);
        } else {
          LOG("File system failed to service request %d, returning code %d.",
              (int)msg.request, (int)ret);
        }
      }

      if (msg.return_code != NULL) {
        *msg.return_code = ret;
      }
      (void)tx_event_flags_set(&file_system_complete_flags, msg.complete_flag,
                               TX_OR);
    }
  }
}
// clang-format off
/* USER CODE END 1 */
