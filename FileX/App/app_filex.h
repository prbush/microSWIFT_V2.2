
/* USER CODE BEGIN Header */
// clang-format on
/**
 ******************************************************************************
 * @file    app_filex.h
 * @author  MCD Application Team
 * @brief   FileX applicative header file
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __APP_FILEX_H__
#define __APP_FILEX_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "fx_api.h"
#include "fx_stm32_sd_driver.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
// clang-format on

// clang-format off
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
// clang-format on

// clang-format off
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
// clang-format on
extern TX_THREAD fx_thread;
// clang-format off
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
// clang-format on
// clang-format off
/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
UINT MX_FileX_Init(VOID *memory_ptr);

/* USER CODE BEGIN EFP */
// clang-format on

// clang-format off
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/

/* USER CODE BEGIN PD */
// clang-format on

// clang-format off
/* USER CODE END PD */

/* USER CODE BEGIN 1 */
// clang-format on

// clang-format off
/* USER CODE END 1 */
#ifdef __cplusplus
}
#endif
#endif /* __APP_FILEX_H__ */
