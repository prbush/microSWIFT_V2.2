/*
 * gnss.h
 *
 *  Created on: Oct 28, 2022
 *      Author: Phil
 *
 */

#ifndef SRC_GPS_H_
#define SRC_GPS_H_

#include "configuration.h"
#include "u_ubx_protocol.h"
#include "tx_api.h"
#include "time.h"
#include "microSWIFT_return_codes.h"
#include "stm32u5xx_hal.h"

// Macros
#define GNSS_CONFIG_BUFFER_SIZE 600
#define GNSS_MESSAGE_BUF_SIZE 200
#define CONFIGURATION_ARRAY_SIZE 164
#define MAX_POSSIBLE_VELOCITY 10000     // 10000 mm/s = 10 m/s
#define UBX_NAV_PVT_MESSAGE_CLASS 0x01
#define UBX_NAV_PVT_MESSAGE_ID 0x07
#define UBX_NAV_PVT_MESSAGE_LENGTH 100
#define UBX_CFG_VALSET_CLASS 0x06
#define UBX_CFG_VALSET_ID 0x8A
#define UBX_MESSAGE_SIZE (92 + U_UBX_PROTOCOL_OVERHEAD_LENGTH_BYTES)
#define UBX_BUFFER_SIZE (10 * UBX_MESSAGE_SIZE)
//#define INITIAL_STAGES_BUFFER_SIZE (gnss_self->global_config->gnss_sampling_rate * UBX_NAV_PVT_MESSAGE_LENGTH)
// Testing top hat
#define INITIAL_STAGES_BUFFER_SIZE ((gnss_self->global_config->gnss_sampling_rate * UBX_NAV_PVT_MESSAGE_LENGTH))
#define FRAME_SYNC_RX_SIZE 200
#define UBX_NAV_PVT_PAYLOAD_LENGTH 92
#define UBX_ACK_MESSAGE_LENGTH 10
#define MAX_ACCEPTABLE_SACC 250 // need to confirm with Jim what this should be
#define MAX_ACCEPTABLE_PDOP 600 // (units = 0.01) greater than 6 means poor fix accuracy
#define MAX_EMPTY_QUEUE_WAIT 50 // wait for max 50ms
#define MAX_EMPTY_CYCLES 5*60*10 // no data for 10 mins
#define MAX_FRAME_SYNC_ATTEMPTS 10
#define MAX_CONFIG_STEP_ATTEMPTS 3
#define GNSS_DEFAULT_BAUD_RATE 9600
#define MAX_THREADX_WAIT_TICKS_FOR_CONFIG (TX_TIMER_TICKS_PER_SECOND + (TX_TIMER_TICKS_PER_SECOND / 4))
#define MILLISECONDS_PER_MINUTE 60000
#define MM_PER_METER 1000.0
#define MIN_SATELLITES_TO_PASS_TEST 4
#define LOWER_4_BITS_MASK 0xF
#define LAT_LON_CONVERSION_FACTOR 10000000 // format as 1E-7
#define GNSS_TIMER_INSTANCE TIM16
// UBX message definitions
#define RESOLVED_TIME_BITS ((1 << 0) | (1 << 2)) // Only resolve to the time of day
#define LAT_LON_INVALID_BIT 1U
#define UBX_NAV_PVT_YEAR_INDEX 4
#define UBX_NAV_PVT_MONTH_INDEX 6
#define UBX_NAV_PVT_DAY_INDEX 7
#define UBX_NAV_PVT_HOUR_INDEX 8
#define UBX_NAV_PVT_MINUTE_INDEX 9
#define UBX_NAV_PVT_SECONDS_INDEX 10
#define UBX_NAV_PVT_VALID_FLAGS_INDEX 11
#define UBX_NAV_PVT_TACC_INDEX 12
#define UBX_NAV_PVT_VALID_FLAGS2_INDEX 22
#define UBX_NAV_PVT_NUMSV_INDEX 23
#define UBX_NAV_PVT_LON_INDEX 24
#define UBX_NAV_PVT_LAT_INDEX 28
#define UBX_NAV_PVT_HACC_INDEX 40
#define UBX_NAV_PVT_VACC_INDEX 44
#define UBX_NAV_PVT_V_NORTH_INDEX 48
#define UBX_NAV_PVT_V_EAST_INDEX 52
#define UBX_NAV_PVT_V_DOWN_INDEX 56
#define UBX_NAV_PVT_SACC_INDEX 68
#define UBX_NAV_PVT_PDOP_INDEX 76
#define UBX_NAV_PVT_FLAGS3_INDEX 78
#define UBX_ACK_ACK_CLSID_INDEX 0
#define UBX_ACK_ACK_MSGID_INDEX 1
#define HIGH_PERFORMANCE_QUERY_SIZE 28
#define HIGH_PERFORMANCE_RESPONSE_SIZE 36
#define ENABLE_HIGH_PERFORMANCE_SIZE 60
#define BREADCRUMB_TRACK_MAX_SIZE 4096

