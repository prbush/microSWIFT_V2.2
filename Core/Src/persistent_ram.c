/*
 * persistent_ram.c
 *
 *  Created on: Sep 12, 2024
 *      Author: philbush
 */

#include "persistent_ram.h"
#include "float.h"
#include "logger.h"
#include "math.h"
#include "string.h"
#include "time.h"
#include <ext_rtc_server.h>
#include <limits.h>

// Save the struct in SRAM2 -- NOLOAD section which will be retained in standby
// mode
static Persistent_Storage persistent_self __attribute__((section(".sram2")));

// Helper functions
static void _persistent_ram_clear(void);

/**
 * Initialize the persistent memory (SRAM 2). This memory section can be
 * maintained during Standby mode.
 *
 * @return void
 */
void persistent_ram_init(const microSWIFT_configuration *config,
                         const microSWIFT_firmware_version_t *version) {
  uint32_t reset_reason = HAL_RCC_GetResetSource();

  // If this has not been initialized previously
  if (persistent_self.magic_number != PERSISTENT_RAM_MAGIC_DOUBLE_WORD) {
    _persistent_ram_clear();
    persistent_ram_set_device_config(config, false);
    persistent_ram_set_firmware_version(version);

    persistent_self.magic_number = PERSISTENT_RAM_MAGIC_DOUBLE_WORD;
  }
  // If a watchdog reset occurred (identified as a hardware pin reset), clear
  // everything out
  else if (reset_reason == RCC_RESET_FLAG_PIN) {
    _persistent_ram_clear();
    persistent_ram_set_device_config(config, false);
    persistent_ram_set_firmware_version(version);

    persistent_self.magic_number = PERSISTENT_RAM_MAGIC_DOUBLE_WORD;
  }

  persistent_self.reset_reason = reset_reason;

  // If the RTC has not been set, then reset the sample window counter
  if (!persistent_ram_get_rtc_time_set()) {
    persistent_self.sample_window_counter = 1;
  }
  // Otherwise increment it
  else {
    persistent_self.sample_window_counter++;
  }

  // Initialize the priority queues.
  // * Previously the "valid" flag had been set to False by the clearing
  //   function setting all memory to 0.
  // * Light and Turbidity have a plain queue, no prioritization.
  for (int ii = 0; ii < MAX_NUM_ACCELEROMETER_MSGS_STORED; ii++) {
    persistent_self.accel_storage.msg_queue[ii].priority = -1;
  }
  for (int ii = 0; ii < MAX_NUM_WAVES_MSGS_STORED; ii++) {
    persistent_self.waves_storage.msg_queue[ii].priority = -1;
  }
}

/**
 * Deinitialize the persistent memory (SRAM 2), clearing all contents.
 *
 * @return void
 */
void persistent_ram_deinit(void) {
  // Clear everything out
  _persistent_ram_clear();
}

/**
 * Copy over the configuration from supplied pointer into the persistent ram.
 *
 * @return void
 */
void persistent_ram_set_device_config(const microSWIFT_configuration *config,
                                      bool ota_update) {
  memcpy(&persistent_self.device_config, config,
         sizeof(microSWIFT_configuration));

  persistent_self.ota_update_received = ota_update;
}

/**
 * Copy the current config to the supplied struct pointer.
 *
 * @return void
 */
void persistent_ram_get_device_config(microSWIFT_configuration *config) {
  memcpy(config, &persistent_self.device_config,
         sizeof(microSWIFT_configuration));
}

/**
 * Copy over the firmware version from supplied pointer into the persistent ram.
 *
 * @return void
 */
void persistent_ram_set_firmware_version(
    const microSWIFT_firmware_version_t *version) {
  memcpy(&persistent_self.version, version,
         sizeof(microSWIFT_firmware_version_t));
}

/**
 * Copy the current firmware version to the supplied pointer.
 *
 * @return void
 */
void persistent_ram_get_firmware_version(
    microSWIFT_firmware_version_t *version) {
  memcpy(version, &persistent_self.version,
         sizeof(microSWIFT_firmware_version_t));
}

/**
 * Copy the current firmware version to the supplied pointer.
 *
 * @return void
 */
bool persistent_ram_get_ota_update_status(void) {
  return persistent_self.ota_update_received;
}

/**
 * Get the status of if the ota ack message has been sent.
 *
 * @return void
 */
bool persistent_ram_get_ota_ack_status(void) {
  return persistent_self.ota_acknowledgement_sent;
}

