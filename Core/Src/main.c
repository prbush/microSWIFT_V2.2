/* USER CODE BEGIN Header */
// clang-format on
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
#include "app_threadx.h"
#include "main.h"
#include "gpdma.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
// clang-format on
#include "octospi.h"
#include "stdbool.h"
#include "testing_hooks.h"

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

#define HARDWARE_VERSION (2U)
#define FIRMWARE_VERSION (9U)

static const microSWIFT_firmware_version_t firmware_version = {
    HARDWARE_VERSION, FIRMWARE_VERSION};

// Configuration bytes programmed using STM32 Cube programmer in the last page
// of flash
static microSWIFT_configuration flash_config
    __attribute__((section(".uservars.CONFIGURATION")));

// Needed to make sure we only wait on the crystal to stabilize on first boot
static bool rtc_crystal_stable_complete __attribute__((section(".sram2"))) =
    false;
// clang-format off
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config ( void );
static void SystemPower_Config ( void );
/* USER CODE BEGIN PFP */
// clang-format on
static void rtc_init_delay(void);
// clang-format off
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// clang-format on

// clang-format off
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main ( void )
{

  /* USER CODE BEGIN 1 */
  // clang-format on
  // clang-format off
  /* USER CODE END 1 */

      /* MCU
         Configuration--------------------------------------------------------*/

      /* Reset of all peripherals, Initializes the Flash interface and the
         Systick. */
      HAL_Init();

  /* USER CODE BEGIN Init */
  // clang-format on

  // Need to delay in order for the RTC oscillator to stabilize. Default is to
  // output 32768Hz clock, and this will be used to calibrate the MSIS when
  // switching over to that clock source as the primary in SystemClock_Config().
  if (!rtc_crystal_stable_complete) {
    rtc_init_delay();
    rtc_crystal_stable_complete = true;
  }

  // Setup the persistent ram if needed
  persistent_ram_init(&flash_config, &firmware_version);

  // clang-format off
  /* USER CODE END Init */

  /* Configure the System Power */
  SystemPower_Config();

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  // clang-format on
  // Disable any previous pullups or pull downs
  HAL_PWREx_DisablePullUpPullDownConfig();
  // clang-format off
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_GPDMA1_Init();
  /* USER CODE BEGIN 2 */
  // clang-format on

  // Disable the RTC Int B pin interrupt for now until we set up the RTC
  HAL_NVIC_DisableIRQ(EXTI2_IRQn);
  HAL_NVIC_ClearPendingIRQ(EXTI2_IRQn);

  // Change the gpio configuration based on which modem type we're using
  configure_iridium_onoff_pin(flash_config.iridium_v3f);

  // Toggle RTC timestamp 1 to capture boot time
  toggle_rtc_timestamp1_pin();

  // Shut down flash bank 2 -- no longer required
  HAL_FLASHEx_EnablePowerDown(FLASH_BANK_2);

  tests_init();

  if (tests.main_test != NULL) {
    tests.main_test(NULL);
  }

  // clang-format off
  /* USER CODE END 2 */

  MX_ThreadX_Init();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  // clang-format on
  while (1) {
    Error_Handler();
    // clang-format off
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    // clang-format on
  }
  // clang-format off
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE4) != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_OscInitStruct.OscillatorType =
      RCC_OSCILLATORTYPE_LSE | RCC_OSCILLATORTYPE_MSI | RCC_OSCILLATORTYPE_MSIK;
  RCC_OscInitStruct.LSEState = RCC_LSE_BYPASS;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_1;
  RCC_OscInitStruct.MSIKClockRange = RCC_MSIKRANGE_2;
  RCC_OscInitStruct.MSIKState = RCC_MSIK_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 |
                                RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) {
    Error_Handler();
  }

  /** Enable MSI Auto calibration
   */
  HAL_RCCEx_EnableMSIPLLModeSelection(RCC_MSIKPLL_MODE_SEL);
  HAL_RCCEx_EnableMSIPLLMode();
}

/**
 * @brief Power Configuration
 * @retval None
 */
static void SystemPower_Config(void) {
  HAL_PWREx_EnableVddIO2();

  /*
   * Switch to SMPS regulator instead of LDO
   */
  if (HAL_PWREx_ConfigSupply(PWR_SMPS_SUPPLY) != HAL_OK) {
    Error_Handler();
  }
  /* USER CODE BEGIN PWR */
  // clang-format on
  // clang-format off
  /* USER CODE END PWR */
}

/* USER CODE BEGIN 4 */
// clang-format on
static void rtc_init_delay(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  // Turn on Green LED to let user know the system has powered up
  __HAL_RCC_GPIOD_CLK_ENABLE();

  GPIO_InitStruct.Pin = LED_GREEN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);

  // Max time for oscillator stabilization is 2s, add a little bit for clock
  // inaccuracy
  HAL_Delay(2050);

  // Turn off and deinit LED GPIO (will be reinitialized in MX_GPIO_Init() )
  HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
  HAL_GPIO_DeInit(GPIOD, LED_GREEN_Pin);
  __HAL_RCC_GPIOD_CLK_DISABLE();
}
// clang-format off
/* USER CODE END 4 */

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed ( uint8_t *file, uint32_t line )
{
  /* USER CODE BEGIN 6 */
  // clang-format on
  /* User can add his own implementation to report the file name and line
   number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
   line) */
  // clang-format off
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
