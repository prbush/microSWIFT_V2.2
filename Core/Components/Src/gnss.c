/*
 * gnss.c
 *
 *  Created on: Oct 28, 2022
 *      Author: Phil
 *
 */

#include "ext_rtc_server.h"
#include "gnss.h"
#include "byte_array.h"
#include "app_threadx.h"
#include "threadx_support.h"
#include "tx_api.h"
#include "main.h"
#include "string.h"
#include "stm32u5xx_hal.h"
#include "stdio.h"
#include "stdbool.h"
#include "u_ubx_protocol.h"
#include "u_error_common.h"
#include "usart.h"
#include "linked_list.h"
#include "watchdog.h"
#include "logger.h"

static GNSS *gnss_self;

// Required buffers
static uint8_t ubx_message_process_buf[GNSS_MESSAGE_BUF_SIZE]__attribute__((section(".ram1"))) =
  { 0 };
static uint8_t gnss_config_response_buf[GNSS_CONFIG_BUFFER_SIZE]__attribute__((section(".ram1"))) =
  { 0 };
static gnss_track_point breadcrumb_track[BREADCRUMB_TRACK_MAX_SIZE] =
  { 0 };

extern DMA_QListTypeDef gnss_dma_linked_list;

// @formatter:off

// The configuration message, type UBX_CFG_VALSET. Default is set to 5Hz.
// !!!! This is output from U-Center 2 software, do not change !!!
static uint8_t config_array[CONFIGURATION_ARRAY_SIZE] =
  { 0xB5, 0x62, 0x06, 0x8A, 0x9C, 0x00, 0x01, 0x01, 0x00, 0x00, 0xBA, 0x00, 0x91, 0x20, 0x00,
    0xBE, 0x00, 0x91, 0x20, 0x00, 0xBB, 0x00, 0x91, 0x20, 0x00, 0xC9, 0x00, 0x91, 0x20, 0x00,
    0xCD, 0x00, 0x91, 0x20, 0x00, 0xCA, 0x00, 0x91, 0x20, 0x00, 0xBF, 0x00, 0x91, 0x20, 0x00,
    0xC3, 0x00, 0x91, 0x20, 0x00, 0xC0, 0x00, 0x91, 0x20, 0x00, 0xC4, 0x00, 0x91, 0x20, 0x00,
    0xC8, 0x00, 0x91, 0x20, 0x00, 0xC5, 0x00, 0x91, 0x20, 0x00, 0xAB, 0x00, 0x91, 0x20, 0x00,
    0xAF, 0x00, 0x91, 0x20, 0x00, 0xAC, 0x00, 0x91, 0x20, 0x00, 0xB0, 0x00, 0x91, 0x20, 0x00,
    0xB4, 0x00, 0x91, 0x20, 0x00, 0xB1, 0x00, 0x91, 0x20, 0x00, 0x07, 0x00, 0x91, 0x20, 0x01,
    0x21, 0x00, 0x11, 0x20, 0x08, 0x04, 0x00, 0x93, 0x10, 0x00, 0x01, 0x00, 0x21, 0x30, 0xC8,
    0x00, 0x02, 0x00, 0x21, 0x30, 0x01, 0x00, 0x07, 0x00, 0x92, 0x20, 0x00, 0x06, 0x00, 0x92,
    0x20, 0x00, 0x0A, 0x00, 0x92, 0x20, 0x00, 0x0D, 0x00, 0x31, 0x10, 0x00, 0x0F, 0x00, 0x31,
    0x10, 0x01, 0x18, 0x00, 0x31, 0x10, 0x01, 0xA4, 0x00, 0x11, 0x20, 0x14, 0x18, 0x5C };
// Struct functions
static uSWIFT_return_code_t _gnss_config ( void );
static uSWIFT_return_code_t _gnss_sync_and_start_reception ( void );
static uSWIFT_return_code_t _gnss_get_location ( float *latitude, float *longitude );
static uSWIFT_return_code_t _gnss_get_running_average_velocities ( void );
static uSWIFT_return_code_t _gnss_software_start ( void );
static uSWIFT_return_code_t _gnss_software_stop ( void );
static uSWIFT_return_code_t _gnss_set_rtc ( uint8_t *msg_payload );
static uSWIFT_return_code_t _gnss_reset_uart ( void );
static uSWIFT_return_code_t _gnss_start_timer ( uint16_t timeout_in_minutes );
static uSWIFT_return_code_t _gnss_stop_timer ( void );
static void                 _gnss_process_message ( void );
static void                 _gnss_on ( void );
static void                 _gnss_off ( void );

// Static helper functions
static uSWIFT_return_code_t __send_config ( uint8_t *config_array, size_t message_size,
                                          uint8_t response_class, uint8_t response_id );
static uSWIFT_return_code_t __enable_high_performance_mode ( void );
static uSWIFT_return_code_t __query_high_performance_mode ( void );
static uSWIFT_return_code_t __start_GNSS_UART_DMA ( uint8_t *buffer, size_t buffer_size );
static void                 __cycle_power ( void );
static void                 __process_frame_sync_messages ( uint8_t *process_buf );
static void                 __reset_struct_fields ( void );
// @formatter:on

/**
 * Initialize the GNSS struct
 *
 * @return void
 */
void gnss_init ( GNSS *struct_ptr, microSWIFT_configuration *global_config,
                 UART_HandleTypeDef *gnss_uart_handle, DMA_HandleTypeDef *gnss_tx_dma_handle,
                 DMA_HandleTypeDef *gnss_rx_dma_handle, TX_EVENT_FLAGS_GROUP *irq_flags,
                 TX_EVENT_FLAGS_GROUP *error_flags, TX_TIMER *timer, float *GNSS_N_Array,
                 float *GNSS_E_Array, float *GNSS_D_Array )
{
  gnss_self = struct_ptr;
  // initialize everything
  gnss_self->global_config = global_config;
  gnss_self->gnss_uart_handle = gnss_uart_handle;
  gnss_self->gnss_tx_dma_handle = gnss_tx_dma_handle;
  gnss_self->gnss_rx_dma_handle = gnss_rx_dma_handle;
  gnss_self->GNSS_N_Array = GNSS_N_Array;
  gnss_self->GNSS_E_Array = GNSS_E_Array;
  gnss_self->GNSS_D_Array = GNSS_D_Array;
  gnss_self->breadcrumb_track = &breadcrumb_track[0];
  gnss_self->breadcrumb_index = 0;

  __reset_struct_fields ();

  gnss_self->irq_flags = irq_flags;
  gnss_self->error_flags = error_flags;
  gnss_self->timer = timer;
  gnss_self->ubx_process_buf = &(ubx_message_process_buf[0]);
  gnss_self->config_response_buf = &(gnss_config_response_buf[0]);

  gnss_self->config = _gnss_config;
  gnss_self->sync_and_start_reception = _gnss_sync_and_start_reception;
  gnss_self->get_location = _gnss_get_location;
  gnss_self->get_running_average_velocities = _gnss_get_running_average_velocities;
  gnss_self->software_start = _gnss_software_start;
  gnss_self->software_stop = _gnss_software_stop;
  gnss_self->set_rtc = _gnss_set_rtc;
  gnss_self->reset_uart = _gnss_reset_uart;
  gnss_self->start_timer = _gnss_start_timer;
  gnss_self->stop_timer = _gnss_stop_timer;
  gnss_self->process_message = _gnss_process_message;
  gnss_self->on = _gnss_on;
  gnss_self->off = _gnss_off;
}

