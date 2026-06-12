/*
 * pcf2131_reg.h
 *
 *  Created on: Jul 24, 2024
 *      Author: philbush
 */

#ifndef COMPONENTS_DRIVERS_INC_PCF2131_REG_H_
#define COMPONENTS_DRIVERS_INC_PCF2131_REG_H_

#include <stdint.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>
#include "reg_driver_def.h"

/**************************************************************************************************/
/************************************** Misc Definitions ******************************************/
/**************************************************************************************************/
// @formatter:off
#define BCD_ERROR 0xFF
#define BCD_TO_DEC_SPLIT(t,u)(t*10+u)
#define BCD_TO_DEC_SINGLE(b)(((((b) & 0xF0) >> 4U) * 10U) + ((b) & 0x0F))
#define DEC_TO_BCD(d)(((d) / 10U) << 4U) | ((d) % 10U);
// @formatter:on
#define PCF2131_I2C_ADDR 0b10100110
#define PCF2131_SPI_READ_BIT (1U << 7)

/**************************************************************************************************/
/**************************************** Return Codes ********************************************/
/**************************************************************************************************/
#define PCF2131_OK 0
#define PCF2131_ERROR -1

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*################################## Register Definitions ########################################*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/

/**************************************************************************************************/
/*********************************** Control Register 1 *******************************************/
/**************************************************************************************************/
#define CTRL1_REG_ADDR (0x00)
#define CTRL1_REG_RESET_VAL (0b00001000)

typedef enum
{
  TWENTY_FOUR_HOUR_MODE = 0,
  TWELVE_HOUR_MODE = 1
} hours_mode_t;

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t second_irq_en :1;
  uint8_t minute_irq_en :1;
  hours_mode_t hours_mode :1;
  uint8_t por_override :1;
  uint8_t one_100_sec_disable :1;
  uint8_t stop :1;
  uint8_t temp_comp_disable :1;
  uint8_t ext_clock_test_en :1;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t ext_clock_test_enable :1;
  uint8_t temp_comp_disable :1;
  uint8_t stop :1;
  uint8_t one_100_sec_disable :1;
  uint8_t por_override :1;
  hours_mode_t hours_mode :1;
  uint8_t minute_irq :1;
  uint8_t second_irq :1;
#endif /* DRV_BYTE_ORDER */
} pcf2131_ctrl1_reg_t;

/**************************************************************************************************/
/*********************************** Control Register 2 *******************************************/
/**************************************************************************************************/
#define CTRL2_REG_ADDR (0x01)
#define CTRL2_REG_RESET_VAL (0b00000000)

// Clock defines
#define PCF2131_1_64HZ_CLK_MAX_PERIOD_MS 16320000U
#define PCF2131_1_4HZ_CLK_MAX_PERIOD_MS 1020000U
#define PCF2131_4HZ_CLK_MAX_PERIOD_MS 63744U
#define PCF2131_64HZ_CLK_MAX_PERIOD_MS 3984U

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t T_bit0 :1;
  uint8_t alarm_irq_en :1;
  uint8_t T_bit1 :2;
  uint8_t alarm_flag :1;
  uint8_t T_bit2 :1;
  uint8_t watchdog_flag :1;
  uint8_t min_sec_flag :1;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t min_sec_flag_en :1;
  uint8_t watchdog_flag_en :1;
  uint8_t T_bit :1;
  uint8_t alarm_flag_en :1;
  uint8_t T_bit :2;
  uint8_t alarm_irq_en :1;
  uint8_t T_bit :1;
#endif /* DRV_BYTE_ORDER */
} pcf2131_ctrl2_reg_t;

/**************************************************************************************************/
/*********************************** Control Register 3 *******************************************/
/**************************************************************************************************/
#define CTRL3_REG_ADDR (0x02)
#define CTRL3_REG_RESET_VAL (0b11100000)

typedef enum
{
  BATTERY_OK = 0,
  BATTERY_LOW = 1
} battery_status_t;

typedef enum
{
  SWTCH_OVER_NORMAEN_LOW_BATT_EN = 0b000,
  SWTCH_OVER_NORMAL_EN_LOW_BATT_DIS = 0b010,
  SWTCH_OVER_DIRECT_EN_LOW_BATT_EN = 0b011,
  SWTCH_OVER_DIRECT_EN_LOW_BATT_DIS = 0b101,
  SWITCH_OVER_DIS_LOW_BATT_DIS = 0b111
} pwr_mgmt_t;

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t batt_low_irq_en :1;
  uint8_t batt_irq_en :1;
  battery_status_t batt_stat_flag :1;
  uint8_t batt_switchover_flag :1;
  uint8_t batt_switchover_timestamp_en :1;
  pwr_mgmt_t pwrmng :3;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t pwrmng :3;
  uint8_t batt_switchover_timestamp_en :1;
  uint8_t batt_switchover_flag :1;
  battery_status_t batt_stat_flag :1;
  uint8_t batt_irq_en :1;
  uint8_t batt_low_flag_en :1;
#endif /* DRV_BYTE_ORDER */
} pcf2131_ctrl3_reg_t;

/**************************************************************************************************/
/*********************************** Control Register 4 *******************************************/
/**************************************************************************************************/

#define CTRL4_REG_ADDR (0x03)
#define CTRL4_REG_RESET_VAL (0b00000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t T_bit :4;
  uint8_t timestamp4_flag :1;
  uint8_t timestamp3_flag :1;
  uint8_t timestamp2_flag :1;
  uint8_t timestamp1_flag :1;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t timestamp1_flag :1;
  uint8_t timestamp2_flag :1;
  uint8_t timestamp3_flag :1;
  uint8_t timestamp4_flag :1;
  uint8_t T_bit :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_ctrl4_reg_t;

/**************************************************************************************************/
/*********************************** Control 5 Register *******************************************/
/**************************************************************************************************/
#define CTRL5_REG_ADDR (0x04)
#define CTRL5_REG_RESET_VAL (0b00000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t T_bit :4;
  uint8_t timestamp4_irq_en :1;
  uint8_t timestamp3_irq_en :1;
  uint8_t timestamp2_irq_en :1;
  uint8_t timestamp1_irq_en :1;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t timestamp1_irq_en :1;
  uint8_t timestamp2_irq_en :1;
  uint8_t timestamp3_irq_en :1;
  uint8_t timestamp4_irq_en :1;
  uint8_t T_bit :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_ctrl5_reg_t;

