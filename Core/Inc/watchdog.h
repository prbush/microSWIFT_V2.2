/*
 * watchdog.h
 *
 *  Created on: Jun 14, 2024
 *      Author: philbush
 */

#ifndef INC_WATCHDOG_H_
#define INC_WATCHDOG_H_

#include "stdbool.h"
#include "tx_api.h"
#include "error_handler.h"
#include "stdint.h"
// @formatter:off

#define WATCHDOG_OK     0
#define WATCHDOG_ERROR  1

enum watchdog_thread_flags
{
  CONTROL_THREAD       = ((ULONG) 1 << 0),
  GNSS_THREAD          = ((ULONG) 1 << 1),
  CT_THREAD            = ((ULONG) 1 << 2),
  TEMPERATURE_THREAD   = ((ULONG) 1 << 3),
  LIGHT_THREAD         = ((ULONG) 1 << 4),
  TURBIDITY_THREAD     = ((ULONG) 1 << 5),
  WAVES_THREAD         = ((ULONG) 1 << 6),
  IRIDIUM_THREAD       = ((ULONG) 1 << 7),
  FILEX_THREAD         = ((ULONG) 1 << 8),
  ACCELEROMETER_THREAD = ((ULONG) 1 << 9)
};

struct watchdog_t
{
  TX_EVENT_FLAGS_GROUP  *check_in_flags;

  bool                  gnss_active;
  bool                  ct_active;
  bool                  temperature_active;
  bool                  light_active;
  bool                  turbidity_active;
  bool                  waves_active;
  bool                  iridium_active;
  bool                  filex_active;
  bool                  accelerometer_active;
};

typedef struct watchdog_t *watchdog_handle;

int32_t watchdog_init ( watchdog_handle handle, TX_EVENT_FLAGS_GROUP *watchdog_check_in_flags );
void    watchdog_event_callback ( TX_EVENT_FLAGS_GROUP *flags_group_ptr );
void    watchdog_check_in ( enum watchdog_thread_flags which_thread );
void    watchdog_register_thread ( enum watchdog_thread_flags which_thread );
void    watchdog_deregister_thread (  enum watchdog_thread_flags which_thread );
// @formatter:on
#endif /* INC_WATCHDOG_H_ */