/**
 * Deinitialize UART, DMA
 *
 * @return void
 */
void gnss_deinit ( void )
{
  // Deinit UART and DMA
  HAL_UART_DeInit (gnss_self->gnss_uart_handle);
  HAL_DMA_DeInit (gnss_self->gnss_rx_dma_handle);
  HAL_DMA_DeInit (gnss_self->gnss_tx_dma_handle);

  // Just to be overly sure we don't get an erroneous IRQ
  HAL_NVIC_ClearPendingIRQ (LPDMA1_Channel0_IRQn);
  HAL_NVIC_ClearPendingIRQ (LPDMA1_Channel1_IRQn);
  HAL_NVIC_ClearPendingIRQ (LPUART1_IRQn);

  HAL_NVIC_DisableIRQ (LPDMA1_Channel0_IRQn);
  HAL_NVIC_DisableIRQ (LPDMA1_Channel1_IRQn);
  HAL_NVIC_DisableIRQ (LPUART1_IRQn);
}

/**
 * Timer expiered callback -- called via ThreadX contol services.
 *
 * @param expiration_input - unused
 */
void gnss_timer_expired ( ULONG expiration_input )
{
  (void) expiration_input;
  gnss_self->timer_timeout = true;
}

/**
 * Get status of GNSS timer.
 *
 * @return True if timer has timed out, false otherwise
 */
bool gnss_get_timer_timeout_status ( void )
{
  return gnss_self->timer_timeout;
}

/**
 * Get GNSS configuration status.
 *
 * @return True if configured, false otherwise
 */
bool gnss_get_configured_status ( void )
{
  return gnss_self->is_configured;
}

/**
 * Get GNSS all samples processed status.
 *
 * @return True if all samples received and processed, false otherwise
 */
bool gnss_get_sample_window_complete ( void )
{
  return gnss_self->all_samples_processed;
}

/**
 * Get current Latitude and Longitude. If a fix has not yet been resolved
 * and sampling has not started, will return uSWIFT_LOCATION_ERROR.
 *
 * @return uSWIFT_SUCCESS or uSWIFT_LOCATION_ERROR
 */
uSWIFT_return_code_t gnss_get_current_lat_lon ( int32_t *lat, int32_t *lon )
{
  if ( !gnss_self->all_resolution_stages_complete )
  {
    return uSWIFT_LOCATION_ERROR;
  }

  *lat = gnss_self->current_latitude;
  *lon = gnss_self->current_longitude;

  return uSWIFT_SUCCESS;
}

/**
 * Get the number of samples that have been processed.
 *
 * @return no. samples processed
 */
uint32_t gnss_get_samples_processed ( void )
{
  return (gnss_self->all_resolution_stages_complete) ?
      gnss_self->total_samples : 0;
}
/**
 * Get the sampling frequency calculated at the end of sampling.
 *
 * @return sample window frequency -- will be inaccurate if sample window is not complete
 */
double gnss_get_sample_window_frequency ( void )
{
  return gnss_self->sample_window_freq;
}

/**
 * Configure the MAX-M10S chip by sending a series of UBX_CFG_VALSET messages
 *
 * @return GNSS_SUCCESS or
 *                 GNSS_CONFIG_ERROR if response was not received
 */
static uSWIFT_return_code_t _gnss_config ( void )
{
  uSWIFT_return_code_t return_code;

  if ( gnss_self->global_config->gnss_sampling_rate == 4 )
  {
    config_array[119] = 0xFA;
    config_array[162] = 0x4A;
    config_array[163] = 0xC2;
  }

// Send over the configuration settings for RAM
  return_code = __send_config (&(config_array[0]), CONFIGURATION_ARRAY_SIZE, UBX_CFG_VALSET_CLASS,
  UBX_CFG_VALSET_ID);

  if ( return_code != uSWIFT_SUCCESS )
  {
    gnss_self->reset_uart ();
    return return_code;
  }

  // Only need to program battery backed RAM once on first power up
  if ( is_first_sample_window () )
  {
    // Only one value (configuration layer) and the checksum change between RAM
    // and Battery-backed-RAM, so we'll adjust that now
    config_array[7] = 0x02;

    if ( gnss_self->global_config->gnss_sampling_rate == 4 )
    {
      config_array[162] = 0x4B;
      config_array[163] = 0x5D;
    }
    else
    { // 4Hz
      config_array[162] = 0x19;
      config_array[163] = 0xF7;
    }

    // Send over the Battery Backed Ram (BBR) config settings
    return_code = __send_config (&(config_array[0]), CONFIGURATION_ARRAY_SIZE, UBX_CFG_VALSET_CLASS,
    UBX_CFG_VALSET_ID);

    if ( return_code != uSWIFT_SUCCESS )
    {
      gnss_self->reset_uart ();
      return return_code;
    }

    gnss_self->reset_uart ();

    // Now set high performance mode (if enabled)
    if ( gnss_self->global_config->gnss_high_performance_mode )
    {
      // First check to see if it has already been set
      return_code = __enable_high_performance_mode ();
    }
  }

  tx_thread_sleep (20);

  return return_code;
}

/**
 *
 *
 * @return uSWIFT_return_code_t
 */