/**************************************************************************************************/
/*********************************** Software Reset Register **************************************/
/**************************************************************************************************/
#define RESET_REG_ADDR (0x05)
#define RESET_REG_RESET_VAL (0b00100100)

typedef enum
{
  SOFTWARE_RESET_BIT_PATTERN = 0b00101100,
  CLEAR_PRESCALAR_BIT_PATTERN = 0b10100100,
  CLEAR_TIMESTAMP_BIT_PATTERN = 0b00100101,
  CLEAR_PRESCALAR_AND_TIMESTAMP_BIT_PATTERN = 0b10100101
} reset_reg_bit_pattern_t;

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  reset_reg_bit_pattern_t bit_pattern :8;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  reset_reg_bit_pattern_t bit_pattern :8;
#endif /* DRV_BYTE_ORDER */
} pcf2131_reset_reg_t;

/**************************************************************************************************/
/*********************************** 1/100 Second Register ****************************************/
/**************************************************************************************************/
#define ONE_100_SEC_REG_ADDR (0x06)
#define ONE_100_SEC_REG_RESET_VAL (0b00000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t units_place :4;
  uint8_t tens_place :4;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t tens_place :4;
  uint8_t units_place :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_one_100_sec_reg_t;

/**************************************************************************************************/
/*********************************** Seconds Register *********************************************/
/**************************************************************************************************/
#define SECONDS_REG_ADDR (0x07)
#define SECONDS_REG_RESET_VAL (0b10000000)

typedef enum
{
  CLOCK_INTEGRITY_GUARANTEED = 0,
  CLOCK_INTEGRITY_NOT_GUARANTEED = 1
} seconds_osf_bit_t;

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t units_place :4;
  uint8_t tens_place :3;
  seconds_osf_bit_t osf :1;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  seconds_osf_bit_t osf :1;
  uint8_t tens_place :3;
  uint8_t units_place :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_seconds_reg_t;

/**************************************************************************************************/
/*********************************** Minutes Register *********************************************/
/**************************************************************************************************/
#define MINUTES_REG_ADDR (0x08)
#define MINUTES_REG_RESET_VAL (0b00000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t units_place :4;
  uint8_t tens_place :3;
  uint8_t dash_bit :1;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t dash_bit :1;
  uint8_t tens_place :3;
  uint8_t units_place :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_minutes_reg_t;

/**************************************************************************************************/
/*********************************** Hour Register ************************************************/
/**************************************************************************************************/
#define HOURS_REG_ADDR (0x09)
#define HOURS_REG_RESET_VAL (0b00000000)

typedef enum
{
  AM = 0,
  PM = 1
} hours_ampm_bit_t;

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t hours_units_place :4;
  uint8_t hours_tens_place :1;
  hours_ampm_bit_t am_pm :1;
  uint8_t dash_bit :2;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t dash_bit :2;
  hours_ampm_bit_t am_pm :1;
  uint8_t hours_tens_place :1;
  uint8_t hours_units_place :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_hours_ampm_format_reg_t;

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t hours_units_place :4;
  uint8_t hours_tens_place :2;
  uint8_t dash_bit :2;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t dash_bit :2;
  uint8_t hours_tens_place :4;
  uint8_t hours_units_place :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_hours_24hr_format_reg_t;

typedef union
{
  pcf2131_hours_ampm_format_reg_t format_ampm;
  pcf2131_hours_24hr_format_reg_t format_24hr;
} pcf2131_hours_reg_t;

/**************************************************************************************************/
/*********************************** Days Register ************************************************/
/**************************************************************************************************/
#define DAYS_REG_ADDR (0x0A)
#define DAYS_REG_RESET_VAL (0b00000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t units_place :4;
  uint8_t tens_place :2;
  uint8_t dash_bit :2;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t dash_bit :2;
  uint8_t tens_place :2;
  uint8_t units_place :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_days_reg_t;

/**************************************************************************************************/
/*********************************** Weekdays Register ********************************************/
/**************************************************************************************************/
#define WEEKDAYS_REG_ADDR (0x0B)
#define WEEKDAYS_REG_RESET_VAL (0b00000001)
#define WEEKDAY_UNKNOWN (-1)

typedef enum
{
  SUNDAY = 0b000,
  MONDAY = 0b001,
  TUESDAY = 0b010,
  WEDNESDAY = 0b011,
  THURSDAY = 0b100,
  FRIDAY = 0b101,
  SATURDAY = 0b110
} weekday_t;

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  weekday_t weekday :3;
  uint8_t dash_bit :5;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t dash_bit :5;
  weekday_t weekday :3;
#endif /* DRV_BYTE_ORDER */
} pcf2131_weekdays_reg_t;

/**************************************************************************************************/
/*********************************** Months Register **********************************************/
/**************************************************************************************************/
#define MONTHS_REG_ADDR (0x0C)
#define MONTHS_REG_RESET_VAL (0b00000001)

typedef enum
{
  JANUARY = 0b00001,
  FEBRUARY = 0b00010,
  MARCH = 0b00011,
  APRIL = 0b00100,
  MAY = 0b00101,
  JUNE = 0b00110,
  JULY = 0b00111,
  AUGUST = 0b01000,
  SEPTERMBER = 0b1001,
  OCTOBER = 0b10000,
  NOVEMBER = 0b10001,
  DECEMBER = 0b10010
} month_t;

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  month_t month :5;
  uint8_t dash_bit :3;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t dash_bit :3;
  month_t month :5;
#endif /* DRV_BYTE_ORDER */
} pcf2131_months_reg_t;

/**************************************************************************************************/
/*********************************** Years Register ***********************************************/
/**************************************************************************************************/
#define YEARS_REG_ADDR (0x0D)
#define YEARS_REG_RESET_VAL (0b00000001)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t units_place :4;
  uint8_t tens_place :4;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t tens_place :4;
  uint8_t units_place :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_years_reg_t;

/**************************************************************************************************/
/*********************************** Second Alarm Register ****************************************/
/**************************************************************************************************/
#define SECOND_ALARM_REG_ADDR (0x0E)
#define SECOND_ALARM_REG_RESET_VAL (0b10000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t units_place :4;
  uint8_t tens_place :3;
  uint8_t alarm_disable :1;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t alarm_disable :1;
  uint8_t tens_place :3;
  uint8_t units_place :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_second_alarm_reg_t;

