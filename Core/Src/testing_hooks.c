/*
 * testing_hooks.c
 *
 *  Created on: Aug 1, 2024
 *      Author: philbush
 */

#include "testing_hooks.h"
#include "stddef.h"
#include "ext_rtc.h"
#include "app_threadx.h"
#include "controller.h"
#include "gnss.h"
#include "iridium.h"
#include "logger.h"
#include "ct_sensor.h"
#include "file_system_server.h"
#include "watchdog.h"
#include "math.h"
#include "NEDWaves/NEDwaves_memlight.h"
#include "NEDWaves/mem_replacements.h"

testing_hooks tests;

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*################################## Test Declarations ###########################################*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
bool test_iridium_queueing ( void *iridium_ptr );
bool test_gnss_config ( void *gnss );
bool gnss_test_control_supplement ( void *control );
bool test_ct_file_writing ( void *ct );
/**************************************************************************************************/
/*********************************** Init --> Assign Tests ****************************************/
/**************************************************************************************************/
void tests_init ( void )
{
  tests.main_test = NULL;
  tests.threadx_init_test = NULL;
  tests.control_test = NULL;
  tests.gnss_thread_test = NULL;
  tests.ct_thread_test = NULL;
  tests.light_thread_test = NULL;
  tests.turbidity_thread_test = NULL;
  tests.waves_thread_test = NULL;
  tests.iridium_thread_test = NULL;
  tests.filex_test = NULL;
  tests.shutdown_test = NULL;
}

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*################################## Test Definitions ############################################*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
bool test_iridium_queueing ( void *iridium_ptr )
{
  Iridium *iridium = (Iridium*) iridium_ptr;
  uSWIFT_return_code_t ret;
  bool current_message_sent = false;
  uint32_t next_message_type = NO_MESSAGE, next_message_size = 0;
  char *log_str;
  sbd_message_type_52 sbd_message =
    { 0 };
  sbd_message_type_54_element light_message =
    { 0 };
  uint8_t msg_buffer[342] =
    { 0 };
  uint8_t *msg_ptr;
  real16_T E[42];
  real16_T Dp;
  real16_T Hs;
  real16_T Tp;
  real16_T b_fmax;
  real16_T b_fmin;
  signed char a1[42];
  signed char a2[42];
  signed char b1[42];
  signed char b2[42];
  unsigned char check[42];

  Dp.bitPattern = TELEMETRY_FIELD_ERROR_CODE;
  Tp.bitPattern = TELEMETRY_FIELD_ERROR_CODE;
  b_fmax.bitPattern = TELEMETRY_FIELD_ERROR_CODE;
  b_fmin.bitPattern = TELEMETRY_FIELD_ERROR_CODE;

  memcpy (&sbd_message.Tp, &Tp, sizeof(real16_T));
  memcpy (&sbd_message.Dp, &Dp, sizeof(real16_T));
  memcpy (&(sbd_message.E_array[0]), &(E[0]), 42 * sizeof(real16_T));
  memcpy (&sbd_message.f_min, &b_fmin, sizeof(real16_T));
  memcpy (&sbd_message.f_max, &b_fmax, sizeof(real16_T));
  memcpy (&(sbd_message.a1_array[0]), &(a1[0]), 42 * sizeof(signed char));
  memcpy (&(sbd_message.b1_array[0]), &(b1[0]), 42 * sizeof(signed char));
  memcpy (&(sbd_message.a2_array[0]), &(a2[0]), 42 * sizeof(signed char));
  memcpy (&(sbd_message.b2_array[0]), &(b2[0]), 42 * sizeof(signed char));
  memcpy (&(sbd_message.cf_array[0]), &(check[0]), 42 * sizeof(unsigned char));

  // Make a bunch of type 52 messages, half with error values, half with an increasing
  // significant wave height
  for ( int i = 0; i < 10; i++ )
  {
    Hs.bitPattern = (i % 2 == 0) ?
        TELEMETRY_FIELD_ERROR_CODE : 0x4248 + i;
    memcpy (&sbd_message.Hs, &Hs, sizeof(real16_T));

    persistent_ram_save_message (WAVES_TELEMETRY, (uint8_t*) &sbd_message);
  }

  memcpy (&(msg_buffer[0]), &sbd_message, sizeof(sbd_message_type_52));

  for ( int i = 0; i < 10 * LIGHT_MSGS_PER_SBD; i++ )
  {
    memset (&light_message.start_lat, i + 1, sizeof(uint8_t));
    memset (&light_message.start_lon, i + 1, sizeof(uint8_t));
    memset (&light_message.end_lat, i + 1, sizeof(uint8_t));
    memset (&light_message.end_lon, i + 1, sizeof(uint8_t));
    memset (&light_message.start_timestamp, i + 1, sizeof(uint8_t));
    memset (&light_message.end_timestamp, i + 1, sizeof(uint8_t));
    memset (&light_message.max_reading_clear, i + 1, sizeof(uint8_t));
    memset (&light_message.min_reading_clear, i + 1, sizeof(uint8_t));
    memset (&light_message.avg_clear, i + 1, sizeof(uint8_t));
    memset (&light_message.avg_f1, i + 1, sizeof(uint8_t));
    memset (&light_message.avg_f2, i + 1, sizeof(uint8_t));
    memset (&light_message.avg_f3, i + 1, sizeof(uint8_t));
    memset (&light_message.avg_f4, i + 1, sizeof(uint8_t));
    memset (&light_message.avg_f5, i + 1, sizeof(uint8_t));
    memset (&light_message.avg_f6, i + 1, sizeof(uint8_t));
    memset (&light_message.avg_f7, i + 1, sizeof(uint8_t));
    memset (&light_message.avg_f8, i + 1, sizeof(uint8_t));
    memset (&light_message.avg_dark, i + 1, sizeof(uint8_t));
    memset (&light_message.avg_nir, i + 1, sizeof(uint8_t));

    persistent_ram_save_message (LIGHT_TELEMETRY, (uint8_t*) &light_message);
  }

  watchdog_check_in (IRIDIUM_THREAD);

  while ( 1 )
  {
    if ( !current_message_sent )
    {
      LOG("Attempting transmission of NEDWaves telemetry...");
      ret = iridium->transmit_message (&(msg_buffer[0]), sizeof(sbd_message_type_52));
      if ( ret == uSWIFT_SUCCESS )
      {
        current_message_sent = true;
      }
      else if ( ret == uSWIFT_TIMEOUT )
      {
        break;
      }

      continue;
    }

    next_message_type = get_next_telemetry_message (&msg_ptr, &configuration);
    if ( next_message_type == NO_MESSAGE )
    {
      LOG("No messages messages left in cache.");
      break;
    }

    next_message_size = (next_message_type == WAVES_TELEMETRY) ?
        sizeof(sbd_message_type_52) :
                        (next_message_type == TURBIDITY_TELEMETRY) ?
                            sizeof(sbd_message_type_53) :
                        (next_message_type == LIGHT_TELEMETRY) ?
                            sizeof(sbd_message_type_54) : 0;

    if ( next_message_size > 0 )
    {
      log_str = (next_message_type == WAVES_TELEMETRY) ?
          "NEDWaves" :
                (next_message_type == TURBIDITY_TELEMETRY) ?
                    "OBS" :
                (next_message_type == LIGHT_TELEMETRY) ?
                    "Light" : "Unknown";
      LOG("Attempting transmission of %s telemetry...", log_str);

      if ( iridium->transmit_message (msg_ptr, next_message_size) == uSWIFT_SUCCESS )
      {
        persistent_ram_delete_message_element (next_message_type, msg_ptr);
      }
    }
  }

  return true;
}