static uSWIFT_return_code_t _gnss_sync_and_start_reception ( void )
{
  uSWIFT_return_code_t return_code = uSWIFT_SUCCESS;
  ULONG actual_flags;
  uint8_t msg_buf[INITIAL_STAGES_BUFFER_SIZE];
  int max_ticks_to_get_message = round (
      (((float) ((float) INITIAL_STAGES_BUFFER_SIZE / (float) UBX_NAV_PVT_MESSAGE_LENGTH))
       * ((float) ((float) TX_TIMER_TICKS_PER_SECOND
                   / (float) gnss_self->global_config->gnss_sampling_rate)))
      + 100);
  uint32_t watchdog_counter = 0;

  __HAL_UART_ENABLE_IT(&huart3, UART_IT_ERR);

  // Zero out the message buffer
  memset (&(msg_buf[0]), 0, INITIAL_STAGES_BUFFER_SIZE);

  // Grabbing and processing 5 samples takes ~ 1 second, so we'll keep trying until we hit
  // the gnss_max_acquisition_wait_time
  while ( !gnss_self->timer_timeout )
  {
    if ( ++watchdog_counter % 5 == 0 )
    {
      watchdog_check_in (GNSS_THREAD);
      watchdog_counter = 0;
    }
    // Grab 5 UBX_NAV_PVT messages
    HAL_UART_Receive_DMA (gnss_self->gnss_uart_handle, &(msg_buf[0]),
    INITIAL_STAGES_BUFFER_SIZE);

    if ( tx_event_flags_get (gnss_self->irq_flags, GNSS_CONFIG_RECVD, TX_OR_CLEAR, &actual_flags,
                             max_ticks_to_get_message)
         != TX_SUCCESS )
    {
      // If we didn't receive the needed messaged in time, cycle the GNSS sensor
      __cycle_power ();
      HAL_UART_DMAStop (gnss_self->gnss_uart_handle);
      tx_thread_sleep (23);
      gnss_self->reset_uart ();
      continue;
    }

    __process_frame_sync_messages (msg_buf);
    // this both ensures we have frame sync'd with the GNSS sensor and are safe
    // to kick off circular DMA receive
    if ( gnss_self->messages_processed == gnss_self->global_config->gnss_sampling_rate
         && gnss_self->number_cycles_without_data == 0
         && gnss_self->total_samples == gnss_self->global_config->gnss_sampling_rate )
    {
      return_code = uSWIFT_SUCCESS;
      break;
    }
    else
    {
      // Short delay to help get the frame sync'd
      HAL_UART_DMAStop (gnss_self->gnss_uart_handle);
      tx_thread_sleep (23);
      gnss_self->reset_uart ();
    }
  }

  watchdog_check_in (GNSS_THREAD);

  if ( gnss_self->timer_timeout )
  {
    return uSWIFT_TIMEOUT;
  }

  // Just to be overly sure we're starting the sampling window from a fresh slate
  __reset_struct_fields ();

  return_code = __start_GNSS_UART_DMA (&(gnss_self->ubx_process_buf[0]),
  UBX_NAV_PVT_MESSAGE_LENGTH);
  watchdog_check_in (GNSS_THREAD);
  // Make sure we start right next time around in case there was an issue starting DMA
  if ( return_code == uSWIFT_IO_ERROR )
  {
    HAL_UART_DMAStop (gnss_self->gnss_uart_handle);
    gnss_self->reset_uart ();
    memset (&(gnss_self->ubx_process_buf[0]), 0, GNSS_MESSAGE_BUF_SIZE);
  }

  gnss_self->is_configured = (return_code == uSWIFT_SUCCESS);
  gnss_self->warmup_counter = 0;
  gnss_self->total_samples = 0;

  return return_code;
}

/**
 * Get the current lat/long. We're going to return the lat/long no matter what,
 * but the return code will indicate if it's any good.
 * !!! Only valid when the GNSS is on and procesing samples or shortly thereafter
 *
 * @param latitude - return parameter for latitude
 * @param longitude - return parameter for longitude
 * @return uSWIFT_return_code_t
 */
static uSWIFT_return_code_t _gnss_get_location ( float *latitude, float *longitude )
{
  uSWIFT_return_code_t return_code = uSWIFT_SUCCESS;

  if ( !gnss_self->current_fix_is_good )
  {
    return_code = uSWIFT_REQUEST_INVALID;
  }

  *latitude = ((float) gnss_self->current_latitude) / ((float) LAT_LON_CONVERSION_FACTOR);
  *longitude = ((float) gnss_self->current_longitude) / ((float) LAT_LON_CONVERSION_FACTOR);

  return return_code;
}

/**
 * If a velocity field > MAX_POSSIBLE_VELOCITY, or the velocity accuracy
 * estimate (vAcc) is outside acceptable range, this function will substitute
 * a running average.
 *
 * @param returnNorth - return parameter for the running average North value
 * @param returnEast - return parameter for the running average East value
 * @param returnDown - return parameter for the running average Down value
 * @return GPS error code (marcos defined in gps_error_codes.h)
 */
static uSWIFT_return_code_t _gnss_get_running_average_velocities ( void )
{
  uSWIFT_return_code_t return_code = uSWIFT_SUCCESS;
  float substitute_north, substitute_east, substitute_down;

  if ( gnss_self->total_samples >= gnss_self->global_config->gnss_samples_per_window )
  {

    return_code = uSWIFT_DONE_SAMPLING;

  }
  // avoid a divide by zero error
  else if ( gnss_self->total_samples == 0 )
  {

    return_code = uSWIFT_NO_SAMPLES_ERROR;

  }
  // Good to replace value with running average
  else
  {

    substitute_north = (((float) gnss_self->v_north_sum) / MM_PER_METER)
                       / ((float) gnss_self->total_samples);
    substitute_east = (((float) gnss_self->v_east_sum) / MM_PER_METER)
                      / ((float) gnss_self->total_samples);
    substitute_down = (((float) gnss_self->v_down_sum) / MM_PER_METER)
                      / ((float) gnss_self->total_samples);

    gnss_self->GNSS_N_Array[gnss_self->total_samples] = substitute_north;
    gnss_self->GNSS_E_Array[gnss_self->total_samples] = substitute_east;
    gnss_self->GNSS_D_Array[gnss_self->total_samples] = substitute_down;

    gnss_self->total_samples++;
    gnss_self->total_samples_averaged++;
  }

  return return_code;
}

/**
 * Send a CFG_RST message to the GNSS chip to either start GNSS
 * processing. This message is not acknowledged, so we just have to trust that
 * it worked.
 *
 * @param gnss_self- GNSS struct
 */