/**************************************************************************************************/
/*********************************** Minute Alarm Register ****************************************/
/**************************************************************************************************/
#define MINUTE_ALARM_REG_ADDR (0x0F)
#define MINUTE_ALARM_REG_RESET_VAL (0b10000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t units_place :4;
  uint8_t tens_place :3;
  uint8_t alarm_disable :1;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t alarm_disable :1;
  uint8_t tens_place :3;
  uint8_t units_place :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_minute_alarm_reg_t;

/**************************************************************************************************/
/*********************************** Hour Alarm Register ******************************************/
/**************************************************************************************************/
#define HOUR_ALARM_REG_ADDR (0x10)
#define HOUR_ALARM_REG_RESET_VAL (0b10000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t units_place :4;
  uint8_t tens_place :1;
  hours_ampm_bit_t ampm :1;
  uint8_t dash_bit :1;
  uint8_t alarm_disable :1;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t alarm_disable :1;
  uint8_t dash_bit :1;
  hours_ampm_bit_t ampm :1;
  uint8_t tens_place :1;
  uint8_t units_place :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_hour_alarm_ampm_format_reg_t;

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t units_place :4;
  uint8_t tens_place :2;
  uint8_t dash_bit :1;
  uint8_t alarm_disable :1;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t alarm_disable :1;
  uint8_t dash_bit :1;
  uint8_t tens_place :2;
  uint8_t units_place :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_hour_alarm_24hr_format_reg_t;

typedef union
{
  pcf2131_hour_alarm_ampm_format_reg_t format_ampm;
  pcf2131_hour_alarm_24hr_format_reg_t format_24hr;
} pcf2131_hour_alarm_reg_t;

/**************************************************************************************************/
/*********************************** Day Alarm Register *******************************************/
/**************************************************************************************************/
#define DAY_ALARM_REG_ADDR (0x11)
#define DAY_ALARM_REG_RESET_VAL (0b10000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t units_place :4;
  uint8_t tens_place :2;
  uint8_t dash_bit :1;
  uint8_t alarm_disable :1;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t alarm_disable :1;
  uint8_t dash_bit :1;
  uint8_t tens_place :2;
  uint8_t units_place :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_day_alarm_reg_t;

/**************************************************************************************************/
/*********************************** Weekday Alarm Register ***************************************/
/**************************************************************************************************/
#define WEEKDAY_ALARM_REG_ADDR (0x12)
#define WEEKDAY_ALARM_REG_RESET_VAL (0b10000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  weekday_t weekday :3;
  uint8_t dash_bit :4;
  uint8_t alarm_disable :1;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t alarm_disable :1;
  uint8_t dash_bit :4;
  weekday_t weekday :3;
#endif /* DRV_BYTE_ORDER */
} pcf2131_weekday_alarm_reg_t;

/**************************************************************************************************/
/*********************************** Clock Out Control Register ***********************************/
/**************************************************************************************************/
#define CLKOUT_CTRL_REG_ADDR (0x13)
#define CLKOUT_CTRL_REG_RESET_VAL (0b00000000) || (0b00100000)

typedef enum
{
  EVERY_32_MINS = 0b00,
  EVERY_16_MINS = 0b01,
  EVERY_8_MINS = 0b10,
  EVERY_4_MINS = 0b11
} temp_meas_period_t;

typedef enum
{
  FREQ_32768 = 0b000,
  FREQ_16384 = 0b001,
  FREQ_8192 = 0b010,
  FREQ_4096 = 0b011,
  FREQ_2048 = 0b100,
  FREQ_1024 = 0b101,
  FREQ_1 = 0b110,
  CLKOUT_HI_Z = 0b111
} clock_frequency_t;

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  clock_frequency_t clk_freq :3;
  uint8_t dash_bit :2;
  uint8_t otp_refresh :1;
  temp_meas_period_t temp_meas_p :2;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  temp_meas_period_t temp_meas_p :2;
  uint8_t otp_refresh_performed :1;
  uint8_t dash_bit :2;
  clock_frequency_t clk_freq :3;
#endif /* DRV_BYTE_ORDER */
} pcf2131_clkout_ctrl_reg_t;

/**************************************************************************************************/
/*********************************** Timestamp 1 Control Register *********************************/
/**************************************************************************************************/
#define TIMESTAMP_1_CTRL_REG_ADDR (0x14)
#define TIMESTAMP_1_CTRL_REG_RESET_VAL (0b00000000)

typedef enum
{
  LAST_EVENT_STORED = 0,
  FIRST_EVENT_STORED = 1
} timestamp_subsequent_event_t;

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t bcd_subsecond :5;
  uint8_t dash_bit :1;
  uint8_t timestamp_disable :1;
  timestamp_subsequent_event_t timestamp_store_option :1;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  timestamp_subsequent_event_t timestamp_store_option :1;
  uint8_t timestamp_disable :1;
  uint8_t dash_bit :1;
  uint8_t bcd_subsecond :5;
#endif /* DRV_BYTE_ORDER */
} pcf2131_timestamp_1_ctrl_reg_t;

/**************************************************************************************************/
/*********************************** Seconds Timestamp 1 Register *********************************/
/**************************************************************************************************/
#define SEC_TIMESTAMP_1_REG_ADDR (0x15)
#define SEC_TIMESTAMP_1_REG_RESET_VAL (0b00000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t units_place :4;
  uint8_t tens_place :3;
  uint8_t dash_bit :1;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t dash_bit :1;
  uint8_t tens_place :3;
  uint8_t units_place :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_timestamp_1_sec_reg_t;

/**************************************************************************************************/
/*********************************** Minute Timestamp 1 Register **********************************/
/**************************************************************************************************/
#define MIN_TIMESTAMP_1_REG_ADDR (0x16)
#define MIN_TIMESTAMP_1_REG_RESET_VAL (0b00000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t units_place :4;
  uint8_t tens_place :3;
  uint8_t dash_bit :1;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t dash_bit :1;
  uint8_t tens_place :3;
  uint8_t units_place :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_timestamp_1_min_reg_t;

/**************************************************************************************************/
/*********************************** Hour Timestamp 1 Register ************************************/
/**************************************************************************************************/
#define HOUR_TIMESTAMP_1_REG_ADDR (0x17)
#define HOUR_TIMESTAMP_1_REG_RESET_VAL (0b00000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t units_place :4;
  uint8_t tens_place :1;
  hours_ampm_bit_t ampm :1;
  uint8_t dash_bit :2;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t dash_bit :2;
  hours_ampm_bit_t ampm :1;
  uint8_t tens_place :1;
  uint8_t units_place :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_timestamp_1_hour_ampm_format_reg_t;

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t units_place :4;
  uint8_t tens_place :2;
  uint8_t dash_bit :2;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t dash_bit :2;
  uint8_t tens_place :2;
  uint8_t units_place :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_timestamp_1_hour_24hr_format_reg_t;

