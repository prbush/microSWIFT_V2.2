/*
 * persistent_ram.c
 *
 *  Created on: Sep 12, 2024
 *      Author: philbush
 */

#include <ext_rtc_server.h>
#include "persistent_ram.h"
#include "float.h"
#include "logger.h"
#include "math.h"
#include "string.h"
#include "time.h"

// Save the struct in SRAM2 -- NOLOAD section which will be retained in standby mode
static Persistent_Storage persistent_self __attribute__((section(".sram2")));

// Helper functions
static void _persistent_ram_clear ( void );

/**
 * Initialize the persistent memory (SRAM 2). This memory section can be maintained during Standby mode.
 *
 * @return void
 */
void persistent_ram_init ( const microSWIFT_configuration *config,
                           const microSWIFT_firmware_version_t *version )
{
  uint32_t reset_reason = HAL_RCC_GetResetSource ();

  // If this has not been initialized previously
  if ( persistent_self.magic_number != PERSISTENT_RAM_MAGIC_DOUBLE_WORD )
  {
    _persistent_ram_clear ();
    persistent_ram_set_device_config (config, false);
    persistent_ram_set_firmware_version (version);

    persistent_self.magic_number = PERSISTENT_RAM_MAGIC_DOUBLE_WORD;
  }
  // If a watchdog reset occurred (identified as a hardware pin reset), clear everything out
  else if ( reset_reason == RCC_RESET_FLAG_PIN )
  {
    _persistent_ram_clear ();
    persistent_ram_set_device_config (config, false);
    persistent_ram_set_firmware_version (version);

    persistent_self.magic_number = PERSISTENT_RAM_MAGIC_DOUBLE_WORD;
  }

  persistent_self.reset_reason = reset_reason;

  // If the RTC has not been set, then reset the sample window counter
  if ( !persistent_ram_get_rtc_time_set () )
  {
    persistent_self.sample_window_counter = 1;
  }
  // Otherwise increment it
  else
  {
    persistent_self.sample_window_counter++;
  }
}

/**
 * Deinitialize the persistent memory (SRAM 2), clearing all contents.
 *
 * @return void
 */
void persistent_ram_deinit ( void )
{
  // Clear everything out
  _persistent_ram_clear ();
}

/**
 * Copy over the configuration from supplied pointer into the persistent ram.
 *
 * @return void
 */
void persistent_ram_set_device_config ( const microSWIFT_configuration *config, bool ota_update )
{
  memcpy (&persistent_self.device_config, config, sizeof(microSWIFT_configuration));

  persistent_self.ota_update_received = ota_update;
}

/**
 * Copy the current config to the supplied struct pointer.
 *
 * @return void
 */
void persistent_ram_get_device_config ( microSWIFT_configuration *config )
{
  memcpy (config, &persistent_self.device_config, sizeof(microSWIFT_configuration));
}

/**
 * Copy over the firmware version from supplied pointer into the persistent ram.
 *
 * @return void
 */
void persistent_ram_set_firmware_version ( const microSWIFT_firmware_version_t *version )
{
  memcpy (&persistent_self.version, version, sizeof(microSWIFT_firmware_version_t));
}

/**
 * Copy the current firmware version to the supplied pointer.
 *
 * @return void
 */
void persistent_ram_get_firmware_version ( microSWIFT_firmware_version_t *version )
{
  memcpy (version, &persistent_self.version, sizeof(microSWIFT_firmware_version_t));
}

/**
 * Copy the current firmware version to the supplied pointer.
 *
 * @return void
 */
bool persistent_ram_get_ota_update_status ( void )
{
  return persistent_self.ota_update_received;
}

/**
 * Get the status of if the ota ack message has been sent.
 *
 * @return void
 */
bool persistent_ram_get_ota_ack_status ( void )
{
  return persistent_self.ota_acknowledgement_sent;
}

/**
 * Copy the OTA ack message to persistent ram if Tx was unsuccessful.
 *
 * @return void
 */