/**
 * Copy the OTA ack message to persistent ram if Tx was unsuccessful.
 *
 * @return void
 */
void persistent_ram_set_ota_ack_msg(sbd_message_type_99 *msg) {
  memcpy(&persistent_self.ota_acknowledgement_msg, msg,
         sizeof(sbd_message_type_99));
}

/**
 * Copy the OTA ack message to the provided return pointer if Tx was
 * unsuccessful.
 *
 * @return void
 */
void persistent_ram_get_ota_ack_msg(sbd_message_type_99 *msg) {
  memcpy(msg, &persistent_self.ota_acknowledgement_msg,
         sizeof(sbd_message_type_99));
}

/**
 * Return the duty cycle count (counting up from power on).
 *
 * @return sample_window_counter
 */
uint32_t persistent_ram_get_sample_window_counter(void) {
  return persistent_self.sample_window_counter;
}

/**
 * Reset the sample window counter to 0. It will be incremented at next boot.
 *
 * @return void
 */
void persistent_ram_reset_sample_window_counter(void) {
  persistent_self.sample_window_counter = 0;
}

/**
 * Set the rtc_time_set flag to true.
 *
 * @return void
 */
void persistent_ram_set_rtc_time_set(void) {
  persistent_self.rtc_time_set = true;
}

/**
 * Get the rtc_time_set flag.
 *
 * @return rtc_time_set flag
 */
bool persistent_ram_get_rtc_time_set(void) {
  return persistent_self.rtc_time_set;
}

/**
 * Get the reset reason code, defined in @defgroup RCC_Reset_Flag Reset Flag
 * within stm32u5xx_hal_rcc.h.
 *
 * @return rtc_time_set flag
 */
uint32_t persistent_ram_get_reset_reason(void) {
  return persistent_self.reset_reason;
}

/**
 * Return the number of enqueued SBD messages for a given message type.
 *
 * @return void
 */
uint32_t persistent_ram_get_num_msgs_enqueued(telemetry_type_t msg_type) {
  switch (msg_type) {
  case WAVES_TELEMETRY:
    LOG("Number of enqueued Waves messages: %lu",
        persistent_self.waves_storage.num_telemetry_msgs_enqueued);
    return persistent_self.waves_storage.num_telemetry_msgs_enqueued;
    break;

  case TURBIDITY_TELEMETRY:
    LOG("Number of enqueued OBS message elements: %lu",
        persistent_self.turbidity_storage.num_msg_elements_enqueued);
    return persistent_self.turbidity_storage.num_msg_elements_enqueued /
           TURBIDITY_MSGS_PER_SBD;
    break;

  case LIGHT_TELEMETRY:
    LOG("Number of enqueued Light message elements: %lu",
        persistent_self.light_storage.num_msg_elements_enqueued);
    return persistent_self.light_storage.num_msg_elements_enqueued /
           LIGHT_MSGS_PER_SBD;
    break;

  case ACCELEROMETER_TELEMETRY:
    LOG("Number of enqueued Accelerometer messages: %lu",
        persistent_self.accel_storage.num_telemetry_msgs_enqueued);
    return persistent_self.accel_storage.num_telemetry_msgs_enqueued;
    break;

  default:
    return 0;
  }
}

/**
 * Save an SBD message for later transmission.
 * !! Turbidity and Light messages are saved as single elements, not a full SBD
 * message.
 *
 * @return void
 */