typedef union
{
  pcf2131_timestamp_1_hour_ampm_format_reg_t format_ampm;
  pcf2131_timestamp_1_hour_24hr_format_reg_t format_24hr;
} pcf2131_timestamp_1_hour_reg_t;

/**************************************************************************************************/
/*********************************** Day Timestamp 1 Register *************************************/
/**************************************************************************************************/
#define DAY_TIMESTAMP_1_REG_ADDR (0x18)
#define DAY_TIMESTAMP_1_REG_RESET_VAL (0b00000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t units_place :4;
  uint8_t tens_place :2;
  uint8_t dash_bit :2;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t dash_bit :2;
  uint8_t tens_place :2;
  uint8_t units_place :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_timestamp_1_day_reg_t;

/**************************************************************************************************/
/*********************************** Month Timestamp 1 Register ***********************************/
/**************************************************************************************************/
#define MONTH_TIMESTAMP_1_REG_ADDR (0x19)
#define MONTH_TIMESTAMP_1_REG_RESET_VAL (0b00000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  month_t month :5;
  uint8_t dash_bit :3;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t dash_bit :3;
  month_t month :5;
#endif /* DRV_BYTE_ORDER */
} pcf2131_timestamp_1_month_reg_t;

/**************************************************************************************************/
/*********************************** Year Timestamp 1 Register ************************************/
/**************************************************************************************************/
#define YEAR_TIMESTAMP_1_REG_ADDR (0x1A)
#define YEAR_TIMESTAMP_1_REG_RESET_VAL (0b00000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t units_place :4;
  uint8_t tens_place :4;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t tens_place :4;
  uint8_t units_place :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_timestamp_1_year_reg_t;

/**************************************************************************************************/
/*********************************** Timestamp 2 Control Register *********************************/
/**************************************************************************************************/
#define TIMESTAMP_2_CTRL_REG_ADDR (0x1B)
#define TIMESTAMP_2_CTRL_REG_RESET_VAL (0b00000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t bcd_subsecond :5;
  uint8_t dash_bit :1;
  uint8_t timestamp_disable :1;
  timestamp_subsequent_event_t timestamp_store_option :1;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  timestamp_subsequent_event_t timestamp_store_option :1;
  uint8_t timestamp_disable :1;
  uint8_t dash_bit :1;
  uint8_t bcd_subsecond :5;
#endif /* DRV_BYTE_ORDER */
} pcf2131_timestamp_2_ctrl_reg_t;

/**************************************************************************************************/
/*********************************** Seconds Timestamp 2 Register *********************************/
/**************************************************************************************************/
#define SEC_TIMESTAMP_2_REG_ADDR (0x1C)
#define SEC_TIMESTAMP_2_REG_RESET_VAL (0b00000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t units_place :4;
  uint8_t tens_place :3;
  uint8_t dash_bit :1;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t dash_bit :1;
  uint8_t tens_place :3;
  uint8_t units_place :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_timestamp_2_sec_reg_t;

/**************************************************************************************************/
/*********************************** Minute Timestamp 2 Register **********************************/
/**************************************************************************************************/
#define MIN_TIMESTAMP_2_REG_ADDR (0x1D)
#define MIN_TIMESTAMP_2_REG_RESET_VAL (0b00000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t units_place :4;
  uint8_t tens_place :3;
  uint8_t dash_bit :1;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t dash_bit :1;
  uint8_t tens_place :3;
  uint8_t units_place :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_timestamp_2_min_reg_t;

/**************************************************************************************************/
/*********************************** Hour Timestamp 2 Register ************************************/
/**************************************************************************************************/
#define HOUR_TIMESTAMP_2_REG_ADDR (0x1E)
#define HOUR_TIMESTAMP_2_REG_RESET_VAL (0b00000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t units_place :4;
  uint8_t tens_place :1;
  hours_ampm_bit_t ampm :1;
  uint8_t dash_bit :2;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t dash_bit :2;
  hours_ampm_bit_t ampm :1;
  uint8_t tens_place :1;
  uint8_t units_place :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_timestamp_2_hour_ampm_format_reg_t;

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t units_place :4;
  uint8_t tens_place :2;
  uint8_t dash_bit :2;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t dash_bit :2;
  uint8_t tens_place :2;
  uint8_t units_place :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_timestamp_2_hour_24hr_format_reg_t;

typedef union
{
  pcf2131_timestamp_2_hour_ampm_format_reg_t format_ampm;
  pcf2131_timestamp_2_hour_24hr_format_reg_t format_24hr;
} pcf2131_timestamp_2_hour_reg_t;

/**************************************************************************************************/
/*********************************** Day Timestamp 2 Register *************************************/
/**************************************************************************************************/
#define DAY_TIMESTAMP_2_REG_ADDR (0x1F)
#define DAY_TIMESTAMP_2_REG_RESET_VAL (0b00000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t units_place :4;
  uint8_t tens_place :2;
  uint8_t dash_bit :2;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t dash_bit :2;
  uint8_t tens_place :2;
  uint8_t units_place :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_timestamp_2_day_reg_t;

/**************************************************************************************************/
/*********************************** Month Timestamp 2 Register ***********************************/
/**************************************************************************************************/
#define MONTH_TIMESTAMP_2_REG_ADDR (0x20)
#define MONTH_TIMESTAMP_2_REG_RESET_VAL (0b00000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  month_t month :5;
  uint8_t dash_bit :3;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t dash_bit :3;
  month_t month :5;
#endif /* DRV_BYTE_ORDER */
} pcf2131_timestamp_2_month_reg_t;

/**************************************************************************************************/
/*********************************** Year Timestamp 2 Register ************************************/
/**************************************************************************************************/
#define YEAR_TIMESTAMP_2_REG_ADDR (0x21)
#define YEAR_TIMESTAMP_2_REG_RESET_VAL (0b00000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t units_place :4;
  uint8_t tens_place :4;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t tens_place :4;
  uint8_t units_place :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_timestamp_2_year_reg_t;