uint32_t num_gnss_configs_passed = 0;
uint32_t num_gnss_configs_failed = 0;
bool test_gnss_config ( void *gnss )
{
  GNSS *gnss_ptr = (GNSS*) gnss;

  for ( int i = 0; i < 100; i++ )
  {
    if ( !gnss_apply_config (gnss_ptr) )
    {
      num_gnss_configs_failed++;
    }
    else
    {
      num_gnss_configs_passed++;
    }
  }
  return true;
}

bool gnss_test_control_supplement ( void *control )
{
  Control *controller_self = (Control*) control;
  tx_thread_sleep (TX_TIMER_TICKS_PER_SECOND * 10);

  (void) tx_thread_resume (controller_self->thread_handles->gnss_thread);
  (void) tx_thread_resume (controller_self->thread_handles->waves_thread);
//  (void) tx_thread_resume (controller_self->thread_handles->iridium_thread);
  while ( 1 )
  {
    tx_thread_sleep (TX_TIMER_TICKS_PER_SECOND * 1);
  }

}

bool test_ct_file_writing ( void *ct )
{
  CT *dut = (CT*) ct;

  for ( int i = 0; i < TOTAL_CT_SAMPLES; i++, dut->total_samples++ )
  {
    dut->samples[i].temp = i * 2.0f;
    dut->samples[i].salinity = i * 5.0f;
  }

  tx_thread_sleep (TX_TIMER_TICKS_PER_SECOND * 5);

  return (file_system_server_save_ct_raw (dut) == uSWIFT_SUCCESS);
}

