/*
 * pcf2131_reg.c
 *
 *  Created on: Jul 24, 2024
 *      Author: philbush
 */

#include "pcf2131_reg.h"
#include "string.h"

static uint8_t __weekday_from_date ( int y, int m, int d );
static bcd_struct_t __dec_to_bcd_struct ( uint8_t decimal_val );

int32_t pcf2131_register_io_functions ( dev_ctx_t *dev_handle, dev_init_ptr init_fn,
                                        dev_deinit_ptr deinit_fn, dev_write_ptr bus_write_fn,
                                        dev_read_ptr bus_read_fn, dev_ms_delay_ptr delay,
                                        void *optional_handle )
{
  dev_handle->init = init_fn;
  dev_handle->deinit = deinit_fn;
  dev_handle->bus_read = bus_read_fn;
  dev_handle->bus_write = bus_write_fn;
  dev_handle->handle = optional_handle;
  dev_handle->delay = delay;

  return dev_handle->init ();
}

int32_t pcf2131_set_date_time ( dev_ctx_t *dev_handle, struct tm *input_date_time )
{
  int32_t ret = PCF2131_OK;
  uint8_t weekday =
      (input_date_time->tm_wday == WEEKDAY_UNKNOWN) ?
          __weekday_from_date (input_date_time->tm_year, input_date_time->tm_mon,
                               input_date_time->tm_mday) :
          (uint8_t) input_date_time->tm_wday;
  uint8_t bcd_year = (uint8_t) input_date_time->tm_year;
  uint8_t bcd_month = (uint8_t) input_date_time->tm_mon;
  uint8_t bcd_day = (uint8_t) input_date_time->tm_mday;
  uint8_t bcd_hour = (uint8_t) input_date_time->tm_hour;
  uint8_t bcd_min = (uint8_t) input_date_time->tm_min;
  uint8_t bcd_sec = (uint8_t) input_date_time->tm_sec;
  uint8_t bcd_sec_fraction = 0x00;
  uint8_t rtc_date_time[9] =
    { CLEAR_PRESCALAR_BIT_PATTERN, bcd_sec_fraction, bcd_sec, bcd_min, bcd_hour, bcd_day, weekday,
      bcd_month, bcd_year };

  /*
   • set STOP bit
   • set CPR (register SR_RESET, CPR is logic 1)
   • address counter rolls over to address 06h
   • set time (100th seconds, seconds to years)
   • wait for external time reference to indicate that time counting should start
   • clear STOP bit (time starts counting from now)
   */

  ret = pcf2131_set_stop_bit (dev_handle);

//  ret |= pcf2131_clear_prescalar (dev_handle);

  ret |= dev_handle->bus_write (NULL, 0, RESET_REG_ADDR, &(rtc_date_time[0]),
                                sizeof(rtc_date_time));

  ret |= pcf2131_clear_stop_bit (dev_handle);

  return ret;
}

int32_t pcf2131_get_date_time ( dev_ctx_t *dev_handle, struct tm *return_date_time )
{
  int32_t ret = PCF2131_OK;
  pcf2131_reg_t time_date[8] =
    { 0 };
  uint8_t time_date_bytes[8] =
    { 0 };

  ret = dev_handle->bus_read (NULL, 0, ONE_100_SEC_REG_ADDR, (uint8_t*) &(time_date[0]),
                              sizeof(time_date));

  memcpy ((void*) &(time_date_bytes[0]), (void*) &(time_date[0]), sizeof(time_date_bytes));

  return_date_time->tm_sec = BCD_TO_DEC_SPLIT(time_date[1].seconds.tens_place,
                                              time_date[1].seconds.units_place);
  return_date_time->tm_min = BCD_TO_DEC_SPLIT(time_date[2].minutes.tens_place,
                                              time_date[2].minutes.units_place);
  return_date_time->tm_hour = BCD_TO_DEC_SPLIT(time_date[3].hours.format_24hr.hours_tens_place,
                                               time_date[3].hours.format_24hr.hours_units_place);
  return_date_time->tm_mday = BCD_TO_DEC_SPLIT(time_date[4].days.tens_place,
                                               time_date[4].days.units_place);
  return_date_time->tm_wday = time_date[5].weekday.weekday;
  return_date_time->tm_mon = BCD_TO_DEC_SPLIT(((time_date[6].months.month & 0x10) >> 4),
      (time_date[6].months.month & 0x0F))
                             - 1;
  return_date_time->tm_year = (BCD_TO_DEC_SPLIT(time_date[7].years.tens_place,
      time_date[7].years.units_place)
                               + 2000)
                              - 1900;
  return_date_time->tm_yday = 0;
  return_date_time->tm_isdst = -1;

  return ret;
}

