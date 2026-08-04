/*
 * controller.c
 *
 *  Created on: Sep 12, 2024
 *      Author: philbush
 */

#include "controller.h"
#include "NEDWaves/rtwhalf.h"
#include "ct_sensor.h"
#include "file_system.h"
#include "leds.h"
#include "logger.h"
#include "persistent_ram.h"
#include "sdmmc.h"
#include "threadx_support.h"
#include "watchdog.h"
#include <ext_rtc_server.h>

// @formatter:off
static Control *controller_self;

// Struct functions
static bool _control_startup_procedure(void);
static void _control_shutdown_procedure(void);
static real16_T _control_get_battery_voltage(void);
static void _control_shutdown_all_peripherals(void);
static void _control_shutdown_all_interfaces(void);
static void _control_enter_processor_standby_mode(void);
static void _control_manage_state(void);
static void _control_monitor_and_handle_errors(void);

// Helper functions
static void __get_alarm_settings(rtc_alarm_struct *alarm);
static void __handle_rtc_error(void);
static void __handle_gnss_error(ULONG error_flag);
static void __handle_ct_error(ULONG error_flag);
static void __handle_temperature_error(ULONG error_flag);
static void __handle_turbidity_error(ULONG error_flag);
static void __handle_light_error(ULONG error_flag);
static void __handle_waves_error(void);
static void __handle_iridium_error(ULONG error_flags);
static void __handle_accel_error(ULONG error_flags);
static void __handle_file_system_error(void);
static void __handle_i2c_error(void);
static void __handle_misc_error(ULONG error_flag);

// Static helper functions

// @formatter:on

void controller_init(Control *struct_ptr,
                     microSWIFT_configuration *global_config,
                     Thread_Handles *thread_handles,
                     TX_EVENT_FLAGS_GROUP *error_flags,
                     TX_EVENT_FLAGS_GROUP *init_flags,
                     TX_EVENT_FLAGS_GROUP *irq_flags,
                     TX_EVENT_FLAGS_GROUP *complete_flags, TX_TIMER *timer,
                     ADC_HandleTypeDef *battery_adc_handle,
                     sbd_message_type_52 *current_message) {
  // Init self
  controller_self = struct_ptr;

  controller_self->global_config = global_config;
  controller_self->thread_handles = thread_handles;
  controller_self->error_flags = error_flags;
  controller_self->init_flags = init_flags;
  controller_self->irq_flags = irq_flags;
  controller_self->complete_flags = complete_flags;
  controller_self->timer = timer;
  controller_self->current_message = current_message;
  controller_self->timer_timeout = false;

  controller_self->thread_status.gnss_complete = false;
  controller_self->thread_status.waves_complete = false;
  controller_self->thread_status.iridium_complete = false;
  controller_self->thread_status.ct_complete =
      !controller_self->global_config->ct_enabled;
  controller_self->thread_status.temperature_complete =
      !controller_self->global_config->temperature_enabled;
  controller_self->thread_status.light_complete =
      !controller_self->global_config->light_enabled;
  controller_self->thread_status.turbidity_complete =
      !controller_self->global_config->turbidity_enabled;
  controller_self->thread_status.accelerometer_complete =
      !controller_self->global_config->accelerometer_enabled;

  controller_self->startup_procedure = _control_startup_procedure;
  controller_self->shutdown_procedure = _control_shutdown_procedure;
  controller_self->get_battery_voltage = _control_get_battery_voltage;
  controller_self->shutdown_all_peripherals = _control_shutdown_all_peripherals;
  controller_self->shutdown_all_interfaces = _control_shutdown_all_interfaces;
  controller_self->enter_processor_standby_mode =
      _control_enter_processor_standby_mode;
  controller_self->manage_state = _control_manage_state;
  controller_self->monitor_and_handle_errors =
      _control_monitor_and_handle_errors;

  // Init battery
  battery_init(&controller_self->battery, battery_adc_handle,
               controller_self->irq_flags);
  // Init RF switch
  rf_switch_init(&controller_self->rf_switch);
}

void control_timer_expired(ULONG expiration_input) {
  (void)expiration_input;

  controller_self->timer_timeout = true;
}

bool control_get_timeout_status(void) { return controller_self->timer_timeout; }

ULONG control_get_accumulated_error_flags(void) {
  return controller_self->accumulated_error_flags;
}

