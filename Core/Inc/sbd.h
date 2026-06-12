/*
 * sbd.h
 *
 *  Created on: Sep 18, 2024
 *      Author: philbush
 */
// clang-format off

#ifndef INC_SBD_H_
#define INC_SBD_H_

#include <stdint.h>
#include "time.h"
#include "NEDWaves/rtwhalf.h"
#include <stdint.h>

#define TYPE_99_CHAR_BUF_LEN 320
#define IRIDIUM_SBD_MAX_LENGTH 340
#define TELEMETRY_FIELD_ERROR_CODE (0x70E2)

// @formatter:off

typedef struct
{
  uint8_t     checksum_a;
  uint8_t     checksum_b;
} iridium_checksum_t;
#define IRIDIUM_CHECKSUM_LENGTH sizeof(iridium_checksum_t)

// Primary message type for wave dynamic measurements
typedef struct __packed
{
  char                legacy_number_7;
  uint8_t             type;
  uint8_t             port;
  uint16_t            size;
  real16_T            Hs;
  real16_T            Tp;
  real16_T            Dp;
  real16_T            E_array[42];
  real16_T            f_min;
  real16_T            f_max;
  signed    char      a1_array[42];
  signed    char      b1_array[42];
  signed    char      a2_array[42];
  signed    char      b2_array[42];
  unsigned  char      cf_array[42];
  float               Lat;
  float               Lon;
  real16_T            mean_temp;
  real16_T            mean_salinity;
  real16_T            mean_voltage;
  // Do not use this message as an example of how to do timestamps correctly.
  // This should have been uint32_t, but cannot be changed now for backwards compatibility.
  float               timestamp;
  uint32_t            error_bits;
  iridium_checksum_t  checksum;
} sbd_message_type_52;

// Message definition for turbidity (OBS) measurements
typedef struct __packed
{
  int32_t     start_lat;
  int32_t     start_lon;
  int32_t     end_lat;
  int32_t     end_lon;
  uint32_t    start_timestamp;
  uint32_t    end_timestamp;
  uint16_t    backscatter_avgs[17];
  uint16_t    ambient_avgs[17];
} sbd_message_type_53_element;

// NOTE(lindzey): This math doesn't seem right -- shouldn't it also
//    subtract the other non-element fields in the type_53 message?
#define TURBIDITY_MSGS_PER_SBD ((IRIDIUM_SBD_MAX_LENGTH - IRIDIUM_CHECKSUM_LENGTH) / sizeof(sbd_message_type_53_element))

// Definition for the SBD message with multiple sample windows
typedef struct __packed
{
  char                        legacy_number_7;
  uint8_t                     type;
  uint8_t                     port;
  uint16_t                    size;
  uint16_t                    serial_number;
  sbd_message_type_53_element elements[TURBIDITY_MSGS_PER_SBD];
  iridium_checksum_t          checksum;
} sbd_message_type_53;

// Message definition for light measurements
typedef struct __packed
{
  int32_t     start_lat;
  int32_t     start_lon;
  int32_t     end_lat;
  int32_t     end_lon;
  uint32_t    start_timestamp;
  uint32_t    end_timestamp;
  uint16_t    max_reading_clear;
  uint16_t    min_reading_clear;
  uint16_t    avg_clear;
  uint16_t    avg_f1;
  uint16_t    avg_f2;
  uint16_t    avg_f3;
  uint16_t    avg_f4;
  uint16_t    avg_f5;
  uint16_t    avg_f6;
  uint16_t    avg_f7;
  uint16_t    avg_f8;
  uint16_t    avg_dark;
  uint16_t    avg_nir;
  uint16_t    valid_samples;
  uint16_t    failed_samples;
} sbd_message_type_54_element;

// Same question regarding this calculation...
#define LIGHT_MSGS_PER_SBD ((IRIDIUM_SBD_MAX_LENGTH - IRIDIUM_CHECKSUM_LENGTH) / sizeof(sbd_message_type_54_element))

// Definition for the SBD message with multiple sample windows
typedef struct __packed
{
  char                        legacy_number_7;
  uint8_t                     type;
  uint8_t                     port;
  uint16_t                    size;
  sbd_message_type_54_element elements[LIGHT_MSGS_PER_SBD];
  iridium_checksum_t          checksum;
} sbd_message_type_54;

// Definition for accelerometer waves message; based heavily on type_52 for the
// waves component.
typedef struct __packed
{
  char legacy_number_7;  // byte 0
  uint8_t type;  // Used by swift code to determine how to parse bytes
  // 1 byte required here for compatibility.
  // Historically, swift messages used this for port, and microSWIFT has
  // started using it for firmware version.
  uint8_t version;
  uint16_t size;

  uint32_t timestamp; // byte 5-8

  float latitude;  // byte 9-12
  float longitude;  // byte 13-16

  // Bytes 17-27 are reserved for planned wake-on-shake support
  uint8_t reserved01;
  uint16_t reserved02;
  uint8_t reserved03;
  int16_t reserved04;
  int16_t reserved05;
  int16_t reserved06;
  uint8_t reserved07;

  real16_T min_x_accel;  // bytes 28-29
  real16_T max_x_accel;
  real16_T mean_x_accel;

  real16_T min_y_accel;  // bytes 34-35
  real16_T max_y_accel;
  real16_T mean_y_accel;

  real16_T min_z_accel;  // bytes 40-41
  real16_T max_z_accel;
  real16_T mean_z_accel;

  real16_T x_spectra[48];
  real16_T y_spectra[48];
  real16_T z_spectra[48];

  real16_T min_freq;
  real16_T max_freq;

  // 2 bytes for the checksum; this MUST be the final element in the struct,
  // since the iridium thread calculates the checksum and calls memcpy on
  // payload[sizeof(sbd_message_type_55) - IRIDIUM_CHECKSUM_LENGTH]
  iridium_checksum_t  checksum;

} sbd_message_type_55;

typedef struct
{
  char                  message_body[TYPE_99_CHAR_BUF_LEN];
  float                 latitude;
  float                 longitude;
  uint32_t              timestamp;
  iridium_checksum_t    checksum;
} sbd_message_type_99;
// @formatter:on
#endif /* INC_SBD_H_ */