int32_t pcf2131_set_alarm ( dev_ctx_t *dev_handle, rtc_alarm_struct *alarm_setting )
{
  int32_t ret = PCF2131_OK;
  pcf2131_ctrl1_reg_t ctrl =
    { 0 };
  pcf2131_second_alarm_reg_t second =
    { 0 };
  pcf2131_minute_alarm_reg_t minute =
    { 0 };
  pcf2131_hour_alarm_reg_t hour =
    { 0 };
  pcf2131_day_alarm_reg_t day =
    { 0 };
  pcf2131_weekday_alarm_reg_t weekday =
    { 0 };
  bcd_struct_t bcd_vals =
    { 0 };

  // Set 24 hour mode
  ret |= dev_handle->bus_read (NULL, 0, CTRL1_REG_ADDR, (uint8_t*) &ctrl,
                               sizeof(pcf2131_ctrl1_reg_t));
  ctrl.hours_mode = TWENTY_FOUR_HOUR_MODE;
  ret |= dev_handle->bus_write (NULL, 0, CTRL1_REG_ADDR, (uint8_t*) &ctrl,
                                sizeof(pcf2131_ctrl1_reg_t));

  if ( alarm_setting->second_alarm_en )
  {
    bcd_vals = __dec_to_bcd_struct (alarm_setting->alarm_second);
    second.tens_place = bcd_vals.tens_place;
    second.units_place = bcd_vals.units_place;
    second.alarm_disable = 0b0;

    ret |= dev_handle->bus_write (NULL, 0, SECOND_ALARM_REG_ADDR, (uint8_t*) &second,
                                  sizeof(pcf2131_second_alarm_reg_t));
  }
  else
  {
    ret |= dev_handle->bus_read (NULL, 0, SECOND_ALARM_REG_ADDR, (uint8_t*) &second,
                                 sizeof(pcf2131_second_alarm_reg_t));
    second.alarm_disable = 0b1;
    ret |= dev_handle->bus_write (NULL, 0, SECOND_ALARM_REG_ADDR, (uint8_t*) &second,
                                  sizeof(pcf2131_second_alarm_reg_t));
  }

  if ( alarm_setting->minute_alarm_en )
  {
    bcd_vals = __dec_to_bcd_struct (alarm_setting->alarm_minute);
    minute.tens_place = bcd_vals.tens_place;
    minute.units_place = bcd_vals.units_place;
    minute.alarm_disable = 0b0;

    ret |= dev_handle->bus_write (NULL, 0, MINUTE_ALARM_REG_ADDR, (uint8_t*) &minute,
                                  sizeof(pcf2131_minute_alarm_reg_t));
  }
  else
  {
    ret |= dev_handle->bus_read (NULL, 0, MINUTE_ALARM_REG_ADDR, (uint8_t*) &minute,
                                 sizeof(pcf2131_minute_alarm_reg_t));
    minute.alarm_disable = 0b1;
    ret |= dev_handle->bus_write (NULL, 0, MINUTE_ALARM_REG_ADDR, (uint8_t*) &minute,
                                  sizeof(pcf2131_minute_alarm_reg_t));
  }

  if ( alarm_setting->hour_alarm_en )
  {
    bcd_vals = __dec_to_bcd_struct (alarm_setting->alarm_hour);
    hour.format_24hr.tens_place = bcd_vals.tens_place;
    hour.format_24hr.units_place = bcd_vals.units_place;
    hour.format_24hr.alarm_disable = 0b0;

    ret |= dev_handle->bus_write (NULL, 0, HOUR_ALARM_REG_ADDR, (uint8_t*) &hour,
                                  sizeof(pcf2131_hour_alarm_reg_t));
  }
  else
  {
    ret |= dev_handle->bus_read (NULL, 0, HOUR_ALARM_REG_ADDR, (uint8_t*) &hour,
                                 sizeof(pcf2131_hour_alarm_reg_t));
    hour.format_24hr.alarm_disable = 0b1;
    ret |= dev_handle->bus_write (NULL, 0, HOUR_ALARM_REG_ADDR, (uint8_t*) &hour,
                                  sizeof(pcf2131_hour_alarm_reg_t));
  }

  if ( alarm_setting->day_alarm_en )
  {
    bcd_vals = __dec_to_bcd_struct (alarm_setting->alarm_day);
    day.tens_place = bcd_vals.tens_place;
    day.units_place = bcd_vals.units_place;
    day.alarm_disable = 0b0;

    ret |= dev_handle->bus_write (NULL, 0, DAY_ALARM_REG_ADDR, (uint8_t*) &day,
                                  sizeof(pcf2131_day_alarm_reg_t));
  }
  else
  {
    ret |= dev_handle->bus_read (NULL, 0, DAY_ALARM_REG_ADDR, (uint8_t*) &day,
                                 sizeof(pcf2131_day_alarm_reg_t));
    day.alarm_disable = 0b1;
    ret |= dev_handle->bus_write (NULL, 0, DAY_ALARM_REG_ADDR, (uint8_t*) &day,
                                  sizeof(pcf2131_day_alarm_reg_t));
  }

  if ( alarm_setting->weekday_alarm_en )
  {
    weekday.weekday = alarm_setting->alarm_weekday;
    weekday.alarm_disable = 0b0;

    ret |= dev_handle->bus_write (NULL, 0, WEEKDAY_ALARM_REG_ADDR, (uint8_t*) &weekday,
                                  sizeof(pcf2131_weekday_alarm_reg_t));
  }
  else
  {
    ret |= dev_handle->bus_read (NULL, 0, WEEKDAY_ALARM_REG_ADDR, (uint8_t*) &weekday,
                                 sizeof(pcf2131_weekday_alarm_reg_t));
    weekday.alarm_disable = 0b1;
    ret |= dev_handle->bus_write (NULL, 0, WEEKDAY_ALARM_REG_ADDR, (uint8_t*) &weekday,
                                  sizeof(pcf2131_weekday_alarm_reg_t));
  }

  rtc_alarm_struct validate_alarm;
  pcf2131_get_alarm (dev_handle, &validate_alarm);

  return ret;
}