void persistent_ram_set_ota_ack_msg ( sbd_message_type_99 *msg )
{
  memcpy (&persistent_self.ota_acknowledgement_msg, msg, sizeof(sbd_message_type_99));
}

/**
 * Copy the OTA ack message to the provided return pointer if Tx was unsuccessful.
 *
 * @return void
 */
void persistent_ram_get_ota_ack_msg ( sbd_message_type_99 *msg )
{
  memcpy (msg, &persistent_self.ota_acknowledgement_msg, sizeof(sbd_message_type_99));
}

/**
 * Return the duty cycle count (counting up from power on).
 *
 * @return sample_window_counter
 */
uint32_t persistent_ram_get_sample_window_counter ( void )
{
  return persistent_self.sample_window_counter;
}

/**
 * Reset the sample window counter to 0. It will be incremented at next boot.
 *
 * @return void
 */
void persistent_ram_reset_sample_window_counter ( void )
{
  persistent_self.sample_window_counter = 0;
}

/**
 * Set the rtc_time_set flag to true.
 *
 * @return void
 */
void persistent_ram_set_rtc_time_set ( void )
{
  persistent_self.rtc_time_set = true;
}

/**
 * Get the rtc_time_set flag.
 *
 * @return rtc_time_set flag
 */
bool persistent_ram_get_rtc_time_set ( void )
{
  return persistent_self.rtc_time_set;
}

/**
 * Get the reset reason code, defined in @defgroup RCC_Reset_Flag Reset Flag within stm32u5xx_hal_rcc.h.
 *
 * @return rtc_time_set flag
 */
uint32_t persistent_ram_get_reset_reason ( void )
{
  return persistent_self.reset_reason;
}

/**
 * Return the number of enqueued SBD messages for a given message type.
 *
 * @return void
 */
uint32_t persistent_ram_get_num_msgs_enqueued ( telemetry_type_t msg_type )
{
  switch ( msg_type )
  {
    case WAVES_TELEMETRY:
      LOG("Number of enqueued Waves messages: %lu",
          persistent_self.waves_storage.num_telemetry_msgs_enqueued);
      return persistent_self.waves_storage.num_telemetry_msgs_enqueued;
      break;

    case TURBIDITY_TELEMETRY:
      LOG("Number of enqueued OBS message elements: %lu",
          persistent_self.turbidity_storage.num_msg_elements_enqueued);
      return persistent_self.turbidity_storage.num_msg_elements_enqueued / TURBIDITY_MSGS_PER_SBD;
      break;

    case LIGHT_TELEMETRY:
      LOG("Number of enqueued Light message elements: %lu",
          persistent_self.light_storage.num_msg_elements_enqueued);
      return persistent_self.light_storage.num_msg_elements_enqueued / LIGHT_MSGS_PER_SBD;
      break;

    case ACCELEROMETER_TELEMETRY:
      LOG("Number of enqueued Accelerometer messages: %lu",
          persistent_self.accelerometer_storage.num_telemetry_msgs_enqueued);
      return persistent_self.accelerometer_storage.num_telemetry_msgs_enqueued;
      break;

    default:
      return 0;
  }
}

/**
 * Save an SBD message for later transmission.
 * !! Turbidity and Light messages are saved as single elements, not a full SBD message.
 *
 * @return void
 */