void persistent_ram_save_message(telemetry_type_t msg_type, float msg_priority,
                                 uint8_t *msg) {
  // Corruption, lack of initialization check
  if (persistent_self.magic_number != PERSISTENT_RAM_MAGIC_DOUBLE_WORD) {
    _persistent_ram_clear();
  }

  // Minimum priority in the queue
  float min_priority = FLT_MAX;
  // Index of minimum priority; where data will be inserted
  int min_priority_idx = -1;

  switch (msg_type) {

  case WAVES_TELEMETRY:
    for (int ii = 0; ii < MAX_NUM_WAVES_MSGS_STORED; ii++) {
      if (persistent_self.waves_storage.msg_queue[ii].priority < min_priority) {
        min_priority = persistent_self.waves_storage.msg_queue[ii].priority;
        min_priority_idx = ii;
        if (min_priority < 0) {
          // short-circuit search if we've found an empty slot
          break;
        }
      }
    }
    if (min_priority_idx >= 0 && msg_priority > min_priority) {
      memcpy(
          &(persistent_self.waves_storage.msg_queue[min_priority_idx].payload),
          msg, sizeof(sbd_message_type_52));
      persistent_self.waves_storage.msg_queue[min_priority_idx].priority =
          msg_priority;

      // If NOT replacing a valid message, increment num enqueued
      if (min_priority < 0) {
        persistent_self.waves_storage.num_telemetry_msgs_enqueued++;
      }
    }
    break; // case WAVES_TELEMETRY

  case ACCELEROMETER_TELEMETRY:

    // Find minimum value and index in queue
    for (int ii = 0; ii < MAX_NUM_ACCELEROMETER_MSGS_STORED; ii++) {
      if (persistent_self.accel_storage.msg_queue[ii].priority < min_priority) {
        min_priority = persistent_self.accel_storage.msg_queue[ii].priority;
        min_priority_idx = ii;
        if (min_priority < 0) {
          // short-circuit search if we've found an empty slot
          break;
        }
      }
    }

    if (min_priority_idx >= 0 && msg_priority > min_priority) {
      // copy the message over
      memcpy(
          &(persistent_self.accel_storage.msg_queue[min_priority_idx].payload),
          msg, sizeof(sbd_message_type_55));
      persistent_self.accel_storage.msg_queue[min_priority_idx].priority =
          msg_priority;
      LOG("Queued msg with priority %0.6f and position %d", msg_priority,
          min_priority_idx);
      // need to increment message count IF we're not replacing an existing one
      if (min_priority < 0) {
        persistent_self.accel_storage.num_telemetry_msgs_enqueued++;
      }
    } else {
      LOG("Cannot enqueue accel msg with priority %0.6f; min priority in queue "
          "= %0.6f",
          msg_priority, min_priority);
    }
    break; // case ACCELEROMETER_TELEMETRY

  case TURBIDITY_TELEMETRY:
    // If the storage queue is full, then just skip
    if ((persistent_self.turbidity_storage.num_msg_elements_enqueued ==
         MAX_NUM_TURBIDITY_MSGS_STORED)) {
      return;
    }
    // First check if there is room in the current message index
    for (int ii = 0; ii < TURBIDITY_MSGS_PER_SBD; ii++) {
      if (!persistent_self.turbidity_storage
               .msg_queue[persistent_self.turbidity_storage.current_msg_index]
               .valid[ii]) {
        // copy the message over
        memcpy(&(persistent_self.turbidity_storage
                     .msg_queue[persistent_self.turbidity_storage
                                    .current_msg_index]
                     .payload.elements[ii]),
               msg, sizeof(sbd_message_type_53_element));
        // Make the entry valid
        persistent_self.turbidity_storage
            .msg_queue[persistent_self.turbidity_storage.current_msg_index]
            .valid[ii] = true;
        persistent_self.turbidity_storage.num_msg_elements_enqueued++;
        return;
      }
    }

    // Need to start a new message
    for (int ii = 0; ii < MAX_NUM_NON_WAVES_MSGS_STORED; ii++) {
      if (!persistent_self.turbidity_storage.msg_queue[ii].valid[0]) {
        // copy the message over
        memcpy(&(persistent_self.turbidity_storage.msg_queue[ii]
                     .payload.elements[0]),
               msg, sizeof(sbd_message_type_53_element));
        // Make the entry valid
        persistent_self.turbidity_storage.msg_queue[ii].valid[0] = true;
        persistent_self.turbidity_storage.num_msg_elements_enqueued++;
        persistent_self.turbidity_storage.current_msg_index = ii;
        return;
      }
    }

    break; // case TURBIDITY_TELEMETRY

  case LIGHT_TELEMETRY:
    // If the storage queue is full, then just skip
    if ((persistent_self.light_storage.num_msg_elements_enqueued ==
         MAX_NUM_LIGHT_MSGS_STORED)) {
      return;
    }
    // First check if there is room in the current message index
    for (int ii = 0; ii < LIGHT_MSGS_PER_SBD; ii++) {
      if (!persistent_self.light_storage
               .msg_queue[persistent_self.light_storage.current_msg_index]
               .valid[ii]) {
        // copy the message over
        memcpy(&(persistent_self.light_storage
                     .msg_queue[persistent_self.light_storage.current_msg_index]
                     .payload.elements[ii]),
               msg, sizeof(sbd_message_type_54_element));
        // Make the entry valid
        persistent_self.light_storage
            .msg_queue[persistent_self.light_storage.current_msg_index]
            .valid[ii] = true;
        persistent_self.light_storage.num_msg_elements_enqueued++;
        return;
      }
    }

    // Need to start a new message
    for (int ii = 0; ii < MAX_NUM_NON_WAVES_MSGS_STORED; ii++) {
      if (!persistent_self.light_storage.msg_queue[ii].valid[0]) {
        // copy the message over
        memcpy(
            &(persistent_self.light_storage.msg_queue[ii].payload.elements[0]),
            msg, sizeof(sbd_message_type_54_element));
        // Make the entry valid
        persistent_self.light_storage.msg_queue[ii].valid[0] = true;
        persistent_self.light_storage.num_msg_elements_enqueued++;
        persistent_self.light_storage.current_msg_index = ii;
        return;
      }
    }

    break; // case LIGHT_TELEMETRY

  default:
    return;
  }
}