static uSWIFT_return_code_t _gnss_software_start ( void )
{
  ULONG actual_flags;
  // 3rd byte -- 0x08 = Controlled GNSS stop, 0x09 = Controlled GNSS start
  uint8_t message_payload[4] =
    { 0x00, 0x00, 0x09, 0x00 };
  char cfg_rst_message[sizeof(message_payload) + U_UBX_PROTOCOL_OVERHEAD_LENGTH_BYTES];

  if ( (uUbxProtocolEncode (0x06, 0x04, (const char*) &(message_payload[0]),
                            sizeof(message_payload), cfg_rst_message))
       < 0 )
  {
    gnss_self->reset_uart ();
    return uSWIFT_CONFIGURATION_ERROR;
  }

  if ( (HAL_UART_Transmit_DMA (gnss_self->gnss_uart_handle, (uint8_t*) &(cfg_rst_message[0]),
                               sizeof(cfg_rst_message)))
       != HAL_OK )
  {
    gnss_self->reset_uart ();
    return uSWIFT_CONFIGURATION_ERROR;
  }

// Make sure the transmission went through completely
  if ( tx_event_flags_get (gnss_self->irq_flags, GNSS_TX_COMPLETE, TX_OR_CLEAR, &actual_flags,
  TX_TIMER_TICKS_PER_SECOND)
       != TX_SUCCESS )
  {
    HAL_UART_DMAStop (gnss_self->gnss_uart_handle);
    gnss_self->reset_uart ();
    tx_thread_sleep (TX_TIMER_TICKS_PER_SECOND / 10);
    return uSWIFT_IO_ERROR;

  }

  return uSWIFT_SUCCESS;
}

static uSWIFT_return_code_t _gnss_software_stop ( void )
{
  ULONG actual_flags;
  // 3rd byte -- 0x08 = Controlled GNSS stop, 0x09 = Controlled GNSS start
  uint8_t message_payload[4] =
    { 0x00, 0x00, 0x08, 0x00 };
  char cfg_rst_message[sizeof(message_payload) + U_UBX_PROTOCOL_OVERHEAD_LENGTH_BYTES];

  if ( (uUbxProtocolEncode (0x06, 0x04, (const char*) &(message_payload[0]),
                            sizeof(message_payload), cfg_rst_message))
       < 0 )
  {
    gnss_self->reset_uart ();
    return uSWIFT_CONFIGURATION_ERROR;
  }

  if ( (HAL_UART_Transmit_DMA (gnss_self->gnss_uart_handle, (uint8_t*) &(cfg_rst_message[0]),
                               sizeof(cfg_rst_message)))
       != HAL_OK )
  {
    gnss_self->reset_uart ();
    return uSWIFT_CONFIGURATION_ERROR;
  }

// Make sure the transmission went through completely
  if ( tx_event_flags_get (gnss_self->irq_flags, GNSS_TX_COMPLETE, TX_OR_CLEAR, &actual_flags,
  TX_TIMER_TICKS_PER_SECOND)
       != TX_SUCCESS )
  {
    HAL_UART_DMAStop (gnss_self->gnss_uart_handle);
    gnss_self->reset_uart ();
    tx_thread_sleep (TX_TIMER_TICKS_PER_SECOND / 10);
    return uSWIFT_IO_ERROR;

  }

  return uSWIFT_SUCCESS;
}

/**
 * Set the RTC clock.
 *
 * @param GNSS - GNSS struct
 * @param msg_payload - UBX_NAV_PVT message payload containing
 *        time information.
 *
 * @return GNSS_SUCCESS or
 *                 GNSS_RTC_ERROR - if setting RTC returned an error
 */
static uSWIFT_return_code_t _gnss_set_rtc ( uint8_t *msg_payload )
{
  uSWIFT_return_code_t return_code = uSWIFT_SUCCESS;
  uSWIFT_return_code_t rtc_ret = uSWIFT_SUCCESS;
  struct tm time;

  uint16_t year = (int16_t) get_two_bytes (msg_payload, UBX_NAV_PVT_YEAR_INDEX, AS_LITTLE_ENDIAN);
  uint8_t month = msg_payload[UBX_NAV_PVT_MONTH_INDEX];
  uint8_t day = msg_payload[UBX_NAV_PVT_DAY_INDEX];
  uint8_t hour = msg_payload[UBX_NAV_PVT_HOUR_INDEX];
  uint8_t min = msg_payload[UBX_NAV_PVT_MINUTE_INDEX];
  uint8_t sec = msg_payload[UBX_NAV_PVT_SECONDS_INDEX];
  uint8_t time_flags = msg_payload[UBX_NAV_PVT_VALID_FLAGS_INDEX];

  time_flags &= LOWER_4_BITS_MASK;

  if ( !(time_flags & RESOLVED_TIME_BITS) )
  {
    return_code = uSWIFT_TIMER_ERROR;
    return return_code;
  }

  time.tm_year = year;
  time.tm_mon = month;
  time.tm_mday = day;
  time.tm_wday = WEEKDAY_UNKNOWN;
  time.tm_hour = hour;
  time.tm_min = min;
  time.tm_sec = sec;

  rtc_ret = rtc_server_set_time (&time, GNSS_REQUEST_PROCESSED);

  if ( rtc_ret != uSWIFT_SUCCESS )
  {
    return_code = uSWIFT_TIMER_ERROR;
    gnss_self->rtc_error = true;
    tx_event_flags_set (gnss_self->error_flags, RTC_ERROR, TX_OR);
    return return_code;
  }

  gnss_self->is_clock_set = true;
  gnss_self->rtc_error = false;

  return return_code;
}

/**
 * Reinitialize the GNSS UART port. Required when switching between Tx and Rx.
 *
 * @param gnss_self - GNSS struct
 * @param baud_rate - baud rate to set port to
 */
static uSWIFT_return_code_t _gnss_reset_uart ( void )
{

  if ( lpuart1_deinit () != UART_OK )
  {
    return uSWIFT_IO_ERROR;
  }

  if ( lpuart1_init () != UART_OK )
  {
    return uSWIFT_IO_ERROR;
  }

  return uSWIFT_SUCCESS;
}

/**
 * Start the timer.
 *
 * @param gnss_self - GNSS struct
 * @param timeout_in_minutes - timeout in minutes
 */
static uSWIFT_return_code_t _gnss_start_timer ( uint16_t timeout_in_minutes )
{
  ULONG timeout = TX_TIMER_TICKS_PER_SECOND * 60 * timeout_in_minutes;
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;

  if ( tx_timer_change (gnss_self->timer, timeout, 0) != TX_SUCCESS )
  {
    ret = uSWIFT_TIMER_ERROR;
    return ret;
  }

  if ( tx_timer_activate (gnss_self->timer) != TX_SUCCESS )
  {
    ret = uSWIFT_TIMER_ERROR;
  }

  return ret;
}

/**
 * Stop the timer.
 *
 * @param gnss_self - GNSS struct
 * @param timeout_in_minutes - timeout in minutes
 */
static uSWIFT_return_code_t _gnss_stop_timer ( void )
{
  return (tx_timer_deactivate (gnss_self->timer) == TX_SUCCESS) ?
      uSWIFT_SUCCESS : uSWIFT_TIMER_ERROR;
}