void persistent_ram_save_message ( telemetry_type_t msg_type, uint8_t *msg )
{
  int i = 0;

  // Corruption, lack of initialization check
  if ( persistent_self.magic_number != PERSISTENT_RAM_MAGIC_DOUBLE_WORD )
  {
    _persistent_ram_clear ();
  }

  switch ( msg_type )
  {
    case WAVES_TELEMETRY:
      // If the storage queue is full, see if we can replace an element
      if ( persistent_self.waves_storage.num_telemetry_msgs_enqueued == MAX_NUM_WAVES_MSGS_STORED )
      {
        for ( i = 0; i < MAX_NUM_WAVES_MSGS_STORED; i++ )
        {
          // Want to make sure we are comparing absolute values, though wave height should always be positive
          if ( (fabsf (halfToFloat (persistent_self.waves_storage.msg_queue[i].payload.Hs)))
               < fabsf (halfToFloat (((sbd_message_type_52*) msg)->Hs)) )
          {
            // copy the message over
            memcpy (&(persistent_self.waves_storage.msg_queue[i].payload), msg,
                    sizeof(sbd_message_type_52));
            // Make the entry valid (should already be, but just in case)
            persistent_self.waves_storage.msg_queue[i].valid = true;
            return;
          }
        }
      }
      else
      {
        // Not full, just need to find an open slot
        for ( i = 0; i < MAX_NUM_WAVES_MSGS_STORED; i++ )
        {
          if ( !persistent_self.waves_storage.msg_queue[i].valid )
          {
            // copy the message over
            memcpy (&(persistent_self.waves_storage.msg_queue[i].payload), msg,
                    sizeof(sbd_message_type_52));
            // Make the entry valid
            persistent_self.waves_storage.msg_queue[i].valid = true;
            persistent_self.waves_storage.num_telemetry_msgs_enqueued++;
            return;
          }
        }
      }

      break;

    case ACCELEROMETER_TELEMETRY:
      // If the storage queue is full, see if we can replace an element
      if ( persistent_self.accelerometer_storage.num_telemetry_msgs_enqueued == MAX_NUM_ACCELEROMETER_MSGS_STORED )
      {
        for ( i = 0; i < MAX_NUM_ACCELEROMETER_MSGS_STORED; i++ )
        {
          // TODO: Ask Jim what metric to use here since we do not calculate
          //       wave heights for accelerometer data.
          // NOTE: The previous logic simply replaces the first message with
          //       smaller wave heights; instead, it might make sense to
          //       replace the absolute smallest element.
          if ( (fabsf (halfToFloat (persistent_self.accelerometer_storage.msg_queue[i].payload.max_z_accel)))
               < fabsf (halfToFloat (((sbd_message_type_55*) msg)->max_z_accel)) )
          {
            // copy the message over
            memcpy (&(persistent_self.accelerometer_storage.msg_queue[i].payload), msg,
                    sizeof(sbd_message_type_55));
            // Make the entry valid (should already be, but just in case)
            persistent_self.accelerometer_storage.msg_queue[i].valid = true;
            return;
          }
        }
        // Do we want to log that we're discarding a message due to full queue?
      }
      else
      {
        // Not full, just need to find an open slot
        for ( i = 0; i < MAX_NUM_ACCELEROMETER_MSGS_STORED; i++ )
        {
          if ( !persistent_self.accelerometer_storage.msg_queue[i].valid )
          {
            // copy the message over
            memcpy (&(persistent_self.accelerometer_storage.msg_queue[i].payload), msg,
                    sizeof(sbd_message_type_55));
            // Make the entry valid
            persistent_self.accelerometer_storage.msg_queue[i].valid = true;
            persistent_self.accelerometer_storage.num_telemetry_msgs_enqueued++;
            return;
          }
        }
        // Getting here would indicate an error -- should we log that?
      }

      break;

    case TURBIDITY_TELEMETRY:
      // If the storage queue is full, then just skip
      if ( !(persistent_self.turbidity_storage.num_msg_elements_enqueued
             == MAX_NUM_TURBIDITY_MSGS_STORED) )
      {
        // First check if there is room in the current message index
        for ( i = 0; i < TURBIDITY_MSGS_PER_SBD; i++ )
        {
          if ( !persistent_self.turbidity_storage.msg_queue[persistent_self.turbidity_storage
              .current_msg_index].valid[i] )
          {
            // copy the message over
            memcpy (
                &(persistent_self.turbidity_storage.msg_queue[persistent_self.turbidity_storage
                    .current_msg_index].payload.elements[i]),
                msg, sizeof(sbd_message_type_53_element));
            // Make the entry valid
            persistent_self.turbidity_storage.msg_queue[persistent_self.turbidity_storage
                .current_msg_index].valid[i] = true;
            persistent_self.turbidity_storage.num_msg_elements_enqueued++;
            return;
          }
        }

        // Need to start a new message
        for ( i = 0; i < MAX_NUM_NON_WAVES_MSGS_STORED; i++ )
        {
          if ( !persistent_self.turbidity_storage.msg_queue[i].valid[0] )
          {
            // copy the message over
            memcpy (&(persistent_self.turbidity_storage.msg_queue[i].payload.elements[0]), msg,
                    sizeof(sbd_message_type_53_element));
            // Make the entry valid
            persistent_self.turbidity_storage.msg_queue[i].valid[0] = true;
            persistent_self.turbidity_storage.num_msg_elements_enqueued++;
            persistent_self.turbidity_storage.current_msg_index = i;
            return;
          }
        }
      }

      break;

    case LIGHT_TELEMETRY:
      // If the storage queue is full, then just skip
      if ( !(persistent_self.light_storage.num_msg_elements_enqueued == MAX_NUM_LIGHT_MSGS_STORED) )
      {
        // First check if there is room in the current message index
        for ( i = 0; i < LIGHT_MSGS_PER_SBD; i++ )
        {
          if ( !persistent_self.light_storage.msg_queue[persistent_self.light_storage
              .current_msg_index].valid[i] )
          {
            // copy the message over
            memcpy (
                &(persistent_self.light_storage.msg_queue[persistent_self.light_storage
                    .current_msg_index].payload.elements[i]),
                msg, sizeof(sbd_message_type_54_element));
            // Make the entry valid
            persistent_self.light_storage.msg_queue[persistent_self.light_storage.current_msg_index]
                .valid[i] = true;
            persistent_self.light_storage.num_msg_elements_enqueued++;
            return;
          }
        }

        // Need to start a new message
        for ( i = 0; i < MAX_NUM_NON_WAVES_MSGS_STORED; i++ )
        {
          if ( !persistent_self.light_storage.msg_queue[i].valid[0] )
          {
            // copy the message over
            memcpy (&(persistent_self.light_storage.msg_queue[i].payload.elements[0]), msg,
                    sizeof(sbd_message_type_54_element));
            // Make the entry valid
            persistent_self.light_storage.msg_queue[i].valid[0] = true;
            persistent_self.light_storage.num_msg_elements_enqueued++;
            persistent_self.light_storage.current_msg_index = i;
            return;
          }
        }
      }

      break;

    default:
      return;
  }
}