/**
 * Return a pointer to the highest priority message of a given type. If no
 * message is available, return pointer will be NULL.
 * !!@ This always returns a pointer to a FULL SBD MESSAGE, not just a single
 * message element.
 *
 * @return void
 */
uint8_t *
persistent_ram_get_prioritized_unsent_message(telemetry_type_t msg_type) {
  float max_priority = -FLT_MAX;
  int max_priority_idx = -1;

  bool msg_full = false;
  uint8_t *ret_ptr = NULL;

  // Corruption, lack of initialization check
  if (persistent_self.magic_number != PERSISTENT_RAM_MAGIC_DOUBLE_WORD) {
    _persistent_ram_clear();
    return NULL;
  }

  switch (msg_type) {
  case WAVES_TELEMETRY:

    // Empty queue check
    if (persistent_self.waves_storage.num_telemetry_msgs_enqueued == 0) {
      return NULL;
    }

    // Find the highest priority message
    for (int ii = 0; ii < MAX_NUM_WAVES_MSGS_STORED; ii++) {
      if (persistent_self.waves_storage.msg_queue[ii].priority > max_priority) {
        max_priority = persistent_self.waves_storage.msg_queue[ii].priority;
        max_priority_idx = ii;
      }
    }

    if (0 <= max_priority_idx && max_priority_idx < MAX_NUM_WAVES_MSGS_STORED) {
      ret_ptr =
          (uint8_t *)&persistent_self.waves_storage.msg_queue[max_priority_idx]
              .payload;
    } else {
      ret_ptr = NULL;
    }
    break; // case WAVES_TELEMETRY

  case ACCELEROMETER_TELEMETRY:

    // Empty queue check
    if (persistent_self.accel_storage.num_telemetry_msgs_enqueued == 0) {
      return NULL;
    }

    for (int ii = 0; ii < MAX_NUM_ACCELEROMETER_MSGS_STORED; ii++) {
      if (persistent_self.accel_storage.msg_queue[ii].priority > max_priority) {
        max_priority = persistent_self.accel_storage.msg_queue[ii].priority;
        max_priority_idx = ii;
      }
    }

    if (max_priority_idx >= 0 &&
        max_priority_idx < MAX_NUM_ACCELEROMETER_MSGS_STORED) {
      ret_ptr =
          (uint8_t *)&persistent_self.accel_storage.msg_queue[max_priority_idx]
              .payload;
    } else {
      ret_ptr = NULL;
    }

    break; // case ACCELEROMETER_TELEMETRY

  case TURBIDITY_TELEMETRY:
    // Make sure we have enough elements to constitute a full msg
    if (persistent_self.turbidity_storage.num_msg_elements_enqueued <
        TURBIDITY_MSGS_PER_SBD) {
      return NULL;
    }

    // Just take the first available
    // NOTE(LEL): I think this effectively becomes a LIFO queue
    for (int ii = 0; ii < MAX_NUM_NON_WAVES_MSGS_STORED; ii++) {
      msg_full = true;
      for (int jj = 0; jj < TURBIDITY_MSGS_PER_SBD; jj++) {
        if (!persistent_self.turbidity_storage.msg_queue[ii].valid[jj]) {
          msg_full = false;
          break;
        }
      }
      if (msg_full) {
        ret_ptr =
            (uint8_t *)&persistent_self.turbidity_storage.msg_queue[ii].payload;
        return ret_ptr;
      }
    }
    break; // case TURBIDITY_TELEMETRY

  case LIGHT_TELEMETRY:
    // Make sure we have enough elements to constitute a full msg
    if (persistent_self.light_storage.num_msg_elements_enqueued <
        LIGHT_MSGS_PER_SBD) {
      return NULL;
    }
    // Just take the first available
    for (int ii = 0; ii < MAX_NUM_NON_WAVES_MSGS_STORED; ii++) {
      msg_full = true;
      for (int jj = 0; jj < LIGHT_MSGS_PER_SBD; jj++) {
        if (!persistent_self.light_storage.msg_queue[ii].valid[jj]) {
          msg_full = false;
          break;
        }
      }

      if (msg_full) {
        ret_ptr =
            (uint8_t *)&persistent_self.light_storage.msg_queue[ii].payload;
        return ret_ptr;
      }
    }

    break; // case LIGHT_TELEMETRY

  default:
    ret_ptr = NULL;
  }

  return ret_ptr;
}