int32_t pcf2131_get_alarm ( dev_ctx_t *dev_handle, rtc_alarm_struct *return_alarm_setting )
{
  int32_t ret = PCF2131_OK;
  pcf2131_second_alarm_reg_t second =
    { 0 };
  pcf2131_minute_alarm_reg_t minute =
    { 0 };
  pcf2131_hour_alarm_reg_t hour =
    { 0 };
  pcf2131_day_alarm_reg_t day =
    { 0 };
  pcf2131_weekday_alarm_reg_t weekday =
    { 0 };

  ret |= dev_handle->bus_read (NULL, 0, WEEKDAY_ALARM_REG_ADDR, (uint8_t*) &weekday,
                               sizeof(pcf2131_weekday_alarm_reg_t));
  ret |= dev_handle->bus_read (NULL, 0, DAY_ALARM_REG_ADDR, (uint8_t*) &day,
                               sizeof(pcf2131_day_alarm_reg_t));
  ret |= dev_handle->bus_read (NULL, 0, HOUR_ALARM_REG_ADDR, (uint8_t*) &hour,
                               sizeof(pcf2131_hour_alarm_reg_t));
  ret |= dev_handle->bus_read (NULL, 0, MINUTE_ALARM_REG_ADDR, (uint8_t*) &minute,
                               sizeof(pcf2131_minute_alarm_reg_t));
  ret |= dev_handle->bus_read (NULL, 0, SECOND_ALARM_REG_ADDR, (uint8_t*) &second,
                               sizeof(pcf2131_second_alarm_reg_t));

  return_alarm_setting->alarm_weekday = weekday.weekday;
  return_alarm_setting->weekday_alarm_en = !weekday.alarm_disable;

  return_alarm_setting->alarm_day = BCD_TO_DEC_SPLIT(day.tens_place, day.units_place);
  return_alarm_setting->day_alarm_en = !day.alarm_disable;

  return_alarm_setting->alarm_hour = BCD_TO_DEC_SPLIT(hour.format_24hr.tens_place,
                                                      hour.format_24hr.units_place);
  return_alarm_setting->hour_alarm_en = !hour.format_24hr.alarm_disable;

  return_alarm_setting->alarm_minute = BCD_TO_DEC_SPLIT(minute.tens_place, minute.units_place);
  return_alarm_setting->minute_alarm_en = !minute.alarm_disable;

  return_alarm_setting->alarm_second = BCD_TO_DEC_SPLIT(second.tens_place, second.units_place);
  return_alarm_setting->second_alarm_en = !second.alarm_disable;

  return ret;
}

int32_t pcf2131_config_interrupts ( dev_ctx_t *dev_handle, pcf2131_irq_config_struct *irq_config )
{
  int32_t ret = PCF2131_OK;
  pcf2131_ctrl5_reg_t ctrl5;
  pcf2131_ctrl3_reg_t ctrl3;
  pcf2131_ctrl2_reg_t ctrl2;
  pcf2131_ctrl1_reg_t ctrl1;
  pcf2131_watchdog_tim_ctrl_reg_t watchdog_reg;

  // Read current register contents
  ret |= dev_handle->bus_read (NULL, 0, CTRL5_REG_ADDR, (uint8_t*) &ctrl5,
                               sizeof(pcf2131_ctrl5_reg_t));
  ret |= dev_handle->bus_read (NULL, 0, CTRL3_REG_ADDR, (uint8_t*) &ctrl3,
                               sizeof(pcf2131_ctrl3_reg_t));
  ret |= dev_handle->bus_read (NULL, 0, CTRL2_REG_ADDR, (uint8_t*) &ctrl2,
                               sizeof(pcf2131_ctrl2_reg_t));
  ret |= dev_handle->bus_read (NULL, 0, CTRL1_REG_ADDR, (uint8_t*) &ctrl1,
                               sizeof(pcf2131_ctrl1_reg_t));
  ret |= dev_handle->bus_read (NULL, 0, WATCHDOG_TIM_CTRL_REG_ADDR, (uint8_t*) &watchdog_reg,
                               sizeof(pcf2131_watchdog_tim_ctrl_reg_t));

  /* Minute/ Second Interrupts */
  ctrl1.minute_irq_en = irq_config->min_irq_en;
  ctrl1.second_irq_en = irq_config->sec_irq_en;
  watchdog_reg.irq_signal_behavior = (irq_config->sec_min_pulsed_irq_en) ?
      PULSED_SIGNAL : LATCHED_SIGNAL;

  /* Watchdog Interrupt */
  watchdog_reg.interrupt_enable = irq_config->watchdog_irq_en;

  /* Timestamp Interrupts */
  ctrl5.timestamp4_irq_en = irq_config->timestamp_4_irq_en;
  ctrl5.timestamp3_irq_en = irq_config->timestamp_3_irq_en;
  ctrl5.timestamp2_irq_en = irq_config->timestamp_2_irq_en;
  ctrl5.timestamp1_irq_en = irq_config->timestamp_1_irq_en;

  /* Battery Interrupts */
  ctrl3.batt_irq_en = irq_config->batt_flag_irq_en;
  ctrl3.batt_low_irq_en = irq_config->batt_low_irq_en;

  /* Alarm Interrupt */
  ctrl2.alarm_irq_en = irq_config->alarm_irq_en;

  // Write back to the registers
  ret |= dev_handle->bus_write (NULL, 0, CTRL5_REG_ADDR, (uint8_t*) &ctrl5,
                                sizeof(pcf2131_ctrl5_reg_t));
  ret |= dev_handle->bus_write (NULL, 0, CTRL3_REG_ADDR, (uint8_t*) &ctrl3,
                                sizeof(pcf2131_ctrl3_reg_t));
  ret |= dev_handle->bus_write (NULL, 0, CTRL2_REG_ADDR, (uint8_t*) &ctrl2,
                                sizeof(pcf2131_ctrl2_reg_t));
  ret |= dev_handle->bus_write (NULL, 0, CTRL1_REG_ADDR, (uint8_t*) &ctrl1,
                                sizeof(pcf2131_ctrl1_reg_t));
  ret |= dev_handle->bus_write (NULL, 0, INT_A_MASK_1_REG_ADDR,
                                (uint8_t*) &irq_config->int_a_mask_1,
                                sizeof(pcf2131_int_a_mask_1_reg_t));
  ret |= dev_handle->bus_write (NULL, 0, INT_A_MASK_2_REG_ADDR,
                                (uint8_t*) &irq_config->int_a_mask_2,
                                sizeof(pcf2131_int_a_mask_2_reg_t));
  ret |= dev_handle->bus_write (NULL, 0, INT_B_MASK_1_REG_ADDR,
                                (uint8_t*) &irq_config->int_b_mask_1,
                                sizeof(pcf2131_int_b_mask_1_reg_t));
  ret |= dev_handle->bus_write (NULL, 0, INT_B_MASK_2_REG_ADDR,
                                (uint8_t*) &irq_config->int_b_mask_2,
                                sizeof(pcf2131_int_b_mask_2_reg_t));
  ret |= dev_handle->bus_write (NULL, 0, WATCHDOG_TIM_CTRL_REG_ADDR, (uint8_t*) &watchdog_reg,
                                sizeof(pcf2131_watchdog_tim_ctrl_reg_t));

  return ret;
}