/**
 * Return a pointer to the highest priority message of a given type. If no message is
 * available, return pointer will be NULL.
 * !!@ This always returns a pointer to a FULL SBD MESSAGE, not just a single message element.
 *
 * @return void
 */
uint8_t* persistent_ram_get_prioritized_unsent_message ( telemetry_type_t msg_type )
{
  // Set to float min value to ensure any message significant wave height will be greater than or equal to
  float most_significant_wave_height = -FLT_MAX;
  float msg_wave_height = 0.0;
  real16_T msg_wave_half_float = { 0 };
  float max_z_accel = -FLT_MAX;
  float msg_z_accel = 0.0;
  real16_T msg_accel_half_float = { 0 };
  int32_t pri_msg_index = 0, valid_msg_index = -1;
  bool msg_full = false;
  uint8_t *ret_ptr = NULL;

  // Corruption, lack of initialization check
  if ( persistent_self.magic_number != PERSISTENT_RAM_MAGIC_DOUBLE_WORD )
  {
    _persistent_ram_clear ();
    return NULL;
  }

  switch ( msg_type )
  {
    case WAVES_TELEMETRY:

      // Empty queue check
      if ( persistent_self.waves_storage.num_telemetry_msgs_enqueued == 0 )
      {
        return NULL;
      }

      // Find the largest significant wave height
      for ( int i = 0; i < MAX_NUM_WAVES_MSGS_STORED; i++ )
      {
        if ( persistent_self.waves_storage.msg_queue[i].valid )
        {
          valid_msg_index = i;
          msg_wave_half_float = persistent_self.waves_storage.msg_queue[i].payload.Hs;
          msg_wave_height = fabsf (halfToFloat (msg_wave_half_float));

          if ( msg_wave_height >= most_significant_wave_height )
          {
            most_significant_wave_height = msg_wave_height;
            pri_msg_index = i;
          }
        }
      }

      // Make sure we don't go out of bounds. Take the priority message first, otherwise, any valid message
      if ( (pri_msg_index >= 0) && (pri_msg_index <= (MAX_NUM_WAVES_MSGS_STORED - 1)) )
      {
        ret_ptr = (uint8_t*) &persistent_self.waves_storage.msg_queue[pri_msg_index].payload;
      }
      else if ( (valid_msg_index >= 0) && (valid_msg_index <= (MAX_NUM_WAVES_MSGS_STORED - 1)) )
      {
        ret_ptr = (uint8_t*) &persistent_self.waves_storage.msg_queue[valid_msg_index].payload;
      }
      else
      {
        ret_ptr = NULL;
      }

      break;

    // This is a clone of the WAVES_TELEMETRY logic
    case ACCELEROMETER_TELEMETRY:

      // Empty queue check
      if ( persistent_self.accelerometer_storage.num_telemetry_msgs_enqueued == 0 )
      {
        return NULL;
      }

      // Find the maximum acceleration in Z.
      // TODO: Update this prioritization.
      for ( int i = 0; i < MAX_NUM_ACCELEROMETER_MSGS_STORED; i++ )
      {
        if ( persistent_self.accelerometer_storage.msg_queue[i].valid )
        {
          valid_msg_index = i;
          msg_accel_half_float = persistent_self.accelerometer_storage.msg_queue[i].payload.max_z_accel;
          msg_z_accel = fabsf (halfToFloat (msg_accel_half_float));

          if ( msg_z_accel >= max_z_accel )
          {
            max_z_accel = msg_z_accel;
            pri_msg_index = i;
          }
        }
      }

      // Make sure we don't go out of bounds. Take the priority message first, otherwise, any valid message
      if ( (pri_msg_index >= 0) && (pri_msg_index <= (MAX_NUM_ACCELEROMETER_MSGS_STORED - 1)) )
      {
        ret_ptr = (uint8_t*) &persistent_self.accelerometer_storage.msg_queue[pri_msg_index].payload;
      }
      else if ( (valid_msg_index >= 0) && (valid_msg_index <= (MAX_NUM_ACCELEROMETER_MSGS_STORED - 1)) )
      {
        ret_ptr = (uint8_t*) &persistent_self.accelerometer_storage.msg_queue[valid_msg_index].payload;
      }
      else
      {
        ret_ptr = NULL;
      }

      break;

    case TURBIDITY_TELEMETRY:

      // Make sure we have enough elements to constitute a full msg
      if ( persistent_self.turbidity_storage.num_msg_elements_enqueued < TURBIDITY_MSGS_PER_SBD )
      {
        return NULL;
      }

      // Just take the first available
      for ( int i = 0; i < MAX_NUM_NON_WAVES_MSGS_STORED; i++ )
      {

        msg_full = true;

        for ( int j = 0; j < TURBIDITY_MSGS_PER_SBD; j++ )
        {
          if ( !persistent_self.turbidity_storage.msg_queue[i].valid[j] )
          {
            msg_full = false;
            break;
          }
        }

        if ( msg_full )
        {
          ret_ptr = (uint8_t*) &persistent_self.turbidity_storage.msg_queue[i].payload;
          return ret_ptr;
        }
      }

      break;

    case LIGHT_TELEMETRY:

      // Make sure we have enough elements to constitute a full msg
      if ( persistent_self.light_storage.num_msg_elements_enqueued < LIGHT_MSGS_PER_SBD )
      {
        return NULL;
      }

      // Just take the first available
      for ( int i = 0; i < MAX_NUM_NON_WAVES_MSGS_STORED; i++ )
      {

        msg_full = true;

        for ( int j = 0; j < LIGHT_MSGS_PER_SBD; j++ )
        {
          if ( !persistent_self.light_storage.msg_queue[i].valid[j] )
          {
            msg_full = false;
            break;
          }
        }

        if ( msg_full )
        {
          ret_ptr = (uint8_t*) &persistent_self.light_storage.msg_queue[i].payload;
          return ret_ptr;
        }
      }

      break;

    default:
      ret_ptr = NULL;
  }

  return ret_ptr;
}