/**************************************************************************************************/
/*********************************** Timestamp 3 Control Register *********************************/
/**************************************************************************************************/
#define TIMESTAMP_3_CTRL_REG_ADDR (0x22)
#define TIMESTAMP_3_CTRL_REG_RESET_VAL (0b00000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t bcd_subsecond :5;
  uint8_t dash_bit :1;
  uint8_t timestamp_disable :1;
  timestamp_subsequent_event_t timestamp_store_option :1;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  timestamp_subsequent_event_t timestamp_store_option :1;
  uint8_t timestamp_disable :1;
  uint8_t dash_bit :1;
  uint8_t bcd_subsecond :5;
#endif /* DRV_BYTE_ORDER */
} pcf2131_timestamp_3_ctrl_reg_t;

/**************************************************************************************************/
/*********************************** Seconds Timestamp 3 Register *********************************/
/**************************************************************************************************/
#define SEC_TIMESTAMP_3_REG_ADDR (0x23)
#define SEC_TIMESTAMP_3_REG_RESET_VAL (0b00000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t units_place :4;
  uint8_t tens_place :3;
  uint8_t dash_bit :1;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t dash_bit :1;
  uint8_t tens_place :3;
  uint8_t units_place :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_timestamp_3_sec_reg_t;

/**************************************************************************************************/
/*********************************** Minute Timestamp 3 Register **********************************/
/**************************************************************************************************/
#define MIN_TIMESTAMP_3_REG_ADDR (0x24)
#define MIN_TIMESTAMP_3_REG_RESET_VAL (0b00000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t units_place :4;
  uint8_t tens_place :3;
  uint8_t dash_bit :1;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t dash_bit :1;
  uint8_t tens_place :3;
  uint8_t units_place :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_timestamp_3_min_reg_t;

/**************************************************************************************************/
/*********************************** Hour Timestamp 3 Register ************************************/
/**************************************************************************************************/
#define HOUR_TIMESTAMP_3_REG_ADDR (0x25)
#define HOUR_TIMESTAMP_3_REG_RESET_VAL (0b00000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t units_place :4;
  uint8_t tens_place :1;
  hours_ampm_bit_t ampm :1;
  uint8_t dash_bit :2;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t dash_bit :2;
  hours_ampm_bit_t ampm :1;
  uint8_t tens_place :1;
  uint8_t units_place :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_timestamp_3_hour_ampm_format_reg_t;

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t units_place :4;
  uint8_t tens_place :2;
  uint8_t dash_bit :2;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t dash_bit :2;
  uint8_t tens_place :2;
  uint8_t units_place :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_timestamp_3_hour_24hr_format_reg_t;

typedef union
{
  pcf2131_timestamp_3_hour_ampm_format_reg_t format_ampm;
  pcf2131_timestamp_3_hour_24hr_format_reg_t format_24hr;
} pcf2131_timestamp_3_hour_reg_t;

/**************************************************************************************************/
/*********************************** Day Timestamp 3 Register *************************************/
/**************************************************************************************************/
#define DAY_TIMESTAMP_3_REG_ADDR (0x26)
#define DAY_TIMESTAMP_3_REG_RESET_VAL (0b00000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t units_place :4;
  uint8_t tens_place :2;
  uint8_t dash_bit :2;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t dash_bit :2;
  uint8_t tens_place :2;
  uint8_t units_place :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_timestamp_3_day_reg_t;

/**************************************************************************************************/
/*********************************** Month Timestamp 3 Register ***********************************/
/**************************************************************************************************/
#define MONTH_TIMESTAMP_3_REG_ADDR (0x27)
#define MONTH_TIMESTAMP_3_REG_RESET_VAL (0b00000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  month_t month :5;
  uint8_t dash_bit :3;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t dash_bit :3;
  month_t month :5;
#endif /* DRV_BYTE_ORDER */
} pcf2131_timestamp_3_month_reg_t;

/**************************************************************************************************/
/*********************************** Year Timestamp 3 Register ************************************/
/**************************************************************************************************/
#define YEAR_TIMESTAMP_3_REG_ADDR (0x28)
#define YEAR_TIMESTAMP_3_REG_RESET_VAL (0b00000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t units_place :4;
  uint8_t tens_place :4;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t tens_place :4;
  uint8_t units_place :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_timestamp_3_year_reg_t;

/**************************************************************************************************/
/*********************************** Timestamp 4 Control Register *********************************/
/**************************************************************************************************/
#define TIMESTAMP_4_CTRL_REG_ADDR (0x29)
#define TIMESTAMP_4_CTRL_REG_RESET_VAL (0b00000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t bcd_subsecond :5;
  uint8_t dash_bit :1;
  uint8_t timestamp_disable :1;
  timestamp_subsequent_event_t timestamp_store_option :1;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  timestamp_subsequent_event_t timestamp_store_option :1;
  uint8_t timestamp_disable :1;
  uint8_t dash_bit :1;
  uint8_t bcd_subsecond :5;
#endif /* DRV_BYTE_ORDER */
} pcf2131_timestamp_4_ctrl_reg_t;

/**************************************************************************************************/
/*********************************** Seconds Timestamp 4 Register *********************************/
/**************************************************************************************************/
#define SEC_TIMESTAMP_4_REG_ADDR (0x2A)
#define SEC_TIMESTAMP_4_REG_RESET_VAL (0b00000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t units_place :4;
  uint8_t tens_place :3;
  uint8_t dash_bit :1;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t dash_bit :1;
  uint8_t tens_place :3;
  uint8_t units_place :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_timestamp_4_sec_reg_t;

/**************************************************************************************************/
/*********************************** Minute Timestamp 4 Register **********************************/
/**************************************************************************************************/
#define MIN_TIMESTAMP_4_REG_ADDR (0x2B)
#define MIN_TIMESTAMP_4_REG_RESET_VAL (0b00000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t units_place :4;
  uint8_t tens_place :3;
  uint8_t dash_bit :1;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t dash_bit :1;
  uint8_t tens_place :3;
  uint8_t units_place :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_timestamp_4_min_reg_t;

/**************************************************************************************************/
/*********************************** Hour Timestamp 4 Register ************************************/
/**************************************************************************************************/
#define HOUR_TIMESTAMP_4_REG_ADDR (0x2C)
#define HOUR_TIMESTAMP_4_REG_RESET_VAL (0b00000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t units_place :4;
  uint8_t tens_place :1;
  hours_ampm_bit_t ampm :1;
  uint8_t dash_bit :2;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t dash_bit :2;
  hours_ampm_bit_t ampm :1;
  uint8_t tens_place :1;
  uint8_t units_place :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_timestamp_4_hour_ampm_format_reg_t;

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t units_place :4;
  uint8_t tens_place :2;
  uint8_t dash_bit :2;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t dash_bit :2;
  uint8_t tens_place :2;
  uint8_t units_place :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_timestamp_4_hour_24hr_format_reg_t;