/**
 * Process the messages in the buffer.
 *
 * @return uSWIFT_return_code_t
 */
static void _gnss_process_message ( void )
{
  uint8_t payload[UBX_NAV_PVT_PAYLOAD_LENGTH];
  const char *buf_start = (const char*) &(gnss_self->ubx_process_buf[0]);
  const char *buf_end = buf_start;
  // Our input buffer is a message off the queue, 10 UBX_NAV_PVT msgs
  size_t buf_length = UBX_MESSAGE_SIZE * 2;
  int32_t message_class = 0;
  int32_t message_id = 0;
  int32_t num_payload_bytes = 0;
  int32_t lat, lon, vnorth, veast, vdown;
  int16_t flags3;
  bool is_ubx_nav_pvt_msg, message_checksum_valid = false;
  bool velocities_non_zero;

  // Really gross for loop that processes msgs in each iteration
  for ( num_payload_bytes = uUbxProtocolDecode (buf_start, buf_length, &message_class, &message_id,
                                                (char*) payload, sizeof(payload), &buf_end);
      num_payload_bytes > 0;
      num_payload_bytes = uUbxProtocolDecode (buf_start, buf_length, &message_class, &message_id,
                                              (char*) payload, sizeof(payload), &buf_end) )
  {

    message_checksum_valid = true;

    // UBX_NAV_PVT payload is 92 bytes, message class is 0x01, message ID is 0x07
    is_ubx_nav_pvt_msg = (num_payload_bytes == UBX_NAV_PVT_PAYLOAD_LENGTH)
                         || (message_class == UBX_NAV_PVT_MESSAGE_CLASS)
                         || (message_id == UBX_NAV_PVT_MESSAGE_ID);

    if ( !is_ubx_nav_pvt_msg )
    {
      gnss_self->get_running_average_velocities ();
      gnss_self->number_cycles_without_data++;
      buf_length -= buf_end - buf_start;
      buf_start = buf_end;
      continue;
    }

    // Even if we don't end up using the values, we did get a valid message
    gnss_self->messages_processed++;

    // Grab a bunch of things from the message
    lon = (int32_t) get_four_bytes (payload, UBX_NAV_PVT_LON_INDEX, AS_LITTLE_ENDIAN);
    lat = (int32_t) get_four_bytes (payload, UBX_NAV_PVT_LAT_INDEX, AS_LITTLE_ENDIAN);
    flags3 = (int16_t) get_two_bytes (payload, UBX_NAV_PVT_FLAGS3_INDEX, AS_LITTLE_ENDIAN);
    vnorth = (int32_t) get_four_bytes (payload, UBX_NAV_PVT_V_NORTH_INDEX, AS_LITTLE_ENDIAN);
    veast = (int32_t) get_four_bytes (payload, UBX_NAV_PVT_V_EAST_INDEX, AS_LITTLE_ENDIAN);
    vdown = (int32_t) get_four_bytes (payload, UBX_NAV_PVT_V_DOWN_INDEX, AS_LITTLE_ENDIAN);

    // This allows us to make sure we're not in the sampling window if time has not been resolved
    if ( !gnss_self->is_clock_set )
    {
      if ( gnss_self->set_rtc ((uint8_t*) payload) != uSWIFT_SUCCESS )
      {
        buf_length -= buf_end - buf_start;
        buf_start = buf_end;
        continue;
      }

      LOG("GNSS time fully resolved and RTC set.");
    }

    // We'll always retain the lat/lon and use a flag to indicate if it is any good
    gnss_self->current_fix_is_good = !(flags3 & LAT_LON_INVALID_BIT);
    gnss_self->current_latitude = lat;
    gnss_self->current_longitude = lon;

    // individual velocities are less than MAX_POSSIBLE_VELOCITY
    velocities_non_zero = (vnorth != 0) && (veast != 0) && (vdown != 0);

    // First sample has not yet resolved velocities
    if ( ((gnss_self->total_samples == 0) && !velocities_non_zero) )
    {
      buf_length -= buf_end - buf_start;
      buf_start = buf_end;
      continue;
    }

    // Did we have at least 1 good sample?
    if ( (gnss_self->total_samples == 0) && velocities_non_zero )
    {
      if ( ++gnss_self->warmup_counter > (60U * gnss_self->global_config->gnss_sampling_rate) )
      {
        gnss_self->all_resolution_stages_complete = true;
        gnss_self->sample_window_start_time = get_system_time ();

        LOG("GNSS sample window started.");
      }
      else
      {
        buf_length -= buf_end - buf_start;
        buf_start = buf_end;
        continue;
      }
    }

    // All velocity values are good to go
    gnss_self->v_north_sum += vnorth;
    gnss_self->v_east_sum += veast;
    gnss_self->v_down_sum += vdown;

    // Assign to the velocity arrays
    gnss_self->GNSS_N_Array[gnss_self->total_samples] = ((float) ((float) vnorth) / MM_PER_METER);
    gnss_self->GNSS_E_Array[gnss_self->total_samples] = ((float) ((float) veast) / MM_PER_METER);
    gnss_self->GNSS_D_Array[gnss_self->total_samples] = ((float) ((float) vdown) / MM_PER_METER);

    gnss_self->number_cycles_without_data = 0;

    if ( (gnss_self->total_samples % gnss_self->global_config->gnss_sampling_rate == 0)
         && (gnss_self->breadcrumb_index < BREADCRUMB_TRACK_MAX_SIZE) )
    {
      gnss_self->breadcrumb_track[gnss_self->breadcrumb_index].lat =
          ((float) lat) / ((float) LAT_LON_CONVERSION_FACTOR);
      gnss_self->breadcrumb_track[gnss_self->breadcrumb_index].lon =
          ((float) lon) / ((float) LAT_LON_CONVERSION_FACTOR);
      gnss_self->breadcrumb_index++;
    }

    gnss_self->total_samples++;

    // Catch the end condition
    if ( gnss_self->total_samples >= gnss_self->global_config->gnss_samples_per_window )
    {
      HAL_UART_DMAStop (gnss_self->gnss_uart_handle);
      gnss_self->sample_window_stop_time = get_system_time ();
      gnss_self->all_samples_processed = true;
      gnss_self->sample_window_freq = (double) (((double) gnss_self->global_config
          ->gnss_samples_per_window)
                                                / (((double) (((double) gnss_self
                                                    ->sample_window_stop_time)
                                                              - ((double) gnss_self
                                                                  ->sample_window_start_time)))));
      return;
    }

    buf_length -= buf_end - buf_start;
    buf_start = buf_end;
  }

  // If the checksum was invalid, replace with running average
  if ( !message_checksum_valid )
  {
    gnss_self->get_running_average_velocities ();
    gnss_self->num_bad_messages++;
  }
}