void persistent_ram_delete_message_element ( telemetry_type_t msg_type, uint8_t *msg_ptr )
{
  // Corruption, lack of initialization check
  if ( persistent_self.magic_number != PERSISTENT_RAM_MAGIC_DOUBLE_WORD )
  {
    _persistent_ram_clear ();
    return;
  }

  switch ( msg_type )
  {
    case WAVES_TELEMETRY:

      // Find the pointer
      for ( int i = 0; i < MAX_NUM_WAVES_MSGS_STORED; i++ )
      {
        if ( (uint8_t*) &persistent_self.waves_storage.msg_queue[i].payload == msg_ptr )
        {
          // Zero out the message
          memset (msg_ptr, 0, sizeof(Iridium_Message_Storage_Element_t));
          persistent_self.waves_storage.num_telemetry_msgs_enqueued--;
          break;
        }
      }

      break;

    case ACCELEROMETER_TELEMETRY:

      // Find the pointer
      for ( int i = 0; i < MAX_NUM_ACCELEROMETER_MSGS_STORED; i++ )
      {
        if ( (uint8_t*) &persistent_self.accelerometer_storage.msg_queue[i].payload == msg_ptr )
        {
          // Zero out the message
          memset (msg_ptr, 0, sizeof(Accelerometer_Message_Storage_Element_t));
          persistent_self.accelerometer_storage.num_telemetry_msgs_enqueued--;
          break;
        }
      }

      break;

    case TURBIDITY_TELEMETRY:

      // Find the pointer
      for ( int i = 0; i < MAX_NUM_NON_WAVES_MSGS_STORED; i++ )
      {
        if ( (uint8_t*) &persistent_self.turbidity_storage.msg_queue[i].payload == msg_ptr )
        {
          // Zero out the message
          memset (msg_ptr, 0, sizeof(Turbidity_Message_Storage_Element_t));
          persistent_self.turbidity_storage.num_msg_elements_enqueued -= TURBIDITY_MSGS_PER_SBD;
          break;
        }
      }

      break;

    case LIGHT_TELEMETRY:

      // Find the pointer
      for ( int i = 0; i < MAX_NUM_NON_WAVES_MSGS_STORED; i++ )
      {
        if ( (uint8_t*) &persistent_self.light_storage.msg_queue[i].payload == msg_ptr )
        {
          // Zero out the message
          memset (msg_ptr, 0, sizeof(Light_Message_Storage_Element_t));
          persistent_self.light_storage.num_msg_elements_enqueued -= LIGHT_MSGS_PER_SBD;
          break;
        }
      }

      break;

    case OTA_ACK_MESSAGE:
      persistent_self.ota_acknowledgement_sent = true;
      memset (&persistent_self.ota_acknowledgement_msg, 0, sizeof(sbd_message_type_99));
      break;

    default:
      break;
  }
}

static void _persistent_ram_clear ( void )
{
  // Clear everything out
  memset (&persistent_self, 0, sizeof(Persistent_Storage));
}