typedef union
{
  pcf2131_timestamp_4_hour_ampm_format_reg_t format_ampm;
  pcf2131_timestamp_4_hour_24hr_format_reg_t format_24hr;
} pcf2131_timestamp_4_hour_reg_t;

/**************************************************************************************************/
/*********************************** Day Timestamp 4 Register *************************************/
/**************************************************************************************************/
#define DAY_TIMESTAMP_4_REG_ADDR (0x2D)
#define DAY_TIMESTAMP_4_REG_RESET_VAL (0b00000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t units_place :4;
  uint8_t tens_place :2;
  uint8_t dash_bit :2;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t dash_bit :2;
  uint8_t tens_place :2;
  uint8_t units_place :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_timestamp_4_day_reg_t;

/**************************************************************************************************/
/*********************************** Month Timestamp 4 Register ***********************************/
/**************************************************************************************************/
#define MONTH_TIMESTAMP_4_REG_ADDR (0x2E)
#define MONTH_TIMESTAMP_4_REG_RESET_VAL (0b00000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  month_t month :5;
  uint8_t dash_bit :3;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t dash_bit :3;
  month_t month :5;
#endif /* DRV_BYTE_ORDER */
} pcf2131_timestamp_4_month_reg_t;

/**************************************************************************************************/
/*********************************** Year Timestamp 4 Register ************************************/
/**************************************************************************************************/
#define YEAR_TIMESTAMP_4_REG_ADDR (0x2F)
#define YEAR_TIMESTAMP_4_REG_RESET_VAL (0b00000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t units_place :4;
  uint8_t tens_place :4;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t tens_place :4;
  uint8_t units_place :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_timestamp_4_year_reg_t;

/**************************************************************************************************/
/*********************************** Aging Offset Register ****************************************/
/**************************************************************************************************/
#define AGING_OFFSET_REG_ADDR (0x30)
#define AGING_OFFSET_REG_RESET_VAL (0b00001000)

typedef enum
{
  PLUS_16 = 0b0000,
  PLUS_14 = 0b0001,
  PLUS_12 = 0b0010,
  PLUS_10 = 0b0011,
  PLUS_8 = 0b0100,
  PLUS_6 = 0b0101,
  PLUS_4 = 0b0110,
  PLUS_2 = 0b0111,
  ZERO = 0b1000,
  MINUS_2 = 0b1001,
  MINUS_4 = 0b1010,
  MINUS_6 = 0b1011,
  MINUS_8 = 0b1100,
  MINUS_10 = 0b1101,
  MINUS_12 = 0b1110,
  MINUS_14 = 0b1111
} aging_offset_t;

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  aging_offset_t offset :4;
  uint8_t dash_bit :4;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t dash_bit :4;
  aging_offset_t offset :4;
#endif /* DRV_BYTE_ORDER */
} pcf2131_aging_offset_reg_t;

/**************************************************************************************************/
/*********************************** Interrupt A Mask 1 Register **********************************/
/**************************************************************************************************/
#define INT_A_MASK_1_REG_ADDR (0x31)
#define INT_A_MASK_1_REG_RESET_VAL (0b00111111)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t low_battery_irq_mask :1;
  uint8_t battery_flag_irq_mask :1;
  uint8_t alarm_irq_mask :1;
  uint8_t watchdog_irq_mask :1;
  uint8_t sec_irq_mask :1;
  uint8_t min_irq_mask :1;
  uint8_t dash_bit :2;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t dash_bit :2;
  uint8_t min_irq_mask :1;
  uint8_t sec_irq_mask :1;
  uint8_t watchdog_irq_mask :1;
  uint8_t alarm_irq_mask :1;
  uint8_t battery_flag_irq_mask :1;
  uint8_t low_battery_irq_mask :1;
#endif /* DRV_BYTE_ORDER */
} pcf2131_int_a_mask_1_reg_t;

/**************************************************************************************************/
/*********************************** Interrupt A Mask 2 Register **********************************/
/**************************************************************************************************/
#define INT_A_MASK_2_REG_ADDR (0x32)
#define INT_A_MASK_2_REG_RESET_VAL (0b00001111)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t timestamp_4_irq_mask :1;
  uint8_t timestamp_3_irq_mask :1;
  uint8_t timestamp_2_irq_mask :1;
  uint8_t timestamp_1_irq_mask :1;
  uint8_t dash_bit :4;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t dash_bit :4;
  uint8_t timestamp_1_irq_mask :1;
  uint8_t timestamp_2_irq_mask :1;
  uint8_t timestamp_3_irq_mask :1;
  uint8_t timestamp_4_irq_mask :1;
#endif /* DRV_BYTE_ORDER */
} pcf2131_int_a_mask_2_reg_t;

/**************************************************************************************************/
/*********************************** Interrupt B Mask 1 Register **********************************/
/**************************************************************************************************/
#define INT_B_MASK_1_REG_ADDR (0x33)
#define INT_B_MASK_1_REG_RESET_VAL (0b00111111)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t low_battery_irq_mask :1;
  uint8_t battery_flag_irq_mask :1;
  uint8_t alarm_irq_mask :1;
  uint8_t watchdog_irq_mask :1;
  uint8_t sec_irq_mask :1;
  uint8_t min_irq_mask :1;
  uint8_t dash_bit :2;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t dash_bit :2;
  uint8_t min_irq_mask :1;
  uint8_t sec_irq_mask :1;
  uint8_t watchdog_irq_mask :1;
  uint8_t alarm_irq_mask :1;
  uint8_t battery_flag_irq_mask :1;
  uint8_t low_battery_irq_mask :1;
#endif /* DRV_BYTE_ORDER */
} pcf2131_int_b_mask_1_reg_t;

/**************************************************************************************************/
/*********************************** Interrupt B Mask 2 Register **********************************/
/**************************************************************************************************/
#define INT_B_MASK_2_REG_ADDR (0x34)
#define INT_B_MASK_2_REG_RESET_VAL (0b00001111)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t timestamp_4_irq_mask :1;
  uint8_t timestamp_3_irq_mask :1;
  uint8_t timestamp_2_irq_mask :1;
  uint8_t timestamp_1_irq_mask :1;
  uint8_t dash_bit :4;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t dash_bit :4;
  uint8_t timestamp_1_irq_mask :1;
  uint8_t timestamp_2_irq_mask :1;
  uint8_t timestamp_3_irq_mask :1;
  uint8_t timestamp_4_irq_mask :1;