/**
 * Switch the FET controlling power to the GNSS unit.
 *
 * @param gnss_self - GNSS struct
 *
 * @return void
 */
static void _gnss_on ( void )
{
  HAL_GPIO_WritePin (GNSS_FET_GPIO_Port, GNSS_FET_Pin, GPIO_PIN_SET);
}

/**
 * Switch the FET controlling power to the GNSS unit.
 *
 * @param gnss_self - GNSS struct
 *
 * @return void
 */
static void _gnss_off ( void )
{
  HAL_GPIO_WritePin (GNSS_FET_GPIO_Port, GNSS_FET_Pin, GPIO_PIN_RESET);
}

/**
 * Send a configuration to the GNSS chip. Will retry up to 10 times before
 * returning failure.
 *
 * @param gnss_self- GNSS struct
 * @param config_array - byte array containing a UBX_CFG_VALSET msg with up to
 *                64 keys
 */
static uSWIFT_return_code_t __send_config ( uint8_t *config_array, size_t message_size,
                                            uint8_t response_class, uint8_t response_id )
{
  int ticks_to_receive_msgs = round (
      (((float) ((float) GNSS_CONFIG_BUFFER_SIZE / (float) UBX_NAV_PVT_MESSAGE_LENGTH))
       * ((float) ((float) TX_TIMER_TICKS_PER_SECOND
                   / (float) gnss_self->global_config->gnss_sampling_rate)))
      + 250);
  ;
  ULONG actual_flags;
  char payload[UBX_NAV_PVT_PAYLOAD_LENGTH];
  const char *buf_start = (const char*) gnss_self->config_response_buf;
  const char *buf_end = buf_start;
  size_t buf_length = 600;
  int32_t message_class = 0;
  int32_t message_id = 0;
  int32_t num_payload_bytes = 0;
  uint8_t response_msg_class;
  uint8_t response_msg_id;

  HAL_UART_DMAStop (gnss_self->gnss_uart_handle);
  gnss_self->reset_uart ();

  // Start with a blank msg_buf -- this will short cycle the for loop
  // below if a message was not received in 10 tries
  memset (&(gnss_self->config_response_buf[0]), 0, GNSS_CONFIG_BUFFER_SIZE);
  // Send over the configuration settings
  HAL_UART_Transmit_DMA (gnss_self->gnss_uart_handle, &(config_array[0]), message_size);

  if ( tx_event_flags_get (gnss_self->irq_flags, GNSS_TX_COMPLETE, TX_OR_CLEAR, &actual_flags,
                           250) != TX_SUCCESS )
  {
    HAL_UART_DMAStop (gnss_self->gnss_uart_handle);
    gnss_self->reset_uart ();
    tx_thread_sleep (10);
    return uSWIFT_CONFIGURATION_ERROR;
  }

  // Grab the acknowledgment message
  HAL_UART_Receive_DMA (gnss_self->gnss_uart_handle, &(gnss_self->config_response_buf[0]),
  GNSS_CONFIG_BUFFER_SIZE);

  if ( tx_event_flags_get (gnss_self->irq_flags, GNSS_CONFIG_RECVD, TX_OR_CLEAR, &actual_flags,
                           ticks_to_receive_msgs)
       != TX_SUCCESS )
  {
    HAL_UART_DMAStop (gnss_self->gnss_uart_handle);
    gnss_self->reset_uart ();
    tx_thread_sleep (10);
    return uSWIFT_CONFIGURATION_ERROR;
  }

  for ( num_payload_bytes = uUbxProtocolDecode (buf_start, buf_length, &message_class, &message_id,
                                                payload, sizeof(payload), &buf_end);
      num_payload_bytes > 0;
      num_payload_bytes = uUbxProtocolDecode (buf_start, buf_length, &message_class, &message_id,
                                              payload, sizeof(payload), &buf_end) )
  {
    if ( message_class == 0x05 )
    {
      // Msg class 0x05 is either an ACK or NAK
      if ( message_id == 0x00 )
      {
        // This is a NAK msg, the config did not go through properly
        return uSWIFT_NAK_RECEIVED;
      }

      if ( message_id == 0x01 )
      {
        // This is an ACK message
        response_msg_class = payload[UBX_ACK_ACK_CLSID_INDEX];
        response_msg_id = payload[UBX_ACK_ACK_MSGID_INDEX];

        // Make sure this is an ack for the CFG_VALSET message type
        if ( response_msg_class == response_class && response_msg_id == response_id )
        {
          // This is an acknowledgement of our configuration message
          gnss_self->reset_uart ();
          tx_thread_sleep (TX_TIMER_TICKS_PER_SECOND / 10);
          return uSWIFT_SUCCESS;
        }
      }
    }
    // Check if it's a NAVPVT msg
    else if ( (num_payload_bytes == UBX_NAV_PVT_PAYLOAD_LENGTH)
              || (message_class == UBX_NAV_PVT_MESSAGE_CLASS)
              || (message_id == UBX_NAV_PVT_MESSAGE_ID) )
    {
      // GNSS is already configured, so we're good
      gnss_self->reset_uart ();
      tx_thread_sleep (TX_TIMER_TICKS_PER_SECOND / 10);
      return uSWIFT_SUCCESS;
    }
    // Adjust pointers to continue searching the buffer
    buf_length -= buf_end - buf_start;
    buf_start = buf_end;
  }

  // If we made it here, the ack message was not in the buffer
  HAL_UART_DMAStop (gnss_self->gnss_uart_handle);
  gnss_self->reset_uart ();
  return uSWIFT_CONFIGURATION_ERROR;
}

/**
 *
 *
 * @param gnss_self- GNSS struct
 * @param
 */
static void __process_frame_sync_messages ( uint8_t *process_buf )
{
  char payload[UBX_NAV_PVT_PAYLOAD_LENGTH];
  const char *buf_start = (const char*) &(process_buf[0]);
  const char *buf_end = buf_start;

  size_t buf_length = INITIAL_STAGES_BUFFER_SIZE;
  int32_t message_class = 0;
  int32_t message_id = 0;
  int32_t num_payload_bytes = 0;
// Reset the counters
  gnss_self->messages_processed = 0;
  gnss_self->number_cycles_without_data = 0;
  gnss_self->total_samples = 0;

// Really gross for loop that processes msgs in each iteration
  for ( num_payload_bytes = uUbxProtocolDecode (buf_start, buf_length, &message_class, &message_id,
                                                payload, sizeof(payload), &buf_end);
      num_payload_bytes > 0;
      num_payload_bytes = uUbxProtocolDecode (buf_start, buf_length, &message_class, &message_id,
                                              payload, sizeof(payload), &buf_end) )
  {
    // UBX_NAV_PVT payload is 92 bytes, message class is 0x01,
    // message ID is 0x07
    if ( num_payload_bytes != UBX_NAV_PVT_PAYLOAD_LENGTH
         || message_class != UBX_NAV_PVT_MESSAGE_CLASS || message_id != UBX_NAV_PVT_MESSAGE_ID )
    {
      gnss_self->number_cycles_without_data++;
      buf_length -= buf_end - buf_start;
      buf_start = buf_end;
      continue;
    }

    // need to keep track of how many messages were processed in the buffer
    gnss_self->messages_processed++;
    gnss_self->number_cycles_without_data = 0;
    gnss_self->total_samples++;

    buf_length -= buf_end - buf_start;
    buf_start = buf_end;
  }
}