int32_t pcf2131_config_int_signal_behavior ( dev_ctx_t *dev_handle, int_signal_behavior_t behavior )
{
  int32_t ret = PCF2131_OK;
  pcf2131_watchdog_tim_ctrl_reg_t watchdog_reg;

  ret |= dev_handle->bus_read (NULL, 0, WATCHDOG_TIM_CTRL_REG_ADDR, (uint8_t*) &watchdog_reg,
                               sizeof(pcf2131_watchdog_tim_ctrl_reg_t));

  watchdog_reg.irq_signal_behavior = behavior;

  ret |= dev_handle->bus_write (NULL, 0, WATCHDOG_TIM_CTRL_REG_ADDR, (uint8_t*) &watchdog_reg,
                                sizeof(pcf2131_watchdog_tim_ctrl_reg_t));

  return ret;
}

int32_t pcf2131_set_timestamp_enable ( dev_ctx_t *dev_handle, pcf2131_timestamp_t which_timestamp,
bool enable )
{
  int32_t ret = PCF2131_OK;
  uint8_t reg_addr = TIMESTAMP_1_CTRL_REG_ADDR + (7 * which_timestamp);
  pcf2131_timestamp_1_ctrl_reg_t reg_bits;

  ret |= dev_handle->bus_read (NULL, 0, reg_addr, (uint8_t*) &reg_bits,
                               sizeof(pcf2131_timestamp_1_ctrl_reg_t));

  reg_bits.timestamp_disable = !enable;

  ret |= dev_handle->bus_write (NULL, 0, reg_addr, (uint8_t*) &reg_bits,
                                sizeof(pcf2131_timestamp_1_ctrl_reg_t));

  return ret;
}

int32_t pcf2131_poro_config ( dev_ctx_t *dev_handle, bool en )
{
  int32_t ret = PCF2131_OK;
  pcf2131_ctrl1_reg_t ctrl1;

  ret |= dev_handle->bus_read (NULL, 0, CTRL1_REG_ADDR, (uint8_t*) &ctrl1,
                               sizeof(pcf2131_ctrl1_reg_t));

  ctrl1.por_override = en;

  ret |= dev_handle->bus_write (NULL, 0, CTRL1_REG_ADDR, (uint8_t*) &ctrl1,
                                sizeof(pcf2131_ctrl1_reg_t));

  return ret;
}

int32_t pcf2131_set_timestamp_store_option ( dev_ctx_t *dev_handle,
                                             pcf2131_timestamp_t which_timestamp,
                                             timestamp_subsequent_event_t option )
{
  int32_t ret = PCF2131_OK;
  uint8_t reg_addr = TIMESTAMP_1_CTRL_REG_ADDR + (7 * which_timestamp);
  pcf2131_timestamp_1_ctrl_reg_t reg_bits;

  ret |= dev_handle->bus_read (NULL, 0, reg_addr, (uint8_t*) &reg_bits,
                               sizeof(pcf2131_timestamp_1_ctrl_reg_t));

  reg_bits.timestamp_store_option = option;

  ret |= dev_handle->bus_write (NULL, 0, reg_addr, (uint8_t*) &reg_bits,
                                sizeof(pcf2131_timestamp_1_ctrl_reg_t));

  return ret;
}