static bool _control_startup_procedure(void) {
  UINT tx_return = TX_SUCCESS;
  ULONG init_success_flags = (RTC_INIT_SUCCESS | GNSS_INIT_SUCCESS |
                              WAVES_THREAD_INIT_SUCCESS | IRIDIUM_INIT_SUCCESS);
  ULONG current_flags = 0;
  uint32_t reset_reason = 0, watchdog_counter = 0;
  real16_T voltage = {0};
  bool initial_powerup = is_first_sample_window();
  ULONG init_wait_ticks = STARTUP_SEQUENCE_MAX_WAIT_TICKS;

  // Invalid acquisition time case
  if (controller_self->global_config->gnss_max_acquisition_wait_time == 0) {
    LOG("Invalid calculated GNSS max acquisition time. Check settings.");

    led_light_sequence(TEST_FAILED_LED_SEQUENCE, LED_SEQUENCE_FOREVER);
    tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND * 30);
    Error_Handler();
  }

  // Start the duty cycle timer
  tx_return = tx_timer_change(controller_self->timer,
                              (controller_self->global_config->duty_cycle * 60 *
                               TX_TIMER_TICKS_PER_SECOND) -
                                  TX_TIMER_TICKS_PER_SECOND,
                              0);
  if (tx_return != TX_SUCCESS) {
    LOG("Unable to set controller duty cycle timer.");

    led_light_sequence(TEST_FAILED_LED_SEQUENCE, LED_SEQUENCE_FOREVER);
    tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND * 30);
    Error_Handler();
  }

  tx_return = tx_timer_activate(controller_self->timer);
  if (tx_return != TX_SUCCESS) {
    LOG("Unable to set controller duty cycle timer.");

    led_light_sequence(TEST_FAILED_LED_SEQUENCE, LED_SEQUENCE_FOREVER);
    tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND * 30);
    Error_Handler();
  }

  if (!initial_powerup) {
    // Set the watchdog reset or software reset flags
    reset_reason = persistent_ram_get_reset_reason();

    if (reset_reason & RCC_RESET_FLAG_PIN) {
      tx_event_flags_set(&error_flags, WATCHDOG_RESET, TX_OR);
    }

    if (reset_reason & RCC_RESET_FLAG_SW) {
      tx_event_flags_set(&error_flags, SOFTWARE_RESET, TX_OR);
    }
  }

  if (persistent_ram_get_ota_update_status()) {
    LOG("Running on configuration settings updated over-the-air, dated: %s %s",
        controller_self->global_config->compile_date,
        controller_self->global_config->compile_time);
  }

  tx_thread_sleep(2);

  // Grab the battery voltage before we start turning everything on
  voltage = controller_self->get_battery_voltage();
  memcpy(&controller_self->current_message->mean_voltage, &voltage,
         sizeof(real16_T));

  battery_deinit();

  // Set the RF switch to GNSS port
  controller_self->rf_switch.set_gnss_port();

  // Start core threads
  (void)tx_thread_resume(controller_self->thread_handles->waves_thread);
  (void)tx_thread_resume(controller_self->thread_handles->gnss_thread);
  (void)tx_thread_resume(controller_self->thread_handles->iridium_thread);

  // Start optional component threads if enabled
  if (controller_self->global_config->ct_enabled) {
    init_success_flags |= CT_INIT_SUCCESS;
    (void)tx_thread_resume(controller_self->thread_handles->ct_thread);
  }

  if (controller_self->global_config->temperature_enabled) {
    init_success_flags |= TEMPERATURE_INIT_SUCCESS;
    (void)tx_thread_resume(controller_self->thread_handles->temperature_thread);
  }

  if (controller_self->global_config->light_enabled) {
    init_success_flags |= LIGHT_INIT_SUCCESS;
    (void)tx_thread_resume(controller_self->thread_handles->light_thread);
  }

  if (controller_self->global_config->turbidity_enabled) {
    init_success_flags |= TURBIDITY_INIT_SUCCESS;
    (void)tx_thread_resume(controller_self->thread_handles->turbidity_thread);
  }

  if (controller_self->global_config->accelerometer_enabled) {
    init_success_flags |= ACCELEROMETER_INIT_SUCCESS;
    LOG("Starting accelerometer thread");
    (void)tx_thread_resume(controller_self->thread_handles->accel_thread);
  }

  // Flash power up sequence (this will also give threads time to execute their
  // init procedures)
  if (initial_powerup) {
    led_light_sequence(INITIAL_LED_SEQUENCE, 10);
  }

  while (init_wait_ticks > 0) {
    if (++watchdog_counter % (TX_TIMER_TICKS_PER_SECOND / 10) == 0) {
      watchdog_check_in(CONTROL_THREAD);
    }

    tx_return =
        tx_event_flags_get(controller_self->init_flags, init_success_flags,
                           TX_AND_CLEAR, &current_flags, TX_NO_WAIT);

    controller_self->monitor_and_handle_errors();

    if (tx_return == TX_SUCCESS) {
      break;
    }

    tx_thread_sleep(10);
    init_wait_ticks -= 10;
  }

  if (tx_return != TX_SUCCESS) {
    // TODO: There may be a larger category of sensors that we give the *chance*
    //   to initialize but then if they don't come online, we run waves anyways.
    if ((current_flags | ACCELEROMETER_INIT_SUCCESS) == init_success_flags) {
      LOG("Accelerometer initialization failed; disabling accelerometer and "
          "continuing with startup");
      tx_return = TX_SUCCESS;
    } else {
      LOG("control startup_procedure failed; success_flags = %lu, "
          "current_flags "
          "= %lu",
          init_success_flags, current_flags);
    }
  }

  return (tx_return == TX_SUCCESS);
}

