/*
 * watchdog.c
 *
 *  Created on: Jun 14, 2024
 *      Author: philbush
 */

#include <ext_rtc_server.h>
#include "watchdog.h"
#include "configuration.h"
#include "threadx_support.h"
#include "logger.h"

static watchdog_handle hidden_handle;

int32_t watchdog_init ( watchdog_handle wd_handle, TX_EVENT_FLAGS_GROUP *watchdog_check_in_flags )
{
  int32_t ret = WATCHDOG_OK;
  // Needed for the callback function
  hidden_handle = wd_handle;

  wd_handle->check_in_flags = watchdog_check_in_flags;

  // NOTE(lindzey): I don't think that the watchdog needs to track thesse
  //     individually, though it is useful to have an enum mapping to which bit is set.
  //     Instead, just have a corresponding uint32 of required_threads,
  //     and use the thread enum to set bits in it, like done for watchdog_check_in.
  //     That would simpify adding a new thread, such that the only thing to do is:
  //       * add an enum to watchdog_thread_flags, call check_in/register/deregister with that value
  //     while currently you also need to:
  //       * add a flag to the watchdog_t struct
  //       * update code to handle that flag in:
  //         - watchdog_event_callback
  //         - watchdog_register_thread
  //         - watchdog_deregister_thread
  wd_handle->gnss_active = false;
  wd_handle->ct_active = false;
  wd_handle->temperature_active = false;
  wd_handle->light_active = false;
  wd_handle->turbidity_active = false;
  wd_handle->waves_active = false;
  wd_handle->iridium_active = false;
  wd_handle->filex_active = false;
  wd_handle->accelerometer_active = false;

  if ( tx_event_flags_set_notify (wd_handle->check_in_flags, watchdog_event_callback) != TX_SUCCESS )
  {
    ret = WATCHDOG_ERROR;
  }

  return ret;
}

/**
 * @brief  Callback which checks the event flags set and refreshes the watchdog accordingly
 * @param  flags_group_ptr := pointer to the watchdog flags group
 * @retval None
 */
void watchdog_event_callback ( TX_EVENT_FLAGS_GROUP *flags_group_ptr )
{
  UINT retval;
  ULONG current_flags, required_flags = 0;

  if ( hidden_handle->gnss_active )
  {
    required_flags |= GNSS_THREAD;
  }

  if ( hidden_handle->ct_active )
  {
    required_flags |= CT_THREAD;
  }

  if ( hidden_handle->temperature_active )
  {
    required_flags |= TEMPERATURE_THREAD;
  }

  if ( hidden_handle->light_active )
  {
    required_flags |= LIGHT_THREAD;
  }

  if ( hidden_handle->turbidity_active )
  {
    required_flags |= TURBIDITY_THREAD;
  }

  if ( hidden_handle->waves_active )
  {
    required_flags |= WAVES_THREAD;
  }

  if ( hidden_handle->iridium_active )
  {
    required_flags |= IRIDIUM_THREAD;
  }

  if ( hidden_handle->filex_active )
  {
    required_flags |= FILEX_THREAD;
  }

  if ( hidden_handle->accelerometer_active )
  {
    required_flags |= ACCELEROMETER_THREAD;
  }

  // Control thread always has to check in
  required_flags |= CONTROL_THREAD;

  retval = tx_event_flags_get (flags_group_ptr, required_flags, TX_AND, &current_flags, TX_NO_WAIT);

  // All of the threads have not checked in
  if ( retval != TX_SUCCESS )
  {
    return;
  }

  // All threads have checked in. Refresh the watchdog and clear the flags
  rtc_server_refresh_watchdog ();

  // Clear the event flags
  (void) clear_event_flags (flags_group_ptr);
}

/**
 * @brief  Register an event flag to indicate the thread has checked in
 * @param  thread := which thread is checking in
 * @retval Void
 */
void watchdog_check_in ( enum watchdog_thread_flags which_thread )
{
  (void) tx_event_flags_set (hidden_handle->check_in_flags, which_thread, TX_OR);
}

/**
 * @brief  Register a thread error so the watchdog can be refreshed without this thread checking in.
 * @param  thread := which thread is reporting an error
 * @retval Void
 */
void watchdog_register_thread ( enum watchdog_thread_flags which_thread )
{
  switch ( which_thread )
  {
    case GNSS_THREAD:
      hidden_handle->gnss_active = true;
      break;

    case CT_THREAD:
      hidden_handle->ct_active = true;
      break;

    case TEMPERATURE_THREAD:
      hidden_handle->temperature_active = true;
      break;

    case LIGHT_THREAD:
      hidden_handle->light_active = true;
      break;

    case TURBIDITY_THREAD:
      hidden_handle->turbidity_active = true;
      break;

    case WAVES_THREAD:
      hidden_handle->waves_active = true;
      break;

    case IRIDIUM_THREAD:
      hidden_handle->iridium_active = true;
      break;

    case FILEX_THREAD:
      hidden_handle->filex_active = true;
      break;

    case ACCELEROMETER_THREAD:
      hidden_handle->accelerometer_active = true;
      break;

    default:
      // This log message never prints; instead it causes a HardFault
      // https://github.com/SASlabgroup/microSWIFT_V2.2_Firmware/issues/1
      LOG("Invalid arg supplied to watchdog_register_thread().");
      break;
  }
}

void watchdog_deregister_thread ( enum watchdog_thread_flags which_thread )
{
  switch ( which_thread )
  {
    case GNSS_THREAD:
      hidden_handle->gnss_active = false;
      break;

    case CT_THREAD:
      hidden_handle->ct_active = false;
      break;

    case TEMPERATURE_THREAD:
      hidden_handle->temperature_active = false;
      break;

    case LIGHT_THREAD:
      hidden_handle->light_active = false;
      break;

    case TURBIDITY_THREAD:
      hidden_handle->turbidity_active = false;
      break;

    case WAVES_THREAD:
      hidden_handle->waves_active = false;
      break;

    case IRIDIUM_THREAD:
      hidden_handle->iridium_active = false;
      break;

    case FILEX_THREAD:
      hidden_handle->filex_active = false;
      break;

    case ACCELEROMETER_THREAD:
      hidden_handle->accelerometer_active = false;
      break;

    default:
      LOG("Invalid arg supplied to watchdog_deregister_thread().");
      break;
  }
}