int32_t pcf2131_get_timestamp ( dev_ctx_t *dev_handle, pcf2131_timestamp_t which_timestamp,
                                struct tm *return_date_time )
{
  int32_t ret = PCF2131_OK;
  pcf2131_reg_t timestamp[6] =
    { 0 };
  uint8_t reg_addr = SEC_TIMESTAMP_1_REG_ADDR + (7 * which_timestamp);

  ret |= dev_handle->bus_read (NULL, 0, reg_addr, (uint8_t*) &(timestamp[0]), sizeof(timestamp));

  return_date_time->tm_sec = BCD_TO_DEC_SPLIT(timestamp[0].timestamp_1_sec.tens_place,
                                              timestamp[0].timestamp_1_sec.units_place);
  return_date_time->tm_min = BCD_TO_DEC_SPLIT(timestamp[1].timestamp_1_min.tens_place,
                                              timestamp[1].timestamp_1_min.units_place);
  return_date_time->tm_hour = BCD_TO_DEC_SPLIT(
      timestamp[2].timestamp_1_hour.format_24hr.tens_place,
      timestamp[2].timestamp_1_hour.format_24hr.units_place);
  return_date_time->tm_mday = BCD_TO_DEC_SPLIT(timestamp[3].timestamp_1_day.tens_place,
                                               timestamp[3].timestamp_1_day.units_place);
  return_date_time->tm_mon = timestamp[4].timestamp_1_month.month;
  return_date_time->tm_year = (BCD_TO_DEC_SPLIT(timestamp[5].timestamp_1_year.tens_place,
      timestamp[5].timestamp_1_year.units_place)
                               + 2000)
                              - 1900;
  return_date_time->tm_wday = __weekday_from_date (return_date_time->tm_year,
                                                   return_date_time->tm_mon,
                                                   return_date_time->tm_mday);
  return_date_time->tm_yday = 0;
  return_date_time->tm_isdst = -1;

  return ret;
}

int32_t pcf2131_temp_comp_config ( dev_ctx_t *dev_handle, bool en )
{
  int32_t ret = PCF2131_OK;
  pcf2131_ctrl1_reg_t ctrl1;

  ret |= dev_handle->bus_read (NULL, 0, CTRL1_REG_ADDR, (uint8_t*) &ctrl1,
                               sizeof(pcf2131_ctrl1_reg_t));

  ctrl1.temp_comp_disable = !en;

  ret |= dev_handle->bus_write (NULL, 0, CTRL1_REG_ADDR, (uint8_t*) &ctrl1,
                                sizeof(pcf2131_ctrl1_reg_t));

  return ret;
}

int32_t pcf2131_set_stop_bit ( dev_ctx_t *dev_handle )
{
  int32_t ret = PCF2131_OK;
  pcf2131_ctrl1_reg_t ctrl1;

  ret |= dev_handle->bus_read (NULL, 0, CTRL1_REG_ADDR, (uint8_t*) &ctrl1,
                               sizeof(pcf2131_ctrl1_reg_t));

  ctrl1.stop = 0b1;

  ret |= dev_handle->bus_write (NULL, 0, CTRL1_REG_ADDR, (uint8_t*) &ctrl1,
                                sizeof(pcf2131_ctrl1_reg_t));

  return ret;
}

int32_t pcf2131_clear_stop_bit ( dev_ctx_t *dev_handle )
{
  int32_t ret = PCF2131_OK;
  pcf2131_ctrl1_reg_t ctrl1;

  ret |= dev_handle->bus_read (NULL, 0, CTRL1_REG_ADDR, (uint8_t*) &ctrl1,
                               sizeof(pcf2131_ctrl1_reg_t));

  ctrl1.stop = 0b0;

  ret |= dev_handle->bus_write (NULL, 0, CTRL1_REG_ADDR, (uint8_t*) &ctrl1,
                                sizeof(pcf2131_ctrl1_reg_t));

  return ret;
}

int32_t pcf2131_one_100_sec_counter_config ( dev_ctx_t *dev_handle, bool en )
{
  int32_t ret = PCF2131_OK;
  pcf2131_ctrl1_reg_t ctrl1;

  ret |= dev_handle->bus_read (NULL, 0, CTRL1_REG_ADDR, (uint8_t*) &ctrl1,
                               sizeof(pcf2131_ctrl1_reg_t));

  ctrl1.one_100_sec_disable = !en;

  ret |= dev_handle->bus_write (NULL, 0, CTRL1_REG_ADDR, (uint8_t*) &ctrl1,
                                sizeof(pcf2131_ctrl1_reg_t));

  return ret;
}

int32_t pcf2131_hours_format_config ( dev_ctx_t *dev_handle, hours_ampm_bit_t hours_format )
{
  int32_t ret = PCF2131_OK;
  pcf2131_ctrl1_reg_t ctrl1;

  ret |= dev_handle->bus_read (NULL, 0, CTRL1_REG_ADDR, (uint8_t*) &ctrl1,
                               sizeof(pcf2131_ctrl1_reg_t));

  ctrl1.hours_mode = hours_format;

  ret |= dev_handle->bus_write (NULL, 0, CTRL1_REG_ADDR, (uint8_t*) &ctrl1,
                                sizeof(pcf2131_ctrl1_reg_t));

  return ret;
}

