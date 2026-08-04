/*
 * persistent_ram.h
 *
 *  Created on: Sep 12, 2024
 *      Author: philbush
 */

#ifndef INC_PERSISTENT_RAM_H_
#define INC_PERSISTENT_RAM_H_

#include "NEDWaves/rtwhalf.h"
#include "configuration.h"
#include "sbd.h"
#include "stdbool.h"
#include "stddef.h"

#define PERSISTENT_RAM_MAGIC_DOUBLE_WORD 0x5048494C42555348

// @formatter:off
typedef struct {
  sbd_message_type_52 payload;
  float priority;
} Iridium_Message_Storage_Element_t;

typedef struct {
  sbd_message_type_53 payload;
  bool valid[TURBIDITY_MSGS_PER_SBD];
} Turbidity_Message_Storage_Element_t;

typedef struct {
  sbd_message_type_54 payload;
  bool valid[LIGHT_MSGS_PER_SBD];
} Light_Message_Storage_Element_t;

typedef struct {
  sbd_message_type_55 payload;
  // We're treating this as a priority queue; I'd rather calculate priority
  // once, rather than re-calculating each time we need to insert something
  // into the queue.
  //
  // Sum of X/Y/Z spectra amplitudes across all frequencies
  // -FLT_MAX if message has already been sent or has not been populated.
  float priority;
} Accelerometer_Message_Storage_Element_t;

// NB: We do not change memory allocation based on which sensors are enabled,
//     so some of this may be unused.
// We have 64KB available in .sram2 for persistent storage

// MAX_NUM_NON_WAVES_MSGS_STORED Originally set to 16, by assuming that
// we'd store an equal number of waves and accel messages, and that
// we'd store 5x more waves than light or turbidity
//
// I'd rather tune that manually, and have the numbers explicitly defined.
//
// sizeof(Iridium_Message_Storage_Element_t) = 340
// sizeof(Accelerometer_Message_Storage_Element_t) = 344
// sizeof(Turbidity_Message_Storage_Element_t) = 288  (fits 3 turbidity msgs)
// sizeof(Light_Message_Storage_Element_t) = 337 (fits 6 turbidity msgs)
// total bytes used must be <= 65535U
// with 80x waves; 80x accel; 16x each non_waves, we're at 63.65 KB
// (360 bytes free)
#define MAX_NUM_WAVES_MSGS_STORED 80
#define MAX_NUM_ACCELEROMETER_MSGS_STORED 80

// At least for now, these are coupled (code uses aggregated SBD msgs
// stored vs. total Turbidity/Light message elements stored in different
// loops.
#define MAX_NUM_NON_WAVES_MSGS_STORED 16
#define MAX_NUM_TURBIDITY_MSGS_STORED                                          \
  (MAX_NUM_NON_WAVES_MSGS_STORED * TURBIDITY_MSGS_PER_SBD)
#define MAX_NUM_LIGHT_MSGS_STORED                                              \
  (MAX_NUM_NON_WAVES_MSGS_STORED * LIGHT_MSGS_PER_SBD)

typedef struct {
  Iridium_Message_Storage_Element_t msg_queue[MAX_NUM_WAVES_MSGS_STORED];
  uint32_t num_telemetry_msgs_enqueued;
} Waves_Message_Storage;

typedef struct {
  Turbidity_Message_Storage_Element_t msg_queue[MAX_NUM_NON_WAVES_MSGS_STORED];
  uint32_t num_msg_elements_enqueued;
  uint32_t current_msg_index;
} Turbidity_Message_Storage;

typedef struct {
  Light_Message_Storage_Element_t msg_queue[MAX_NUM_NON_WAVES_MSGS_STORED];
  uint32_t num_msg_elements_enqueued;
  uint32_t current_msg_index;
} Light_Message_Storage;

typedef struct {
  Accelerometer_Message_Storage_Element_t
      msg_queue[MAX_NUM_ACCELEROMETER_MSGS_STORED];
  uint32_t num_telemetry_msgs_enqueued;
} Accelerometer_Message_Storage;

// All of the things we need to retain in standby mode
typedef struct {
  uint64_t magic_number;
  int32_t sample_window_counter;
  uint32_t reset_reason;
  bool rtc_time_set;
  bool ota_update_received;
  bool ota_acknowledgement_sent;
  sbd_message_type_99 ota_acknowledgement_msg;
  Waves_Message_Storage waves_storage;
  Turbidity_Message_Storage turbidity_storage;
  Light_Message_Storage light_storage;
  Accelerometer_Message_Storage accel_storage;
  microSWIFT_configuration device_config;
  microSWIFT_firmware_version_t version;
} Persistent_Storage;

typedef enum {
  WAVES_TELEMETRY = 0,
  TURBIDITY_TELEMETRY = 1,
  LIGHT_TELEMETRY = 2,
  ACCELEROMETER_TELEMETRY = 3,
  OTA_ACK_MESSAGE = 4,
  NO_MESSAGE = 5
} telemetry_type_t;

void persistent_ram_init(const microSWIFT_configuration *config,
                         const microSWIFT_firmware_version_t *version);
void persistent_ram_deinit(void);
void persistent_ram_set_device_config(const microSWIFT_configuration *config,
                                      bool ota_update);
void persistent_ram_get_device_config(microSWIFT_configuration *config);
void persistent_ram_set_firmware_version(
    const microSWIFT_firmware_version_t *version);
void persistent_ram_get_firmware_version(
    microSWIFT_firmware_version_t *version);
bool persistent_ram_get_ota_update_status(void);
bool persistent_ram_get_ota_ack_status(void);
void persistent_ram_set_ota_ack_msg(sbd_message_type_99 *msg);
void persistent_ram_get_ota_ack_msg(sbd_message_type_99 *msg);
uint32_t persistent_ram_get_sample_window_counter(void);
void persistent_ram_reset_sample_window_counter(void);
void persistent_ram_set_rtc_time_set(void);
bool persistent_ram_get_rtc_time_set(void);
uint32_t persistent_ram_get_reset_reason(void);
uint32_t persistent_ram_get_num_msgs_enqueued(telemetry_type_t msg_type);
void persistent_ram_save_message(telemetry_type_t msg_type, float msg_priority,
                                 uint8_t *msg);
uint8_t *
persistent_ram_get_prioritized_unsent_message(telemetry_type_t msg_type);
void persistent_ram_delete_message_element(telemetry_type_t msg_type,
                                           uint8_t *msg_ptr);
void persistent_ram_get_accel_priorities(float *priorities);

// @formatter:on

#endif /* INC_PERSISTENT_RAM_H_ */