/**
 *
 *
 * @param gnss_self- GNSS struct
 * @param
 */
static uSWIFT_return_code_t __enable_high_performance_mode ( void )
{
  uSWIFT_return_code_t return_code;
  int config_step_attempts = 0;
  uint8_t enable_high_performance_mode[ENABLE_HIGH_PERFORMANCE_SIZE] =
    { 0xB5, 0x62, 0x06, 0x41, 0x10, 0x00, 0x03, 0x00, 0x04, 0x1F, 0x54, 0x5E, 0x79, 0xBF, 0x28,
      0xEF, 0x12, 0x05, 0xFD, 0xFF, 0xFF, 0xFF, 0x8F, 0x0D, 0xB5, 0x62, 0x06, 0x41, 0x1C, 0x00,
      0x04, 0x01, 0xA4, 0x10, 0xBD, 0x34, 0xF9, 0x12, 0x28, 0xEF, 0x12, 0x05, 0x05, 0x00, 0xA4,
      0x40, 0x00, 0xB0, 0x71, 0x0B, 0x0A, 0x00, 0xA4, 0x40, 0x00, 0xD8, 0xB8, 0x05, 0xDE, 0xAE };

  while ( config_step_attempts < MAX_CONFIG_STEP_ATTEMPTS )
  {
    watchdog_check_in (GNSS_THREAD);
    return_code = __query_high_performance_mode ();

    switch ( return_code )
    {
      case uSWIFT_NAK_RECEIVED:
        // Zero out the config response buffer
        memset (gnss_self->config_response_buf, 0, GNSS_CONFIG_BUFFER_SIZE);
        config_step_attempts = 0;

        // Now send over the command to enable high performance mode
        while ( config_step_attempts < MAX_CONFIG_STEP_ATTEMPTS )
        {
          return_code = __send_config (&(enable_high_performance_mode[0]),
          ENABLE_HIGH_PERFORMANCE_SIZE,
                                       0x06, 0x41);

          if ( return_code != uSWIFT_SUCCESS )
          {
            config_step_attempts++;
          }
          else
          {
            break;
          }
        }

        if ( config_step_attempts == MAX_CONFIG_STEP_ATTEMPTS )
        {
          HAL_UART_DMAStop (gnss_self->gnss_uart_handle);
          gnss_self->reset_uart ();
          tx_thread_sleep (TX_TIMER_TICKS_PER_SECOND / 10);
          return_code = uSWIFT_CONFIGURATION_ERROR;
          return return_code;
        }

        // Must cycle power before the high performance mode will kick in
        __cycle_power ();

        tx_thread_sleep (TX_TIMER_TICKS_PER_SECOND / 10);

        // Zero out the config response buffer
        memset (gnss_self->config_response_buf, 0, GNSS_CONFIG_BUFFER_SIZE);
        config_step_attempts = 0;

        // Now check to see if the changes stuck
        while ( config_step_attempts < MAX_CONFIG_STEP_ATTEMPTS )
        {
          return_code = __query_high_performance_mode ();

          if ( return_code != uSWIFT_SUCCESS )
          {
            config_step_attempts++;
          }
          else
          {
            break;
          }
        }

        if ( config_step_attempts == MAX_CONFIG_STEP_ATTEMPTS )
        {
          HAL_UART_DMAStop (gnss_self->gnss_uart_handle);
          gnss_self->reset_uart ();
          tx_thread_sleep (TX_TIMER_TICKS_PER_SECOND / 10);
          return_code = uSWIFT_CONFIGURATION_ERROR;
          return return_code;
        }

        return_code = uSWIFT_SUCCESS;
        return return_code;

      case uSWIFT_SUCCESS:
        return return_code;

      case uSWIFT_IO_ERROR:
        config_step_attempts++;
        break;

      case uSWIFT_CONFIGURATION_ERROR:
        config_step_attempts++;
        break;

      default:
        return uSWIFT_UNKNOWN_ERROR;
    }
  }

  if ( config_step_attempts == MAX_CONFIG_STEP_ATTEMPTS )
  {
    HAL_UART_DMAStop (gnss_self->gnss_uart_handle);
    gnss_self->reset_uart ();
    tx_thread_sleep (TX_TIMER_TICKS_PER_SECOND / 10);
    return_code = uSWIFT_CONFIGURATION_ERROR;
    return return_code;
  }

  return uSWIFT_UNKNOWN_ERROR;
}

/**
 *
 *
 * @param gnss_self- GNSS struct
 * @param
 */