int32_t pcf2131_get_msf_flag ( dev_ctx_t *dev_handle, bool *return_flag )
{
  int32_t ret = PCF2131_OK;
  pcf2131_ctrl2_reg_t ctrl2;

  ret |= dev_handle->bus_read (NULL, 0, CTRL2_REG_ADDR, (uint8_t*) &ctrl2,
                               sizeof(pcf2131_ctrl2_reg_t));

  *return_flag = ctrl2.min_sec_flag;

  return ret;
}

int32_t pcf2131_clear_msf_flag ( dev_ctx_t *dev_handle )
{
  int32_t ret = PCF2131_OK;
  pcf2131_ctrl2_reg_t ctrl2;

  ret |= dev_handle->bus_read (NULL, 0, CTRL2_REG_ADDR, (uint8_t*) &ctrl2,
                               sizeof(pcf2131_ctrl2_reg_t));

  ctrl2.min_sec_flag = 0b0;

  ret |= dev_handle->bus_write (NULL, 0, CTRL2_REG_ADDR, (uint8_t*) &ctrl2,
                                sizeof(pcf2131_ctrl2_reg_t));

  return ret;
}

int32_t pcf2131_get_watchdog_flag ( dev_ctx_t *dev_handle, bool *return_flag )
{
  int32_t ret = PCF2131_OK;
  pcf2131_ctrl2_reg_t ctrl2;

  ret |= dev_handle->bus_read (NULL, 0, CTRL2_REG_ADDR, (uint8_t*) &ctrl2,
                               sizeof(pcf2131_ctrl2_reg_t));

  *return_flag = ctrl2.watchdog_flag;

  return ret;
}

int32_t pcf2131_clear_watchdog_flag ( dev_ctx_t *dev_handle )
{
  int32_t ret = PCF2131_OK;
  pcf2131_watchdog_tim_value_reg_t watchdog_timer =
    { 0 };

  // Writing a Zero to the watchdog timer register clears the flag. The flag cannot be cleared by writing a 0 to the flag bit in ctrl2 register
  ret |= dev_handle->bus_write (NULL, 0, WATCHDOG_TIM_VALUE_REG_ADDR, (uint8_t*) &watchdog_timer,
                                sizeof(pcf2131_watchdog_tim_value_reg_t));

  return ret;
}

int32_t pcf2131_get_alarm_flag ( dev_ctx_t *dev_handle, bool *return_flag )
{
  int32_t ret = PCF2131_OK;
  pcf2131_ctrl2_reg_t ctrl2;

  ret |= dev_handle->bus_read (NULL, 0, CTRL2_REG_ADDR, (uint8_t*) &ctrl2,
                               sizeof(pcf2131_ctrl2_reg_t));

  *return_flag = ctrl2.alarm_flag;

  return ret;
}

int32_t pcf2131_clear_alarm_flag ( dev_ctx_t *dev_handle )
{
  int32_t ret = PCF2131_OK;
  pcf2131_ctrl2_reg_t ctrl2;

  ret |= dev_handle->bus_read (NULL, 0, CTRL2_REG_ADDR, (uint8_t*) &ctrl2,
                               sizeof(pcf2131_ctrl2_reg_t));

  ctrl2.alarm_flag = 0b0;

  ret |= dev_handle->bus_write (NULL, 0, CTRL2_REG_ADDR, (uint8_t*) &ctrl2,
                                sizeof(pcf2131_ctrl2_reg_t));

  return ret;
}

int32_t pcf2131_get_battery_status_flag ( dev_ctx_t *dev_handle, bool *return_flag )
{
  int32_t ret = PCF2131_OK;
  pcf2131_ctrl3_reg_t ctrl3;

  ret |= dev_handle->bus_read (NULL, 0, CTRL3_REG_ADDR, (uint8_t*) &ctrl3,
                               sizeof(pcf2131_ctrl3_reg_t));

  *return_flag = ctrl3.batt_stat_flag;

  return ret;
}

int32_t pcf2131_clear_battery_status_flag ( dev_ctx_t *dev_handle )
{
  int32_t ret = PCF2131_OK;
  pcf2131_ctrl3_reg_t ctrl3;

  ret |= dev_handle->bus_read (NULL, 0, CTRL3_REG_ADDR, (uint8_t*) &ctrl3,
                               sizeof(pcf2131_ctrl3_reg_t));

  ctrl3.batt_stat_flag = 0b0;

  ret |= dev_handle->bus_write (NULL, 0, CTRL3_REG_ADDR, (uint8_t*) &ctrl3,
                                sizeof(pcf2131_ctrl3_reg_t));

  return ret;
}

int32_t pcf2131_get_battery_switch_over_flag ( dev_ctx_t *dev_handle, bool *return_flag )
{
  int32_t ret = PCF2131_OK;
  pcf2131_ctrl3_reg_t ctrl3;

  ret |= dev_handle->bus_read (NULL, 0, CTRL3_REG_ADDR, (uint8_t*) &ctrl3,
                               sizeof(pcf2131_ctrl3_reg_t));

  *return_flag = ctrl3.batt_switchover_flag;

  return ret;
}