static void _control_shutdown_procedure(void) {
  rtc_alarm_struct alarm_settings;

  led_light_sequence(LIGHTS_OFF, LED_SEQUENCE_FOREVER);

  // Make extra sure the alarm flag is cleared
  if (rtc_server_clear_flag(ALARM_FLAG, CONTROL_REQUEST_COMPLETE) !=
      uSWIFT_SUCCESS) {
    Error_Handler();
  }

  tx_thread_sleep(1);

  // And the alarm interrupt line on the RTC is high (off)
  if (HAL_GPIO_ReadPin(RTC_INT_B_GPIO_Port, RTC_INT_B_Pin) != GPIO_PIN_SET) {
    Error_Handler();
  }

  // Set the alarm
  __get_alarm_settings(&alarm_settings);
  if (rtc_server_set_alarm(alarm_settings, CONTROL_REQUEST_COMPLETE) !=
      uSWIFT_SUCCESS) {
    Error_Handler();
  }

  LOG("All threads complete. Entering processor standby mode. Alarm set for "
      "%02d:%02d:%02d UTC.",
      (int)alarm_settings.alarm_hour, (int)alarm_settings.alarm_minute,
      (int)alarm_settings.alarm_second);

  // Give the logger and file system time to complete
  tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);

  file_system_deinit();

  // Make sure everything is shut down
  controller_self->shutdown_all_peripherals();

  // Deinit all enabled peripherals
  controller_self->shutdown_all_interfaces();

  // Enter standby mode -- processor will be woken by RTC alarm
  controller_self->enter_processor_standby_mode();
}

static real16_T _control_get_battery_voltage(void) {
  real16_T battery_voltage = {BATTERY_ERROR_VOLTAGE_VALUE};
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;

  ret = controller_self->battery.get_voltage(&battery_voltage);
  if (ret != uSWIFT_SUCCESS) {
    switch (ret) {
    case uSWIFT_INITIALIZATION_ERROR:
      LOG("Battery ADC initialization error, unable to obtain battery voltage");
      break;

    case uSWIFT_CALIBRATION_ERROR:
      LOG("Battery ADC calibration error, unable to obtain battery voltage");
      break;

    case uSWIFT_TIMEOUT:
      LOG("Battery ADC timed out, unable to obtain battery voltage");
      break;

    default:
      LOG("Unknown battery error, unable to obtain battery voltage");
      break;
    }
  } else {
    LOG("Battery Voltage = %2.2f Volts", halfToFloat(battery_voltage));
  }

  return battery_voltage;
}