// We don't pop from the queue because we only want to remove an element
// after it has been transmitted successfully. So we have a {get, delete}
// in place of the more traditional {peek, pop}.
void persistent_ram_delete_message_element(telemetry_type_t msg_type,
                                           uint8_t *msg_ptr) {
  // Corruption, lack of initialization check
  if (persistent_self.magic_number != PERSISTENT_RAM_MAGIC_DOUBLE_WORD) {
    _persistent_ram_clear();
    return;
  }

  switch (msg_type) {
  case WAVES_TELEMETRY:

    // Find the pointer
    for (int ii = 0; ii < MAX_NUM_WAVES_MSGS_STORED; ii++) {
      if ((uint8_t *)&persistent_self.waves_storage.msg_queue[ii].payload ==
          msg_ptr) {
        // Zero out the message
        memset(msg_ptr, 0, sizeof(Iridium_Message_Storage_Element_t));
        ((Iridium_Message_Storage_Element_t *)msg_ptr)->priority = -1;
        persistent_self.waves_storage.num_telemetry_msgs_enqueued--;
        break;
      }
    }
    break; // case WAVES_TELEMETRY

  case ACCELEROMETER_TELEMETRY:

    // Find the pointer
    for (int i = 0; i < MAX_NUM_ACCELEROMETER_MSGS_STORED; i++) {
      if ((uint8_t *)&persistent_self.accel_storage.msg_queue[i].payload ==
          msg_ptr) {
        // Zero out the message
        memset(msg_ptr, 0, sizeof(Accelerometer_Message_Storage_Element_t));
        ((Accelerometer_Message_Storage_Element_t *)msg_ptr)->priority = -1;
        persistent_self.accel_storage.num_telemetry_msgs_enqueued--;
        break;
      }
    }
    break; // case ACCELEROMETER_TELEMETRY

  case TURBIDITY_TELEMETRY:

    // Find the pointer
    for (int i = 0; i < MAX_NUM_NON_WAVES_MSGS_STORED; i++) {
      if ((uint8_t *)&persistent_self.turbidity_storage.msg_queue[i].payload ==
          msg_ptr) {
        // Zero out the message
        memset(msg_ptr, 0, sizeof(Turbidity_Message_Storage_Element_t));
        persistent_self.turbidity_storage.num_msg_elements_enqueued -=
            TURBIDITY_MSGS_PER_SBD;
        break;
      }
    }
    break; // case TURBIDITY_TELEMETRY

  case LIGHT_TELEMETRY:

    // Find the pointer
    for (int i = 0; i < MAX_NUM_NON_WAVES_MSGS_STORED; i++) {
      if ((uint8_t *)&persistent_self.light_storage.msg_queue[i].payload ==
          msg_ptr) {
        // Zero out the message
        memset(msg_ptr, 0, sizeof(Light_Message_Storage_Element_t));
        persistent_self.light_storage.num_msg_elements_enqueued -=
            LIGHT_MSGS_PER_SBD;
        break;
      }
    }
    break; // case LIGHT_TELEMETRY

  case OTA_ACK_MESSAGE:
    persistent_self.ota_acknowledgement_sent = true;
    memset(&persistent_self.ota_acknowledgement_msg, 0,
           sizeof(sbd_message_type_99));
    break;

  default:
    break;
  }
}

VOID persistent_ram_get_accel_priorities(float *priorities) {
  for (int ii = 0; ii < MAX_NUM_ACCELEROMETER_MSGS_STORED; ii++) {
    priorities[ii] = persistent_self.accel_storage.msg_queue[ii].priority;
  }
}

static void _persistent_ram_clear(void) {
  // Clear everything out
  memset(&persistent_self, 0, sizeof(Persistent_Storage));
}