static uSWIFT_return_code_t __query_high_performance_mode ( void )
{
  uSWIFT_return_code_t return_code;
  ULONG actual_flags;
  uint8_t msg_buf[GNSS_CONFIG_BUFFER_SIZE];
  char payload[UBX_NAV_PVT_PAYLOAD_LENGTH];
  const char *buf_start = (const char*) gnss_self->config_response_buf;
  const char *buf_end = buf_start;
  size_t buf_length = 600;
  int32_t message_class = 0;
  int32_t message_id = 0;
  int32_t num_payload_bytes = 0;
  uint8_t high_performance_mode_query[HIGH_PERFORMANCE_QUERY_SIZE] =
    { 0xB5, 0x62, 0x06, 0x8B, 0x14, 0x00, 0x00, 0x04, 0x00, 0x00, 0x01, 0x00, 0xA4, 0x40, 0x03,
      0x00, 0xA4, 0x40, 0x05, 0x00, 0xA4, 0x40, 0x0A, 0x00, 0xA4, 0x40, 0x4C, 0x15 };
  uint8_t high_performance_mode_response[HIGH_PERFORMANCE_RESPONSE_SIZE] =
    { 0x01, 0x04, 0x00, 0x00, 0x01, 0x00, 0xA4, 0x40, 0x00, 0xB0, 0x71, 0x0B, 0x03, 0x00, 0xA4,
      0x40, 0x00, 0xB0, 0x71, 0x0B, 0x05, 0x00, 0xA4, 0x40, 0x00, 0xB0, 0x71, 0x0B, 0x0A, 0x00,
      0xA4, 0x40, 0x00, 0xD8, 0xB8, 0x05 };

// First, check to see if high performance mode has already been set
  HAL_UART_Transmit_DMA (gnss_self->gnss_uart_handle, &(high_performance_mode_query[0]),
  HIGH_PERFORMANCE_QUERY_SIZE);

  if ( tx_event_flags_get (gnss_self->irq_flags, GNSS_TX_COMPLETE, TX_OR_CLEAR, &actual_flags,
  TX_TIMER_TICKS_PER_SECOND)
       != TX_SUCCESS )
  {
    HAL_UART_DMAStop (gnss_self->gnss_uart_handle);
    gnss_self->reset_uart ();
    tx_thread_sleep (TX_TIMER_TICKS_PER_SECOND / 10);
    return uSWIFT_IO_ERROR;

  }

// Zero out the config response buffer
  memset (gnss_self->config_response_buf, 0, GNSS_CONFIG_BUFFER_SIZE);
  memset (&(payload[0]), 0, UBX_NAV_PVT_PAYLOAD_LENGTH);

// Grab the response (or lack thereof)
  HAL_UART_Receive_DMA (gnss_self->gnss_uart_handle, &(gnss_self->config_response_buf[0]),
                        sizeof(msg_buf));

  if ( tx_event_flags_get (gnss_self->irq_flags, GNSS_CONFIG_RECVD, TX_OR_CLEAR, &actual_flags,
  TX_TIMER_TICKS_PER_SECOND * 2)
       != TX_SUCCESS )
  {
    HAL_UART_DMAStop (gnss_self->gnss_uart_handle);
    gnss_self->reset_uart ();
    tx_thread_sleep (TX_TIMER_TICKS_PER_SECOND / 10);
    return uSWIFT_IO_ERROR;
  }

  for ( num_payload_bytes = uUbxProtocolDecode (buf_start, buf_length, &message_class, &message_id,
                                                payload, sizeof(payload), &buf_end);
      num_payload_bytes > 0;
      num_payload_bytes = uUbxProtocolDecode (buf_start, buf_length, &message_class, &message_id,
                                              payload, sizeof(payload), &buf_end) )
  {
    // If true, this is a NAK message, and High Performance mode is not set
    if ( message_class == 0x05 && message_id == 0x00 )
    {
      return_code = uSWIFT_NAK_RECEIVED;
      return return_code;
    }
    else if ( message_class == 0x06 && message_id == 0x8B )
    {
      // Need to ensure the response is identical to the expected response
      for ( int i = 0; i < HIGH_PERFORMANCE_RESPONSE_SIZE; i++ )
      {

        if ( payload[i] != high_performance_mode_response[i] )
        {
          return_code = uSWIFT_CONFIGURATION_ERROR;
          return return_code;
        }
      }

      return_code = uSWIFT_SUCCESS;
      return return_code;
    }
    // Adjust pointers to continue searching the buffer
    buf_length -= buf_end - buf_start;
    buf_start = buf_end;
  }

  return uSWIFT_CONFIGURATION_ERROR;
}

/**
 * Cycle power to the unit with a short delay
 *
 * @param gnss_self - GNSS struct
 *
 * @return void
 */
static void __cycle_power ( void )
{
  gnss_self->off ();
  tx_thread_sleep (10);
  gnss_self->on ();
  tx_thread_sleep (10);
}

/**
 * Zero out all struct fields.
 *
 * @param gnss_self- GNSS struct
 * @param
 */
static void __reset_struct_fields ( void )
{
  gnss_self->messages_processed = 0;
  gnss_self->v_north_sum = 0;
  gnss_self->v_east_sum = 0;
  gnss_self->v_down_sum = 0;
  gnss_self->current_latitude = 0;
  gnss_self->current_longitude = 0;
  gnss_self->sample_window_start_time = 0;
  gnss_self->sample_window_stop_time = 0;
  gnss_self->sample_window_freq = 0.0;
  gnss_self->total_samples = 0;
  gnss_self->total_samples_averaged = 0;
  gnss_self->num_bad_messages = 0;
  gnss_self->number_cycles_without_data = 0;
  gnss_self->warmup_counter = 0;
  gnss_self->current_fix_is_good = false;
  gnss_self->all_resolution_stages_complete = false;
  gnss_self->is_configured = false;
  gnss_self->is_clock_set = false;
  gnss_self->rtc_error = false;
  gnss_self->all_samples_processed = false;
  gnss_self->timer_timeout = false;
}

/**
 * @brief  Helper function to get UART Rx in DMA circular mode
 *

 * @param  buffer - buffer to store UBX messages in
 * @param  buffer_size - capacity of the bufer
 *
 * @retval GNSS_SUCCESS or
 *         GNS_UART_ERROR
 */
static uSWIFT_return_code_t __start_GNSS_UART_DMA ( uint8_t *buffer, size_t msg_size )
{
  uSWIFT_return_code_t return_code = uSWIFT_SUCCESS;
  HAL_StatusTypeDef hal_return_code = HAL_OK;
  int32_t try_counter = 0;

  while ( try_counter++ < 25 )
  {

    memset (&(buffer[0]), 0, UBX_MESSAGE_SIZE * 2);

    HAL_DMA_Abort (gnss_self->gnss_rx_dma_handle);
    HAL_DMA_Abort (gnss_self->gnss_tx_dma_handle);

    gnss_self->reset_uart ();

    hal_return_code = MX_gnss_dma_linked_list_Config ();

    if ( hal_return_code != HAL_OK )
    {
      return_code = uSWIFT_IO_ERROR;
    }

    __HAL_LINKDMA(gnss_self->gnss_uart_handle, hdmarx, *gnss_self->gnss_rx_dma_handle);

    hal_return_code = HAL_DMAEx_List_LinkQ (gnss_self->gnss_rx_dma_handle, &gnss_dma_linked_list);
    if ( hal_return_code != HAL_OK )
    {
      return_code = uSWIFT_IO_ERROR;
    }

    tx_thread_sleep (25);

    hal_return_code = HAL_UART_Receive_DMA (gnss_self->gnss_uart_handle, (uint8_t*) &(buffer[0]),
                                            msg_size);

    if ( hal_return_code != HAL_OK )
    {
      return_code = uSWIFT_IO_ERROR;
    }

    if ( return_code == uSWIFT_SUCCESS )
    {
      break;
    }

  }

  return return_code;
}