int32_t pcf2131_clear_battery_switch_over_flag ( dev_ctx_t *dev_handle )
{
  int32_t ret = PCF2131_OK;
  pcf2131_ctrl3_reg_t ctrl3;

  ret |= dev_handle->bus_read (NULL, 0, CTRL3_REG_ADDR, (uint8_t*) &ctrl3,
                               sizeof(pcf2131_ctrl3_reg_t));

  ctrl3.batt_switchover_flag = 0b0;

  ret |= dev_handle->bus_write (NULL, 0, CTRL3_REG_ADDR, (uint8_t*) &ctrl3,
                                sizeof(pcf2131_ctrl3_reg_t));

  return ret;
}

int32_t pcf2131_get_timestamp_flag ( dev_ctx_t *dev_handle, pcf2131_timestamp_t which_timestamp,
bool *return_flag )
{
  int32_t ret = PCF2131_OK;
  pcf2131_ctrl4_reg_t ctrl4;

  ret |= dev_handle->bus_read (NULL, 0, CTRL4_REG_ADDR, (uint8_t*) &ctrl4,
                               sizeof(pcf2131_ctrl4_reg_t));

  *return_flag = *((uint8_t*) &ctrl4) & (1 << (7 - which_timestamp));

  return ret;
}

int32_t pcf2131_clear_timestamp_flag ( dev_ctx_t *dev_handle, pcf2131_timestamp_t which_timestamp )
{
  int32_t ret = PCF2131_OK;
  pcf2131_ctrl4_reg_t ctrl4;
  uint8_t clear_bit = ~(1 << (7 - which_timestamp));

  ret |= dev_handle->bus_read (NULL, 0, CTRL4_REG_ADDR, (uint8_t*) &ctrl4,
                               sizeof(pcf2131_ctrl4_reg_t));

  *((uint8_t*) &ctrl4) = *((uint8_t*) &ctrl4) & clear_bit;

  ret |= dev_handle->bus_write (NULL, 0, CTRL4_REG_ADDR, (uint8_t*) &ctrl4,
                                sizeof(pcf2131_ctrl4_reg_t));

  return ret;
}

int32_t pcf2131_set_temp_meas_period ( dev_ctx_t *dev_handle, temp_meas_period_t meas_period )
{
  int32_t ret = PCF2131_OK;
  pcf2131_clkout_ctrl_reg_t clk_out;

  ret |= dev_handle->bus_read (NULL, 0, CLKOUT_CTRL_REG_ADDR, (uint8_t*) &clk_out,
                               sizeof(pcf2131_clkout_ctrl_reg_t));

  clk_out.temp_meas_p = meas_period;

  ret |= dev_handle->bus_write (NULL, 0, CLKOUT_CTRL_REG_ADDR, (uint8_t*) &clk_out,
                                sizeof(pcf2131_clkout_ctrl_reg_t));

  return ret;
}

int32_t pcf2131_set_clkout_freq ( dev_ctx_t *dev_handle, clock_frequency_t freq_out )
{
  int32_t ret = PCF2131_OK;
  pcf2131_clkout_ctrl_reg_t clk_out;

  ret |= dev_handle->bus_read (NULL, 0, CLKOUT_CTRL_REG_ADDR, (uint8_t*) &clk_out,
                               sizeof(pcf2131_clkout_ctrl_reg_t));

  clk_out.clk_freq = freq_out;

  ret |= dev_handle->bus_write (NULL, 0, CLKOUT_CTRL_REG_ADDR, (uint8_t*) &clk_out,
                                sizeof(pcf2131_clkout_ctrl_reg_t));

  return ret;
}

int32_t pcf2131_perform_otp_refresh ( dev_ctx_t *dev_handle )
{
  int32_t ret = PCF2131_OK;
  pcf2131_clkout_ctrl_reg_t clk_out;

  ret |= dev_handle->bus_read (NULL, 0, CLKOUT_CTRL_REG_ADDR, (uint8_t*) &clk_out,
                               sizeof(pcf2131_clkout_ctrl_reg_t));

  clk_out.otp_refresh = 0b0;

  ret |= dev_handle->bus_write (NULL, 0, CLKOUT_CTRL_REG_ADDR, (uint8_t*) &clk_out,
                                sizeof(pcf2131_clkout_ctrl_reg_t));

  clk_out.otp_refresh = 0b1;

  ret |= dev_handle->bus_write (NULL, 0, CLKOUT_CTRL_REG_ADDR, (uint8_t*) &clk_out,
                                sizeof(pcf2131_clkout_ctrl_reg_t));

  // Datasheet states OTP refresh takes 100ms to complete
  dev_handle->delay (150);

  ret |= dev_handle->bus_read (NULL, 0, CLKOUT_CTRL_REG_ADDR, (uint8_t*) &clk_out,
                               sizeof(pcf2131_clkout_ctrl_reg_t));

  if ( clk_out.otp_refresh != 0b1 )
  {
    ret = PCF2131_ERROR;
  }

  return ret;
}

int32_t pcf2131_set_crystal_aging_offset ( dev_ctx_t *dev_handle, aging_offset_t offset )
{
  int32_t ret = PCF2131_OK;
  pcf2131_aging_offset_reg_t aging;

  ret |= dev_handle->bus_read (NULL, 0, AGING_OFFSET_REG_ADDR, (uint8_t*) &aging,
                               sizeof(pcf2131_aging_offset_reg_t));

  aging.offset = offset;

  ret |= dev_handle->bus_write (NULL, 0, AGING_OFFSET_REG_ADDR, (uint8_t*) &aging,
                                sizeof(pcf2131_aging_offset_reg_t));

  return ret;
}