#endif /* DRV_BYTE_ORDER */
} pcf2131_int_b_mask_2_reg_t;

/**************************************************************************************************/
/*********************************** Watchdog Timer Control Register ******************************/
/**************************************************************************************************/
#define WATCHDOG_TIM_CTRL_REG_ADDR (0x35)
#define WATCHDOG_TIM_CTRL_REG_RESET_VAL (0b00000011)

typedef enum
{
  LATCHED_SIGNAL = 0,
  PULSED_SIGNAL = 1
} int_signal_behavior_t;

typedef enum
{
  HZ_64 = 0b00,
  HZ_4 = 0b01,
  HZ_1_4 = 0b10,
  HZ_1_64 = 0b11
} watchdog_time_source_t;

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  watchdog_time_source_t clock_source :2;
  uint8_t dash_bit :3;
  int_signal_behavior_t irq_signal_behavior :1;
  uint8_t T_bit :1;
  uint8_t interrupt_enable :1;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t interrupt_enable :1;
  uint8_t T_bit :1;
  watchdog_int_signal_t irq_signal_behavior :1;
  uint8_t dash_bit :3;
  watchdog_time_source_t clock_source :2;
#endif /* DRV_BYTE_ORDER */
} pcf2131_watchdog_tim_ctrl_reg_t;

/**************************************************************************************************/
/*********************************** Watchdog Timer Value Register ********************************/
/**************************************************************************************************/
#define WATCHDOG_TIM_VALUE_REG_ADDR (0x36)
#define WATCHDOG_TIM_VALUE_REG_RESET_VAL (0b00000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t timer_count :8;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t timer_count :8;
#endif /* DRV_BYTE_ORDER */
} pcf2131_watchdog_tim_value_reg_t;

/**************************************************************************************************/
/*********************************** Register Union ***********************************************/
/**************************************************************************************************/

typedef union
{
  pcf2131_ctrl1_reg_t ctrl1;
  pcf2131_ctrl2_reg_t ctrl2;
  pcf2131_ctrl3_reg_t ctrl3;
  pcf2131_ctrl3_reg_t ctrl4;
  pcf2131_ctrl4_reg_t ctrl5;
  pcf2131_reset_reg_t reset;
  pcf2131_one_100_sec_reg_t one_100_sec;
  pcf2131_seconds_reg_t seconds;
  pcf2131_minutes_reg_t minutes;
  pcf2131_hours_reg_t hours;
  pcf2131_days_reg_t days;
  pcf2131_weekdays_reg_t weekday;
  pcf2131_months_reg_t months;
  pcf2131_years_reg_t years;
  pcf2131_second_alarm_reg_t second_alarm;
  pcf2131_minute_alarm_reg_t minute_alarm;
  pcf2131_hour_alarm_reg_t hour_alarm;
  pcf2131_day_alarm_reg_t day_alarm;
  pcf2131_weekday_alarm_reg_t weekday_alarm;
  pcf2131_clkout_ctrl_reg_t clkout_ctrl;
  pcf2131_timestamp_1_ctrl_reg_t timestamp_1_ctrl;
  pcf2131_timestamp_1_sec_reg_t timestamp_1_sec;
  pcf2131_timestamp_1_min_reg_t timestamp_1_min;
  pcf2131_timestamp_1_hour_reg_t timestamp_1_hour;
  pcf2131_timestamp_1_day_reg_t timestamp_1_day;
  pcf2131_timestamp_1_month_reg_t timestamp_1_month;
  pcf2131_timestamp_1_year_reg_t timestamp_1_year;
  pcf2131_timestamp_2_ctrl_reg_t timestamp_2_ctrl;
  pcf2131_timestamp_2_sec_reg_t timestamp_2_sec;
  pcf2131_timestamp_2_min_reg_t timestamp_2_min;
  pcf2131_timestamp_2_hour_reg_t timestamp_2_hour;
  pcf2131_timestamp_2_day_reg_t timestamp_2_day;
  pcf2131_timestamp_2_month_reg_t timestamp_2_month;
  pcf2131_timestamp_2_year_reg_t timestamp_2_year;
  pcf2131_timestamp_3_ctrl_reg_t timestamp_3_ctrl;
  pcf2131_timestamp_3_sec_reg_t timestamp_3_sec;
  pcf2131_timestamp_3_min_reg_t timestamp_3_min;
  pcf2131_timestamp_3_hour_reg_t timestamp_3_hour;
  pcf2131_timestamp_3_day_reg_t timestamp_3_day;
  pcf2131_timestamp_3_month_reg_t timestamp_3_month;
  pcf2131_timestamp_3_year_reg_t timestamp_3_year;
  pcf2131_timestamp_4_ctrl_reg_t timestamp_4_ctrl;
  pcf2131_timestamp_4_sec_reg_t timestamp_4_sec;
  pcf2131_timestamp_4_min_reg_t timestamp_4_min;
  pcf2131_timestamp_4_hour_reg_t timestamp_4_hour;
  pcf2131_timestamp_4_day_reg_t timestamp_4_day;
  pcf2131_timestamp_4_month_reg_t timestamp_4_month;
  pcf2131_timestamp_4_year_reg_t timestamp_4_year;
  pcf2131_int_a_mask_1_reg_t int_a_mask_1;
  pcf2131_int_a_mask_2_reg_t int_a_mask_2;
  pcf2131_int_b_mask_1_reg_t int_b_mask_1;
  pcf2131_int_b_mask_2_reg_t int_b_mask_2;
  pcf2131_watchdog_tim_ctrl_reg_t watchdog_tim_ctrl;
  pcf2131_watchdog_tim_value_reg_t watchdog_tim_val;
  uint8_t byte;
} pcf2131_reg_t;

/**************************************************************************************************/
/*********************************** Misc structs, enums, and typedefs ****************************/
/**************************************************************************************************/
// Define the timestamps
typedef enum
{
  TIMESTAMP_1 = 0,
  TIMESTAMP_2 = 1,
  TIMESTAMP_3 = 2,
  TIMESTAMP_4 = 3,
  NUMBER_OF_TIMESTAMPS = 4
} pcf2131_timestamp_t;

