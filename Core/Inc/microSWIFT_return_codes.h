/*
 * microSWIFT_return_codes.h
 *
 *  Created on: Nov 6, 2024
 *      Author: philbush
 */
// clang-format off

#ifndef INC_MICROSWIFT_RETURN_CODES_H_
#define INC_MICROSWIFT_RETURN_CODES_H_

//@formatter:off
typedef enum
{
  uSWIFT_SUCCESS                =  0,
  // Common errors used in low level drivers
  uSWIFT_IO_ERROR               = -1,
  uSWIFT_BUSY                   = -2,

  uSWIFT_TIMEOUT                = -3,
  uSWIFT_PARAMETERS_INVALID     = -4,
  uSWIFT_REQUEST_INVALID        = -5,
  uSWIFT_ALREADY_IN_USE         = -6,
  uSWIFT_MESSAGE_QUEUE_ERROR    = -7,
  uSWIFT_EVENT_FLAG_ERROR       = -8,
  uSWIFT_TIMER_ERROR            = -9,
  uSWIFT_EMPTY_REQUEST          = -10,
  uSWIFT_INITIALIZATION_ERROR   = -11,
  uSWIFT_CONFIGURATION_ERROR    = -12,
  uSWIFT_CALIBRATION_ERROR      = -13,
  uSWIFT_NAK_RECEIVED           = -14,
  uSWIFT_PROCESSING_ERROR       = -15,
  uSWIFT_MEMORY_BUFFER_ERROR    = -16,
  uSWIFT_NO_SAMPLES_ERROR       = -17,
  uSWIFT_DONE_SAMPLING          = -18,
  uSWIFT_SYNC_ERROR             = -19,
  uSWIFT_LOCATION_ERROR         = -20,
  uSWIFT_TIME_ERROR             = -21,
  uSWIFT_FILE_SYSTEM_ERROR      = -22,
  // More specific I2C errors
  uSWIFT_I2C_BUSY_ERROR         = -23,
  uSWIFT_I2C_SEMAPHORE_ERROR    = -24,
  uSWIFT_I2C_WRITE_ERROR        = -25,
  uSWIFT_I2C_READ_ERROR         = -26,

  uSWIFT_OBJECT_SPECIFIC_1      = -90,
  uSWIFT_OBJECT_SPECIFIC_2      = -91,
  uSWIFT_OBJECT_SPECIFIC_3      = -92,
  uSWIFT_OBJECT_SPECIFIC_4      = -93,
  uSWIFT_OBJECT_SPECIFIC_5      = -94,
  uSWIFT_OBJECT_SPECIFIC_6      = -95,
  uSWIFT_OBJECT_SPECIFIC_7      = -96,
  uSWIFT_OBJECT_SPECIFIC_8      = -97,
  uSWIFT_OBJECT_SPECIFIC_9      = -98,
  uSWIFT_UNKNOWN_ERROR          = -99,

  __FORCE_32_BIT__              = 0XFFFFFFFF
}uSWIFT_return_code_t;
//@formatter:on
#endif /* INC_MICROSWIFT_RETURN_CODES_H_ */