int32_t pcf2131_config_pwr_mgmt_scheme ( dev_ctx_t *dev_handle, pwr_mgmt_t mgmt_scheme )
{
  int32_t ret = PCF2131_OK;
  pcf2131_ctrl3_reg_t ctrl3;

  ret |= dev_handle->bus_read (NULL, 0, CTRL3_REG_ADDR, (uint8_t*) &ctrl3,
                               sizeof(pcf2131_ctrl3_reg_t));

  ctrl3.pwrmng = mgmt_scheme;

  ret |= dev_handle->bus_write (NULL, 0, CTRL3_REG_ADDR, (uint8_t*) &ctrl3,
                                sizeof(pcf2131_ctrl3_reg_t));

  return ret;
}

int32_t pcf2131_get_osf_flag ( dev_ctx_t *dev_handle, seconds_osf_bit_t *return_flag )
{
  int32_t ret = PCF2131_OK;
  pcf2131_seconds_reg_t seconds;

  ret |= dev_handle->bus_read (NULL, 0, SECONDS_REG_ADDR, (uint8_t*) &seconds,
                               sizeof(pcf2131_seconds_reg_t));

  *return_flag = seconds.osf;

  return ret;
}

int32_t pcf2131_software_reset ( dev_ctx_t *dev_handle )
{
  int32_t ret = PCF2131_OK;
  pcf2131_reset_reg_t reset;

  reset.bit_pattern = SOFTWARE_RESET_BIT_PATTERN;

  ret |= dev_handle->bus_read (NULL, 0, RESET_REG_ADDR, (uint8_t*) &reset,
                               sizeof(pcf2131_reset_reg_t));

  return ret;
}

int32_t pcf2131_clear_prescalar ( dev_ctx_t *dev_handle )
{
  int32_t ret = PCF2131_OK;
  pcf2131_reset_reg_t reset;

  reset.bit_pattern = CLEAR_PRESCALAR_BIT_PATTERN;

  ret |= dev_handle->bus_read (NULL, 0, RESET_REG_ADDR, (uint8_t*) &reset,
                               sizeof(pcf2131_reset_reg_t));

  return ret;
}

int32_t pcf2131_clear_timestamps ( dev_ctx_t *dev_handle )
{
  int32_t ret = PCF2131_OK;
  pcf2131_reset_reg_t reset;

  reset.bit_pattern = CLEAR_TIMESTAMP_BIT_PATTERN;

  ret |= dev_handle->bus_read (NULL, 0, RESET_REG_ADDR, (uint8_t*) &reset,
                               sizeof(pcf2131_reset_reg_t));

  return ret;
}

int32_t pcf2131_clear_prescalar_and_timestamps ( dev_ctx_t *dev_handle )
{
  int32_t ret = PCF2131_OK;
  pcf2131_reset_reg_t reset;

  reset.bit_pattern = CLEAR_PRESCALAR_AND_TIMESTAMP_BIT_PATTERN;

  ret |= dev_handle->bus_read (NULL, 0, RESET_REG_ADDR, (uint8_t*) &reset,
                               sizeof(pcf2131_reset_reg_t));

  return ret;
}

int32_t pcf2131_watchdog_config_time_source ( dev_ctx_t *dev_handle,
                                              watchdog_time_source_t time_source )
{
  int32_t ret = PCF2131_OK;
  pcf2131_watchdog_tim_ctrl_reg_t watchdog;

  ret |= dev_handle->bus_read (NULL, 0, WATCHDOG_TIM_CTRL_REG_ADDR, (uint8_t*) &watchdog,
                               sizeof(pcf2131_watchdog_tim_ctrl_reg_t));

  watchdog.clock_source = time_source;

  ret |= dev_handle->bus_write (NULL, 0, WATCHDOG_TIM_CTRL_REG_ADDR, (uint8_t*) &watchdog,
                                sizeof(pcf2131_watchdog_tim_ctrl_reg_t));

  return ret;
}

int32_t pcf2131_set_watchdog_timer_value ( dev_ctx_t *dev_handle, uint8_t timer_value )
{
  int32_t ret = PCF2131_OK;
  pcf2131_watchdog_tim_value_reg_t watchdog =
    { timer_value };

  ret |= dev_handle->bus_write (NULL, 0, WATCHDOG_TIM_VALUE_REG_ADDR, (uint8_t*) &watchdog,
                                sizeof(pcf2131_watchdog_tim_value_reg_t));

  return ret;
}

static uint8_t __weekday_from_date ( int y, int m, int d )
{
  y += 2000;
  /* wikipedia.org/wiki/Determination_of_the_day_of_the_week#Implementation-dependent_methods */
  uint8_t weekday = (uint8_t) ((d += m < 3 ?
      y-- : y - 2, 23 * m / 9 + d + 4 + y / 4 - y / 100 + y / 400)
                               % 7);

  return weekday;
}

static bcd_struct_t __dec_to_bcd_struct ( uint8_t decimal_val )
{
  uint8_t bcd_full = DEC_TO_BCD(decimal_val)
  ;
  bcd_struct_t ret =
    { ((bcd_full & 0xF0) >> 4), bcd_full & 0x0F };
  return ret;
}