typedef struct
{
  // Enable seconds interrupt
  bool sec_irq_en;
  // Enable minutes interrupt
  bool min_irq_en;
  // Enable pulsed interrupt mode for seconds and/or minutes interrupt (latched mode otherwise)
  bool sec_min_pulsed_irq_en;
  // Enable watchdog interrupt
  bool watchdog_irq_en;
  // Enable alarm interrupt
  bool alarm_irq_en;
  // Enable battery flag
  bool batt_flag_irq_en;
  // Enable low battery interrupt
  bool batt_low_irq_en;
  // Enable timestamp interrupts
  bool timestamp_1_irq_en;
  bool timestamp_2_irq_en;
  bool timestamp_3_irq_en;
  bool timestamp_4_irq_en;

  pcf2131_int_a_mask_1_reg_t int_a_mask_1;
  pcf2131_int_a_mask_2_reg_t int_a_mask_2;
  pcf2131_int_b_mask_1_reg_t int_b_mask_1;
  pcf2131_int_b_mask_2_reg_t int_b_mask_2;
} pcf2131_irq_config_struct;

typedef struct
{
  uint8_t alarm_second;
  uint8_t alarm_minute;
  uint8_t alarm_hour;
  uint8_t alarm_day;
  weekday_t alarm_weekday;
  bool second_alarm_en;
  bool minute_alarm_en;
  bool hour_alarm_en;
  bool day_alarm_en;
  bool weekday_alarm_en;
} rtc_alarm_struct;

typedef struct
{
  uint8_t tens_place;
  uint8_t units_place;
} bcd_struct_t;

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*################################## Function Declarations #######################################*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/

int32_t pcf2131_register_io_functions ( dev_ctx_t *dev_handle, dev_init_ptr init_fn,
                                        dev_deinit_ptr deinit_fn, dev_write_ptr bus_write_fn,
                                        dev_read_ptr bus_read_fn, dev_ms_delay_ptr delay,
                                        void *optional_handle );
int32_t pcf2131_set_date_time ( dev_ctx_t *dev_handle, struct tm *input_date_time );
int32_t pcf2131_get_date_time ( dev_ctx_t *dev_handle, struct tm *return_date_time );
int32_t pcf2131_set_alarm ( dev_ctx_t *dev_handle, rtc_alarm_struct *alarm_setting );
int32_t pcf2131_get_alarm ( dev_ctx_t *dev_handle, rtc_alarm_struct *return_alarm_setting );
int32_t pcf2131_config_interrupts ( dev_ctx_t *dev_handle, pcf2131_irq_config_struct *irq_config );
int32_t pcf2131_config_int_signal_behavior ( dev_ctx_t *dev_handle, int_signal_behavior_t behavior );
int32_t pcf2131_set_timestamp_enable ( dev_ctx_t *dev_handle, pcf2131_timestamp_t which_timestamp,
bool enable );
int32_t pcf2131_poro_config ( dev_ctx_t *dev_handle, bool en );
int32_t pcf2131_set_timestamp_store_option ( dev_ctx_t *dev_handle,
                                             pcf2131_timestamp_t which_timestamp,
                                             timestamp_subsequent_event_t option );
int32_t pcf2131_get_timestamp ( dev_ctx_t *dev_handle, pcf2131_timestamp_t which_timestamp,
                                struct tm *return_date_time );
int32_t pcf2131_temp_comp_config ( dev_ctx_t *dev_handle, bool en );
int32_t pcf2131_set_stop_bit ( dev_ctx_t *dev_handle );
int32_t pcf2131_clear_stop_bit ( dev_ctx_t *dev_handle );
int32_t pcf2131_one_100_sec_counter_config ( dev_ctx_t *dev_handle, bool en );
int32_t pcf2131_hours_format_config ( dev_ctx_t *dev_handle, hours_ampm_bit_t hours_format );
int32_t pcf2131_get_msf_flag ( dev_ctx_t *dev_handle, bool *return_flag );
int32_t pcf2131_clear_msf_flag ( dev_ctx_t *dev_handle );
int32_t pcf2131_get_watchdog_flag ( dev_ctx_t *dev_handle, bool *return_flag );
int32_t pcf2131_clear_watchdog_flag ( dev_ctx_t *dev_handle );
int32_t pcf2131_get_alarm_flag ( dev_ctx_t *dev_handle, bool *return_flag );
int32_t pcf2131_clear_alarm_flag ( dev_ctx_t *dev_handle );
int32_t pcf2131_get_battery_status_flag ( dev_ctx_t *dev_handle, bool *return_flag );
int32_t pcf2131_clear_battery_status_flag ( dev_ctx_t *dev_handle );
int32_t pcf2131_get_battery_switch_over_flag ( dev_ctx_t *dev_handle, bool *return_flag );
int32_t pcf2131_clear_battery_switch_over_flag ( dev_ctx_t *dev_handle );
int32_t pcf2131_get_timestamp_flag ( dev_ctx_t *dev_handle, pcf2131_timestamp_t which_timestamp,
bool *return_flag );
int32_t pcf2131_clear_timestamp_flag ( dev_ctx_t *dev_handle, pcf2131_timestamp_t which_timestamp );
int32_t pcf2131_set_temp_meas_period ( dev_ctx_t *dev_handle, temp_meas_period_t meas_period );
int32_t pcf2131_set_clkout_freq ( dev_ctx_t *dev_handle, clock_frequency_t freq_out );
int32_t pcf2131_perform_otp_refresh ( dev_ctx_t *dev_handle );
int32_t pcf2131_set_crystal_aging_offset ( dev_ctx_t *dev_handle, aging_offset_t offset );
int32_t pcf2131_config_pwr_mgmt_scheme ( dev_ctx_t *dev_handle, pwr_mgmt_t mgmt_scheme );
int32_t pcf2131_get_osf_flag ( dev_ctx_t *dev_handle, seconds_osf_bit_t *return_flag );
int32_t pcf2131_software_reset ( dev_ctx_t *dev_handle );
int32_t pcf2131_clear_prescalar ( dev_ctx_t *dev_handle );
int32_t pcf2131_clear_timestamps ( dev_ctx_t *dev_handle );
int32_t pcf2131_clear_prescalar_and_timestamps ( dev_ctx_t *dev_handle );
int32_t pcf2131_watchdog_config_time_source ( dev_ctx_t *dev_handle,
                                              watchdog_time_source_t time_source );
int32_t pcf2131_set_watchdog_timer_value ( dev_ctx_t *dev_handle, uint8_t timer_value );

#endif /* COMPONENTS_DRIVERS_INC_PCF2131_REG_H_ */