#define GNSS_WINDOW_BUFFER_TIME 2
// @formatter:off

typedef struct
{
  float   lat;
  float   lon;
} gnss_track_point;

typedef struct GNSS
{
  // Our global configuration struct
  microSWIFT_configuration  *global_config;
  // The UART and DMA handle for the GNSS interface
  UART_HandleTypeDef        *gnss_uart_handle;
  DMA_HandleTypeDef         *gnss_tx_dma_handle;
  DMA_HandleTypeDef         *gnss_rx_dma_handle;
  // Event flags
  TX_EVENT_FLAGS_GROUP      *irq_flags;
  TX_EVENT_FLAGS_GROUP      *error_flags;
  // Pointer to timer
  TX_TIMER                  *timer;
  // UBX message process buffer filled from DMA ISR
  uint8_t                   *ubx_process_buf;
  // Configuration response buffer
  uint8_t                   *config_response_buf;
  // Velocity sample array pointers
  float                     *GNSS_N_Array;
  float                     *GNSS_E_Array;
  float                     *GNSS_D_Array;
  // Pointer to the breadcrumb track buffer
  gnss_track_point          *breadcrumb_track;
  uint32_t                  breadcrumb_index;
  // Number of messages processed in a given buffer
  uint32_t                  messages_processed;
  // Keep a running track of sum -- to be used in getRunningAverage
  // NOTE: These values are in units of mm/s and must be converted
  int32_t                   v_north_sum;
  int32_t                   v_east_sum;
  int32_t                   v_down_sum;
  // Hold the current lat/long for whatever we might need it for (modem)
  int32_t                   current_latitude;
  int32_t                   current_longitude;
  // The start time for the sampling window
  time_t                    sample_window_start_time;
  // The start time for the sampling window
  time_t                    sample_window_stop_time;
  // The true calculated sample window frequency
  double                    sample_window_freq;
  // Increment with each sample or running average
  uint32_t                  total_samples;
  // We'll keep track of how many times we had to sub in a running average
  uint32_t                  total_samples_averaged;
  // Number of messages which could not be parsed
  uint32_t                  num_bad_messages;
  // How many times we've had to skip a sample - gets reset with valid data
  uint32_t                  number_cycles_without_data;
  // Make sure we have a warmup period to get good data right off the bat
  uint32_t                  warmup_counter;
  // Flags
  bool                      current_fix_is_good;
  bool                      all_resolution_stages_complete;
  bool                      is_configured;
  bool                      is_clock_set;
  bool                      rtc_error;
  bool                      all_samples_processed;
  bool                      timer_timeout;
  // Function pointers
  uSWIFT_return_code_t      (*config) ( void );
  uSWIFT_return_code_t      (*sync_and_start_reception) ( void );
  uSWIFT_return_code_t      (*get_location) ( float *latitude, float *longitude );
  uSWIFT_return_code_t      (*get_running_average_velocities) ( void );
  uSWIFT_return_code_t      (*software_start) ( void );
  uSWIFT_return_code_t      (*software_stop) ( void );
  uSWIFT_return_code_t      (*set_rtc) ( uint8_t *msg_payload );
  uSWIFT_return_code_t      (*reset_uart) ( void );
  uSWIFT_return_code_t      (*start_timer) ( uint16_t timeout_in_minutes );
  uSWIFT_return_code_t      (*stop_timer) ( void );
  void                      (*process_message) ( void );
  void                      (*on) ( void );
  void                      (*off)( void );
} GNSS;


/* Function declarations */
void                    gnss_init ( GNSS *struct_ptr, microSWIFT_configuration *global_config,
                                    UART_HandleTypeDef *gnss_uart_handle, DMA_HandleTypeDef *gnss_tx_dma_handle,
                                    DMA_HandleTypeDef *gnss_rx_dma_handle, TX_EVENT_FLAGS_GROUP *irq_flags,
                                    TX_EVENT_FLAGS_GROUP *error_flags, TX_TIMER *timer, float *GNSS_N_Array,
                                    float *GNSS_E_Array, float *GNSS_D_Array );
void                    gnss_deinit ( void );
void                    gnss_timer_expired ( ULONG expiration_input );
bool                    gnss_get_timer_timeout_status ( void );
bool                    gnss_get_configured_status ( void );
bool                    gnss_get_sample_window_complete ( void );
uSWIFT_return_code_t    gnss_get_current_lat_lon (int32_t *lat, int32_t *lon);
uint32_t                gnss_get_samples_processed ( void );
double                  gnss_get_sample_window_frequency ( void );

// @formatter:on
#endif /* SRC_GPS_H_ */