static void _control_shutdown_all_peripherals(void) {
  HAL_GPIO_WritePin(CT_FET_GPIO_Port, CT_FET_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(RF_SWITCH_VCTL_GPIO_Port, RF_SWITCH_VCTL_Pin,
                    GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GNSS_FET_GPIO_Port, GNSS_FET_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(BUS_5V_FET_GPIO_Port, BUS_5V_FET_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(SD_CARD_FET_GPIO_Port, SD_CARD_FET_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(RS232_FORCEOFF_GPIO_Port, RS232_FORCEOFF_Pin,
                    GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(TEMPERATURE_FET_GPIO_Port, TEMPERATURE_FET_Pin,
                    GPIO_PIN_RESET);
  HAL_GPIO_WritePin(TURBIDITY_FET_GPIO_Port, TURBIDITY_FET_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LIGHT_FET_GPIO_Port, LIGHT_FET_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(RTC_WDOG_OR_INPUT_GPIO_Port, RTC_WDOG_OR_INPUT_Pin,
                    GPIO_PIN_RESET);
  HAL_GPIO_WritePin(EXP_GPIO_1_GPIO_Port, EXP_GPIO_1_Pin, GPIO_PIN_RESET);
}

static void _control_shutdown_all_interfaces(void) {
  (void)i2c2_deinit();
  (void)octospi1_deinit();
  (void)sdmmc1_deinit();
  (void)spi1_deinit();
  (void)spi2_deinit();
  (void)lpuart1_deinit();
  (void)usart1_deinit();
  (void)usart2_deinit();
  (void)usart3_deinit();
  (void)uart4_deinit();
  (void)battery_deinit();
}

static void _control_enter_processor_standby_mode(void) {
  __IO uint32_t dummy, i;

  // Disable all non-relevant interrupts
  for (i = WWDG_IRQn; i <= HSPI1_IRQn; i++) {
    HAL_NVIC_DisableIRQ(i);
    HAL_NVIC_ClearPendingIRQ(i);
  }

  // Make sure the IRQ is enabled for the wakeup pin
  HAL_NVIC_SetPriority(PVD_PVM_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(PVD_PVM_IRQn);

  // Retain all SRAM2 contents
  HAL_PWREx_EnableSRAM2ContentStandbyRetention(PWR_SRAM2_FULL_STANDBY);

  // Enable power clock
  __HAL_RCC_PWR_CLK_ENABLE();

  // Ensure RTC is disabled
  CLEAR_BIT(RCC->BDCR, RCC_BDCR_RTCEN);

  // Clear the backup ram retention bit
  CLEAR_BIT(PWR->BDCR1, PWR_BDCR1_BREN);

  HAL_PWREx_EnablePullUpPullDownConfig();
  HAL_PWREx_EnableGPIOPullUp(PWR_GPIO_D, BUS_5V_FET_Pin);
  HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_D, RS232_FORCEOFF_Pin);
  HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_G, IRIDIUM_OnOff_Pin);

  // Enable wakeup on the RTC alarm pin: PWR_WAKEUP_PIN1_LOW_1 = PB2 low
  // polarity = RTC Int B
  HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1_LOW_1);

  // Clear the stop mode and standby mode flags
  __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SBF);
  __HAL_PWR_CLEAR_FLAG(PWR_WAKEUP_FLAG1);

  // #warning "!!! Remove this, and disable debug in low power modes in build
  // settings prior to deployment !!!" #ifndef DEBUG
  //   DBGMCU->CR = 0; // Disable debug, trace and IWDG in low-power modes
  // #else
  //   HAL_EnableDBGStandbyMode ();
  // #endif

  DBGMCU->CR = 0;

  /* Select Standby mode */
  MODIFY_REG(PWR->CR1, PWR_CR1_LPMS, PWR_CR1_LPMS_2);

  /* Set SLEEPDEEP bit of Cortex System Control Register */
  SET_BIT(SCB->SCR, ((uint32_t)SCB_SCR_SLEEPDEEP_Msk));

  // Make sure register operations are complete
  dummy = PWR->SR;
  dummy = PWR->CR1;
  dummy = SCB->SCR;

  i = dummy;

  // Enter low-power mode
  while (1) {
    __DSB();
    __WFI();
  }
}

static void _control_manage_state(void) {
  ULONG current_flags;
  UINT ret = TX_SUCCESS;

  bool iridium_ready = false;

  // Check first if the duty cycle timer has expired
  if (controller_self->timer_timeout) {
    LOG("Controller timer expired; shutting buoy down.");
    controller_self->shutdown_procedure();
  }

  (void)tx_event_flags_get(controller_self->complete_flags, ALL_EVENT_FLAGS,
                           TX_OR_CLEAR, &current_flags, TX_NO_WAIT);

  // If we're running in continuous mode AND GNSS errored out,
  // we may need to wake up the accelerometer thread once more in order
  // to query the accel board for spectra.
  if (controller_self->thread_status.gnss_complete &&
      !controller_self->thread_status.accelerometer_complete) {

    UINT thread_state;
    UINT status =
        tx_thread_info_get(controller_self->thread_handles->accel_thread, NULL,
                           &thread_state, NULL, NULL, NULL, NULL, NULL, NULL);

    if (status == TX_SUCCESS) {
      // Thread information successfully retrieved
      if (TX_SUSPENDED == thread_state) {
        LOG("...accel thread_state was TX_SUSPENDED; resuming");
        tx_thread_resume(controller_self->thread_handles->accel_thread);
      }
    }
  }

  // Exit early case
  if (current_flags == 0) {
    return;
  }

  // Handle completed statuses first to avoid trying to resume a thread that has
  // been terminated.
  if ((current_flags & CT_THREAD_COMPLETED_SUCCESSFULLY) ||
      (current_flags & CT_THREAD_COMPLETED_WITH_ERRORS)) {
    controller_self->thread_status.ct_complete = true;

    LOG("CT thread complete.");
    tx_thread_sleep(25);
  }

  if ((current_flags & TEMPERATURE_THREAD_COMPLETED_SUCCESSFULLY) ||
      (current_flags & TEMPERATURE_THREAD_COMPLETED_WITH_ERRORS)) {
    controller_self->thread_status.temperature_complete = true;

    LOG("Temperature thread complete.");
    tx_thread_sleep(25);
  }

  if ((current_flags & TURBIDITY_THREAD_COMPLETED_SUCCESSFULLY) ||
      (current_flags & TURBIDITY_THREAD_COMPLETED_WITH_ERRORS)) {
    controller_self->thread_status.turbidity_complete = true;

    LOG("Turbidity thread complete.");
    tx_thread_sleep(25);
  }

  if ((current_flags & LIGHT_THREAD_COMPLETED_SUCCESSFULLY) ||
      (current_flags & LIGHT_THREAD_COMPLETED_WITH_ERRORS)) {
    controller_self->thread_status.light_complete = true;

    LOG("Light thread complete.");
    tx_thread_sleep(25);
  }

  if ((current_flags & ACCELEROMETER_THREAD_COMPLETED_SUCCESSFULLY) ||
      (current_flags & ACCELEROMETER_THREAD_COMPLETED_WITH_ERRORS)) {
    controller_self->thread_status.accelerometer_complete = true;

    LOG("Accelerometer thread complete.");
    tx_thread_sleep(25);
  }

  if ((current_flags & WAVES_THREAD_COMPLETED_SUCCESSFULLY) |
      (current_flags & WAVES_THREAD_COMPLETED_WITH_ERRORS)) {
    controller_self->thread_status.waves_complete = true;

    LOG("NED Waves thread complete.");

    tx_thread_sleep(25);
  }

  // When the GNSS has resolved time, has a fix, and has started sampling,
  // signal the light, turbidity, and/or accelerometer threads to start.
  if (current_flags & GNSS_SAMPLING_STARTED_FIX_GOOD) {
    if (!controller_self->thread_status.light_complete) {
      ret |= tx_thread_resume(controller_self->thread_handles->light_thread);
    }

    if (!controller_self->thread_status.turbidity_complete) {
      ret |=
          tx_thread_resume(controller_self->thread_handles->turbidity_thread);
    }

    if (!controller_self->thread_status.accelerometer_complete) {
      LOG("Acquired GNSS fix; resuming accelerometer thread");
      ret |= tx_thread_resume(controller_self->thread_handles->accel_thread);
    }
  }

  // When the GNSS sample window is close to being done, kick off CT or
  // Temperature sensor sampling Either of these (or both) will complete in less
  // than 2 mins
  if (current_flags & GNSS_TWO_MINS_OUT_FROM_COMPLETION) {
    if (!controller_self->thread_status.ct_complete) {
      ret |= tx_thread_resume(controller_self->thread_handles->ct_thread);
    }

    if (!controller_self->thread_status.temperature_complete) {
      ret |=
          tx_thread_resume(controller_self->thread_handles->temperature_thread);
    }
    // If we're running the accelerometer in continuous mode, need to
    // wake that thread back up so it can query the accel board for the
    // highest priority messages.
    if (controller_self->global_config->accelerometer_continuous_sampling &&
        !controller_self->thread_status.accelerometer_complete) {
      LOG("GNSS two minutes from completion: resuming accel_thread");
      ret |= tx_thread_resume(controller_self->thread_handles->accel_thread);
    }
  }

  // When the GNSS thread is complete, we can run the Waves algo
  if (current_flags & GNSS_THREAD_COMPLETED_SUCCESSFULLY) {
    controller_self->thread_status.gnss_complete = true;

    ret |= tx_thread_resume(controller_self->thread_handles->waves_thread);

    LOG("GNSS thread complete.");
    tx_thread_sleep(25);
  }

  // If GNSS thread errors out, do not run waves
  if (current_flags & GNSS_THREAD_COMPLETED_WITH_ERRORS) {
    controller_self->thread_status.gnss_complete = true;

    if (!controller_self->thread_status.ct_complete) {
      ret |= tx_thread_resume(controller_self->thread_handles->ct_thread);
    }

    if (!controller_self->thread_status.temperature_complete) {
      ret |=
          tx_thread_resume(controller_self->thread_handles->temperature_thread);
    }

    // NOTE: I needed this for testing indoors, so that the accelerometer
    // would run even when I couldn't get a GPS fix. If we want to use it
    // operationally, should we also start the light thread so we get
    // that data as well? (I guess normally you wouldn't because it's not worth
    // using 18 mins of battery just for light data?)
    //
    // NB: This works for polled mode, but the continuous mode needs an
    //     additional resume to request data after sampling.
    if (!controller_self->thread_status.accelerometer_complete) {
      LOG("No GNSS fix; resuming accelerometer thread anyways");
      ret |= tx_thread_resume(controller_self->thread_handles->accel_thread);
    }

    LOG("GNSS thread complete.");
    tx_thread_sleep(25);
  }

  iridium_ready = controller_self->thread_status.gnss_complete &&
                  controller_self->thread_status.ct_complete &&
                  controller_self->thread_status.temperature_complete &&
                  controller_self->thread_status.light_complete &&
                  controller_self->thread_status.turbidity_complete &&
                  controller_self->thread_status.accelerometer_complete &&
                  controller_self->thread_status.waves_complete;

  if (iridium_ready) {
    controller_self->rf_switch.set_iridium_port();
    ret |= tx_thread_resume(controller_self->thread_handles->iridium_thread);
  }

  controller_self->thread_status.iridium_complete =
      (current_flags & IRIDIUM_THREAD_COMPLETED_SUCCESSFULLY) |
      (current_flags & IRIDIUM_THREAD_COMPLETED_WITH_ERRORS);

  if (controller_self->thread_status.iridium_complete) {
    LOG("Iridium thread complete, now terminating.");
    tx_thread_sleep(25);
    controller_self->shutdown_procedure();
  }

  if (ret != TX_SUCCESS) {
    controller_self->shutdown_all_peripherals();
    // NOTE(LEL): This makes me nervous -- go into an infinite loop in
    // case of failure?
    tx_thread_sleep(25);
    Error_Handler();
  }
}

static void _control_monitor_and_handle_errors(void) {
  ULONG current_flags;
  ULONG gnss_errors, ct_errors, temperature_errors, light_errors,
      turbidity_errors, waves_errors, iridium_errors, file_system_errors,
      i2c_errors, rtc_errors, misc_errors, accel_errors;

  // Get the error flags
  (void)tx_event_flags_get(controller_self->error_flags, ALL_EVENT_FLAGS,
                           TX_OR_CLEAR, &current_flags, TX_NO_WAIT);

  // Exit early case
  if (current_flags == 0) {
    return;
  }

  controller_self->accumulated_error_flags |= current_flags;

  gnss_errors =
      current_flags & (GNSS_INIT_FAILED | GNSS_CONFIGURATION_FAILED |
                       GNSS_RESOLUTION_ERROR | GNSS_TOO_MANY_PARTIAL_MSGS |
                       GNSS_SAMPLE_WINDOW_TIMEOUT | GNSS_FRAME_SYNC_FAILED);
  ct_errors = current_flags &
              (CT_INIT_FAILED | CT_SAMPLING_ERROR | CT_SAMPLE_WINDOW_TIMEOUT);
  temperature_errors =
      current_flags & (TEMPERATURE_INIT_FAILED | TEMPERATURE_SAMPLING_ERROR |
                       TEMPERATURE_SAMPLE_WINDOW_TIMEOUT);
  light_errors = current_flags & (LIGHT_INIT_FAILED | LIGHT_SAMPLING_ERROR |
                                  LIGHT_SAMPLE_WINDOW_TIMEOUT);
  turbidity_errors =
      current_flags & (TURBIDITY_INIT_FAILED | TURBIDITY_SAMPLING_ERROR |
                       TURBIDITY_SAMPLE_WINDOW_TIMEOUT);
  accel_errors = current_flags &
                 (ACCELEROMETER_INIT_FAILED | ACCELEROMETER_SAMPLING_ERROR);
  iridium_errors =
      current_flags & (IRIDIUM_INIT_ERROR | IRIDIUM_UART_COMMS_ERROR);
  waves_errors = current_flags & (WAVES_INIT_FAILED | NED_WAVES_RAN_OUT_OF_MEM |
                                  FPU_EXCEPTION_OCCURRED);
  file_system_errors = current_flags & (FILE_SYSTEM_ERROR);
  i2c_errors = current_flags & (CORE_I2C_BUS_ERROR);
  rtc_errors = current_flags & (RTC_ERROR);
  misc_errors = current_flags &
                (WATCHDOG_RESET | SOFTWARE_RESET | MEMORY_CORRUPTION_ERROR);

  if (gnss_errors) {
    __handle_gnss_error(gnss_errors);
  }

  if (iridium_errors) {
    __handle_iridium_error(iridium_errors);
  }

  if (rtc_errors) {
    __handle_rtc_error();
  }

  if (ct_errors) {
    __handle_ct_error(ct_errors);
  }

  if (temperature_errors) {
    __handle_temperature_error(temperature_errors);
  }

  if (light_errors) {
    __handle_light_error(light_errors);
  }

  if (turbidity_errors) {
    __handle_turbidity_error(turbidity_errors);
  }

  if (accel_errors) {
    __handle_accel_error(accel_errors);
  }

  if (waves_errors) {
    __handle_waves_error();
  }

  if (file_system_errors) {
    __handle_file_system_error();
  }

  if (i2c_errors) {
    __handle_i2c_error();
  }

  if (misc_errors) {
    __handle_misc_error(misc_errors);
  }
}

static void __get_alarm_settings(rtc_alarm_struct *alarm) {
  struct tm boot_time = {0};
  struct tm time_now = {0};
  time_t boot_timestamp = 0;
  time_t now_timestamp = 0;

  alarm->day_alarm_en = false;
  alarm->hour_alarm_en = true;
  alarm->minute_alarm_en = true;
  alarm->second_alarm_en = true;
  alarm->weekday_alarm_en = false;
  // Alarm second is always 0
  alarm->alarm_second = 0;

  // Start by getting current time. This will be used to set the alarm in the
  // first case and used for sanity check in the second case
  if (rtc_server_get_time(&time_now, CONTROL_THREAD_REQUEST_PROCESSED) !=
      uSWIFT_SUCCESS) {
    Error_Handler();
  }

  // If the clock has not been set (GNSS failed to get a fix) or this is the
  // first sample window, the next window will start at the top of the hour,
  // either in real time or whatever the RTC is set to
  if (is_first_sample_window() || (!persistent_ram_get_rtc_time_set())) {
    alarm->alarm_minute = 0;

    // Edge case where the current time is xx:59:59, etc
    if (time_now.tm_min == 59) {
      alarm->alarm_hour = (time_now.tm_hour + 2) % 24;
    } else {
      alarm->alarm_hour = (time_now.tm_hour + 1) % 24;
    }
  }
  // Otherwise set the alarm to be boot time + duty cycle
  else {
    // Get the timestamp from RTC timestamp 1, which was set at boot
    if (rtc_server_get_timestamp(BOOT_TIME_TIMESTAMP, &boot_time,
                                 CONTROL_THREAD_REQUEST_PROCESSED) !=
        uSWIFT_SUCCESS) {
      Error_Handler();
    }

    // Turn the struct tm into a time_t timestamp for easy arithmetic
    boot_timestamp = mktime(&boot_time);
    // Boot timestamp now reflects the wakeup time
    boot_timestamp += controller_self->global_config->duty_cycle * 60U;

    // Sanity check, make sure the alarm is not being set in the past.
    // Need a few seconds buffer (5U) for the rest of the code to run before
    // standby mode.
    now_timestamp = mktime(&time_now);
    if (boot_timestamp < (now_timestamp + 5U)) {
      // Set wakeup to be time now + duty cycle.
      boot_timestamp =
          now_timestamp + (controller_self->global_config->duty_cycle * 60U);
    }

    // Recycle boot_time struct to turn the wakeup time_t (boot_timestamp) into
    // a struct tm
    boot_time = *gmtime(&boot_timestamp);

    alarm->alarm_minute = boot_time.tm_min;
    alarm->alarm_hour = boot_time.tm_hour;
  }
}

static void __handle_rtc_error(void) {
  controller_self->shutdown_all_peripherals();
  controller_self->shutdown_all_interfaces();

  tx_thread_sleep(10);
  Error_Handler();
}

static void __handle_gnss_error(ULONG error_flags) {
  ULONG flags =
      GNSS_THREAD_COMPLETED_WITH_ERRORS | WAVES_THREAD_COMPLETED_WITH_ERRORS;

  // These sensor threads depend on being resumed when GNSS gets a fix.
  // So, if GNSS errors out, they won't ever terminate on their own.
  if (controller_self->global_config->turbidity_enabled) {
    flags |= TURBIDITY_THREAD_COMPLETED_WITH_ERRORS;
  }

  if (controller_self->global_config->light_enabled) {
    flags |= LIGHT_THREAD_COMPLETED_WITH_ERRORS;
  }

  // For testing in offices without GPS, I had to start the accelerometer
  // on GNSS error (alongside temp and ct). This actually seems like a good idea
  // for deployment as well; even without GNSS-based waves, we can get
  // accelerometer-based ones.
  //
  // if (controller_self->global_config->accelerometer_enabled) {
  //   flags |= ACCELEROMETER_THREAD_COMPLETED_WITH_ERRORS;
  // }

  // Shut down GNSS sensor
  HAL_GPIO_WritePin(GNSS_FET_GPIO_Port, GNSS_FET_Pin, GPIO_PIN_RESET);
  // Set all the fields of the SBD message to error values
  memset(controller_self->current_message, 0, sizeof(sbd_message_type_52));

  // Set the GNSS_THREAD_COMPLETED_WITH_ERRORS and
  // WAVES_THREAD_COMPLETED_WITH_ERRORS flags to prevent waves thread from
  // running
  (void)tx_event_flags_set(controller_self->complete_flags, flags, TX_OR);

  // Terminate the GNSS thread
  (void)tx_thread_suspend(controller_self->thread_handles->gnss_thread);
  (void)tx_thread_terminate(controller_self->thread_handles->gnss_thread);
}

static void __handle_ct_error(ULONG error_flag) {
  // Shut down CT sensor
  HAL_GPIO_WritePin(CT_FET_GPIO_Port, CT_FET_Pin, GPIO_PIN_RESET);

  memset(&controller_self->current_message->mean_temp,
         TELEMETRY_FIELD_ERROR_CODE, sizeof(real16_T));
  memset(&controller_self->current_message->mean_salinity,
         TELEMETRY_FIELD_ERROR_CODE, sizeof(real16_T));

  // Set the CT_COMPLETED_WITH_ERRORS flag
  (void)tx_event_flags_set(controller_self->complete_flags,
                           CT_THREAD_COMPLETED_WITH_ERRORS, TX_OR);

  (void)tx_thread_suspend(controller_self->thread_handles->ct_thread);
  (void)tx_thread_terminate(controller_self->thread_handles->ct_thread);
}

static void __handle_temperature_error(ULONG error_flag) {
  memset(&controller_self->current_message->mean_temp,
         TELEMETRY_FIELD_ERROR_CODE, sizeof(real16_T));

  // Set the TEMPERATURE_COMPLETED_WITH_ERRORS flag
  (void)tx_event_flags_set(controller_self->complete_flags,
                           TEMPERATURE_THREAD_COMPLETED_WITH_ERRORS, TX_OR);

  (void)tx_thread_suspend(controller_self->thread_handles->temperature_thread);
  (void)tx_thread_terminate(
      controller_self->thread_handles->temperature_thread);
}

static void __handle_turbidity_error(ULONG error_flag) {
  (void)tx_event_flags_set(controller_self->complete_flags,
                           TURBIDITY_THREAD_COMPLETED_WITH_ERRORS, TX_OR);

  (void)tx_thread_suspend(controller_self->thread_handles->turbidity_thread);
  (void)tx_thread_terminate(controller_self->thread_handles->turbidity_thread);
}

static void __handle_accel_error(ULONG error_flag) {
  (void)tx_event_flags_set(controller_self->complete_flags,
                           ACCELEROMETER_THREAD_COMPLETED_WITH_ERRORS, TX_OR);

  (void)tx_thread_suspend(controller_self->thread_handles->accel_thread);
  (void)tx_thread_terminate(controller_self->thread_handles->accel_thread);
}

static void __handle_light_error(ULONG error_flag) {
  (void)tx_event_flags_set(controller_self->complete_flags,
                           LIGHT_THREAD_COMPLETED_WITH_ERRORS, TX_OR);

  (void)tx_thread_suspend(controller_self->thread_handles->light_thread);
  (void)tx_thread_terminate(controller_self->thread_handles->light_thread);
}

static void __handle_waves_error(void) {
  real16_T Dp = {0};
  real16_T Hs = {0};
  real16_T Tp = {0};
  real16_T b_fmax = {0};
  real16_T b_fmin = {0};
  // Set all the fields of the SBD message to error values
  memset(controller_self->current_message, 0, sizeof(sbd_message_type_52));

  Dp.bitPattern = TELEMETRY_FIELD_ERROR_CODE;
  Hs.bitPattern = TELEMETRY_FIELD_ERROR_CODE;
  Tp.bitPattern = TELEMETRY_FIELD_ERROR_CODE;
  b_fmax.bitPattern = TELEMETRY_FIELD_ERROR_CODE;
  b_fmin.bitPattern = TELEMETRY_FIELD_ERROR_CODE;

  memcpy(&controller_self->current_message->Hs, &Hs, sizeof(real16_T));
  memcpy(&controller_self->current_message->Tp, &Tp, sizeof(real16_T));
  memcpy(&controller_self->current_message->Dp, &Dp, sizeof(real16_T));
  memcpy(&controller_self->current_message->f_min, &b_fmin, sizeof(real16_T));
  memcpy(&controller_self->current_message->f_max, &b_fmax, sizeof(real16_T));

  (void)tx_event_flags_set(controller_self->complete_flags,
                           WAVES_THREAD_COMPLETED_WITH_ERRORS, TX_OR);

  (void)tx_thread_suspend(controller_self->thread_handles->waves_thread);
  (void)tx_thread_terminate(controller_self->thread_handles->waves_thread);
}

static void __handle_iridium_error(ULONG error_flag) {
  // Shut down the modem
  HAL_GPIO_WritePin(IRIDIUM_OnOff_GPIO_Port, IRIDIUM_OnOff_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(BUS_5V_FET_GPIO_Port, BUS_5V_FET_Pin, GPIO_PIN_RESET);

  (void)tx_event_flags_set(controller_self->complete_flags,
                           IRIDIUM_THREAD_COMPLETED_WITH_ERRORS, TX_OR);

  (void)tx_thread_suspend(controller_self->thread_handles->iridium_thread);
  (void)tx_thread_terminate(controller_self->thread_handles->iridium_thread);
}
static void __handle_file_system_error(void) {
  HAL_GPIO_WritePin(SD_CARD_FET_GPIO_Port, SD_CARD_FET_Pin, GPIO_PIN_RESET);

  (void)tx_thread_suspend(controller_self->thread_handles->filex_thread);
  (void)tx_thread_terminate(controller_self->thread_handles->filex_thread);
}

static void __handle_i2c_error(void) {
  static uint32_t error_counter = 0;

  if (++error_counter > 5) {
    if (controller_self->global_config->temperature_enabled) {
      (void)tx_event_flags_set(controller_self->complete_flags,
                               TEMPERATURE_THREAD_COMPLETED_WITH_ERRORS, TX_OR);
    }

    if (controller_self->global_config->light_enabled) {
      (void)tx_event_flags_set(controller_self->complete_flags,
                               LIGHT_THREAD_COMPLETED_WITH_ERRORS, TX_OR);
    }

    if (controller_self->global_config->turbidity_enabled) {
      (void)tx_event_flags_set(controller_self->complete_flags,
                               TURBIDITY_THREAD_COMPLETED_WITH_ERRORS, TX_OR);
    }
  }
}

static void __handle_misc_error(ULONG error_flag) {
  if (error_flag & WATCHDOG_RESET) {
    LOG("Watchdog reset occured.");
    return;
  }

  if (error_flag & SOFTWARE_RESET) {
    LOG("Software reset occured.");
    return;
  }

  if (error_flag & MEMORY_CORRUPTION_ERROR) {
    LOG("Memory corruption detected.");

    controller_self->shutdown_all_peripherals();
    controller_self->shutdown_all_interfaces();
    // Clear out persistent ram if memory cannot be trusted
    persistent_ram_deinit();
    tx_thread_sleep(10);
    Error_Handler();
  }

  if (error_flag & FPU_EXCEPTION_OCCURRED) {
    LOG("Floating point exception occurred! Check NED Waves!");
  }
}
