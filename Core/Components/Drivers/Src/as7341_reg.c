/*
 * as7341_reg.c
 *
 *  Created on: Oct 24, 2024
 *      Author: philbush
 */

#include "as7341_reg.h"
#include "stdbool.h"
#include "stdint.h"

int32_t as7341_register_io_functions(dev_ctx_t *dev_handle,
                                     dev_init_ptr init_fn,
                                     dev_deinit_ptr deinit_fn,
                                     dev_write_ptr bus_write_fn,
                                     dev_read_ptr bus_read_fn,
                                     dev_ms_delay_ptr delay) {
  dev_handle->init = init_fn;
  dev_handle->deinit = deinit_fn;
  dev_handle->bus_read = bus_read_fn;
  dev_handle->bus_write = bus_write_fn;
  dev_handle->handle = NULL;
  dev_handle->delay = delay;

  return (dev_handle->init != NULL) ? dev_handle->init() : AS7341_OK;
}

int32_t as7341_set_register_bank(dev_ctx_t *dev_handle,
                                 as7341_reg_bank_t bank) {
  int32_t ret = AS7341_OK;
  as7341_cfg0_reg_t cfg0;

  ret |= dev_handle->bus_read(NULL, AS7341_I2C_ADDR, CFG0_REG_ADDR,
                              (uint8_t *)&cfg0, 1);

  cfg0.reg_bank = bank;

  ret |= dev_handle->bus_write(NULL, AS7341_I2C_ADDR, CFG0_REG_ADDR,
                               (uint8_t *)&cfg0, 1);

  return ret;
}

int32_t as7341_get_id(dev_ctx_t *dev_handle, uint8_t *id) {
  int32_t ret = AS7341_OK;
  as7341_id_reg_t id_reg = {0};

  ret |= dev_handle->bus_read(NULL, AS7341_I2C_ADDR, ID_REG_ADDR,
                              (uint8_t *)&id_reg, 1);

  *id = id_reg.id;

  return ret;
}

int32_t as7341_set_integration_mode(dev_ctx_t *dev_handle,
                                    as7341_int_mode_t mode) {
  int32_t ret = AS7341_OK;
  as7341_config_reg_t config_reg;

  ret |= dev_handle->bus_read(NULL, AS7341_I2C_ADDR, CONFIG_REG_ADDR,
                              (uint8_t *)&config_reg, 1);

  config_reg.int_mode = mode;

  ret |= dev_handle->bus_write(NULL, AS7341_I2C_ADDR, CONFIG_REG_ADDR,
                               (uint8_t *)&config_reg, 1);

  return ret;
}

int32_t as7341_config_smux(dev_ctx_t *dev_handle,
                           as7341_smux_assignment *smux_assignment) {
  int32_t ret = AS7341_OK;
  as7341_smux_memory smux_memory = {SMUX_ASSIGNMENT_DISABLE};
  as7341_smux_assignment_t adc_assignment;
  as7341_smux_channels_t channel_assignment;
  as7341_enable_reg_t enable_reg = {0};
  uint8_t *smux_mem_ptr = (uint8_t *)&smux_memory;
  uint8_t timeout = 100, counter = 0;
  bool success = false;

  for (int i = 0; i < AS7341_NUM_ADCS; i++) {
    adc_assignment = i + 1;
    channel_assignment = smux_assignment->adc_assignments[i];

    switch (channel_assignment) {
    case ADC_DISABLE:
      break;

    case F1:
      smux_memory.addr_0x01.f1_left = adc_assignment;
      smux_memory.addr_0x10.f1_right = adc_assignment;
      break;

    case F2:
      smux_memory.addr_0x05.f2_left = adc_assignment;
      smux_memory.addr_0x0c.f2_right = adc_assignment;
      break;

    case F3:
      smux_memory.addr_0x00.f3_left = adc_assignment;
      smux_memory.addr_0x0f.f3_right = adc_assignment;
      break;

    case F4:
      smux_memory.addr_0x05.f4_left = adc_assignment;
      smux_memory.addr_0x0d.f4_right = adc_assignment;
      break;

    case F5:
      smux_memory.addr_0x06.f5_left = adc_assignment;
      smux_memory.addr_0x09.f5_right = adc_assignment;
      break;

    case F6:
      smux_memory.addr_0x04.f6_left = adc_assignment;
      smux_memory.addr_0x0e.f6_right = adc_assignment;
      break;

    case F7:
      smux_memory.addr_0x07.f7_left = adc_assignment;
      smux_memory.addr_0x0a.f7_right = adc_assignment;
      break;

    case F8:
      smux_memory.addr_0x03.f8_left = adc_assignment;
      smux_memory.addr_0x0e.f8_right = adc_assignment;
      break;

    case CLEAR:
      smux_memory.addr_0x08.clear_left = adc_assignment;
      smux_memory.addr_0x11.clear_right = adc_assignment;
      break;

    case NIR:
      smux_memory.addr_0x13.nir = adc_assignment;
      break;

    case FLICKER:
      smux_memory.addr_0x13.flicker = adc_assignment;
      break;

    case GPIO:
      smux_memory.addr_0x10.ext_gpio = adc_assignment;
      break;

    case EXT_INT:
      smux_memory.addr_0x11.ext_int = adc_assignment;
      break;

    case DARK:
      smux_memory.addr_0x12.dark = adc_assignment;
      break;

    default:
      return AS7341_ERROR;
    }
  }

  // Send SMUX command to write to SMUX memory
  ret |= as7341_send_smux_command(dev_handle, SMUX_WRITE_CONFIG_FROM_RAM);

  ret |= dev_handle->bus_write(NULL, AS7341_I2C_ADDR, SMUX_MEMORY_ADDR_LOW,
                               smux_mem_ptr, SMUX_MEMORY_SIZE);

  // Enable the SMUX
  ret |= as7341_smux_enable(dev_handle);

  // Poll the SMUX enable bit until it clears to indicate command has completed
  while (counter++ < timeout) {
    ret |= dev_handle->bus_read(NULL, AS7341_I2C_ADDR, ENABLE_REG_ADDR,
                                (uint8_t *)&enable_reg, 1);

    if (enable_reg.smuxen == 0) {
      success = true;
      break;
    }
  }

  if ((counter == timeout) && !success) {
    ret = AS7341_ERROR;
  }

  return ret;
}

int32_t as7341_validate_smux_config(dev_ctx_t *dev_handle,
                                    as7341_smux_assignment *smux_assignment,
                                    bool *validated) {
  int32_t ret = AS7341_OK;
  as7341_smux_memory smux_memory = {SMUX_ASSIGNMENT_DISABLE};
  as7341_smux_assignment_t adc_assignment;
  as7341_smux_channels_t channel_assignment;
  uint8_t smux_mem_read[SMUX_MEMORY_SIZE] = {0};
  uint8_t *smux_assignment_ptr = (uint8_t *)&smux_memory;

  for (int i = 0; i < AS7341_NUM_ADCS; i++) {
    adc_assignment = i + 1;
    channel_assignment = smux_assignment->adc_assignments[i];

    switch (channel_assignment) {
    case ADC_DISABLE:
      break;

    case F1:
      smux_memory.addr_0x01.f1_left = adc_assignment;
      smux_memory.addr_0x10.f1_right = adc_assignment;
      break;

    case F2:
      smux_memory.addr_0x05.f2_left = adc_assignment;
      smux_memory.addr_0x0c.f2_right = adc_assignment;
      break;

    case F3:
      smux_memory.addr_0x00.f3_left = adc_assignment;
      smux_memory.addr_0x0f.f3_right = adc_assignment;
      break;

    case F4:
      smux_memory.addr_0x05.f4_left = adc_assignment;
      smux_memory.addr_0x0d.f4_right = adc_assignment;
      break;

    case F5:
      smux_memory.addr_0x06.f5_left = adc_assignment;
      smux_memory.addr_0x09.f5_right = adc_assignment;
      break;

    case F6:
      smux_memory.addr_0x04.f6_left = adc_assignment;
      smux_memory.addr_0x0e.f6_right = adc_assignment;
      break;

    case F7:
      smux_memory.addr_0x07.f7_left = adc_assignment;
      smux_memory.addr_0x0a.f7_right = adc_assignment;
      break;

    case F8:
      smux_memory.addr_0x03.f8_left = adc_assignment;
      smux_memory.addr_0x0e.f8_right = adc_assignment;
      break;

    case CLEAR:
      smux_memory.addr_0x08.clear_left = adc_assignment;
      smux_memory.addr_0x11.clear_right = adc_assignment;
      break;

    case NIR:
      smux_memory.addr_0x13.nir = adc_assignment;
      break;

    case FLICKER:
      smux_memory.addr_0x13.flicker = adc_assignment;
      break;

    case GPIO:
      smux_memory.addr_0x10.ext_gpio = adc_assignment;
      break;

    case EXT_INT:
      smux_memory.addr_0x11.ext_int = adc_assignment;
      break;

    case DARK:
      smux_memory.addr_0x12.dark = adc_assignment;
      break;

    default:
      return AS7341_ERROR;
    }
  }

  ret |= dev_handle->bus_read(NULL, AS7341_I2C_ADDR, SMUX_MEMORY_ADDR_LOW,
                              &(smux_mem_read[0]), SMUX_MEMORY_SIZE);

  *validated = true;

  for (int i = 0; i < SMUX_MEMORY_SIZE; i++, smux_assignment_ptr++) {
    if (*smux_assignment_ptr != smux_mem_read[i]) {
      *validated = false;
      break;
    }
  }

  return ret;
}

int32_t as7341_power(dev_ctx_t *dev_handle, bool on) {
  int32_t ret = AS7341_OK;
  as7341_enable_reg_t enable_reg;

  ret |= dev_handle->bus_read(NULL, AS7341_I2C_ADDR, ENABLE_REG_ADDR,
                              (uint8_t *)&enable_reg, 1);

  enable_reg.pon = on;

  ret |= dev_handle->bus_write(NULL, AS7341_I2C_ADDR, ENABLE_REG_ADDR,
                               (uint8_t *)&enable_reg, 1);

  return ret;
}

int32_t as7341_low_power_config(dev_ctx_t *dev_handle, bool enable) {
  int32_t ret = AS7341_OK;
  as7341_cfg0_reg_t cfg0_reg;

  ret |= dev_handle->bus_read(NULL, AS7341_I2C_ADDR, CFG0_REG_ADDR,
                              (uint8_t *)&cfg0_reg, 1);

  cfg0_reg.low_power = enable;

  ret |= dev_handle->bus_write(NULL, AS7341_I2C_ADDR, CFG0_REG_ADDR,
                               (uint8_t *)&cfg0_reg, 1);

  return ret;
}

int32_t as7341_smux_enable(dev_ctx_t *dev_handle) {
  int32_t ret = AS7341_OK;
  as7341_enable_reg_t enable_reg;

  ret |= dev_handle->bus_read(NULL, AS7341_I2C_ADDR, ENABLE_REG_ADDR,
                              (uint8_t *)&enable_reg, 1);

  enable_reg.smuxen = PROPERTY_ENABLE;

  ret |= dev_handle->bus_write(NULL, AS7341_I2C_ADDR, ENABLE_REG_ADDR,
                               (uint8_t *)&enable_reg, 1);

  return ret;
}

int32_t as7341_send_smux_command(dev_ctx_t *dev_handle, as7341_smux_cmd_t cmd) {
  int32_t ret = AS7341_OK;
  as7341_cfg6_reg_t cfg6;

  ret |= dev_handle->bus_read(NULL, AS7341_I2C_ADDR, CFG6_REG_ADDR,
                              (uint8_t *)&cfg6, 1);

  cfg6.smux_cmd = cmd;

  ret |= dev_handle->bus_write(NULL, AS7341_I2C_ADDR, CFG6_REG_ADDR,
                               (uint8_t *)&cfg6, 1);

  return ret;
}

int32_t as7341_config_smux_interrupt(dev_ctx_t *dev_handle, bool enable) {
  int32_t ret = AS7341_OK;
  as7341_cfg9_reg_t cfg9;

  ret |= dev_handle->bus_read(NULL, AS7341_I2C_ADDR, CFG9_REG_ADDR,
                              (uint8_t *)&cfg9, 1);

  cfg9.sien_smux = enable;

  ret |= dev_handle->bus_write(NULL, AS7341_I2C_ADDR, CFG9_REG_ADDR,
                               (uint8_t *)&cfg9, 1);

  return ret;
}

int32_t as7341_config_sys_interrupts(dev_ctx_t *dev_handle, bool enable) {
  int32_t ret = AS7341_OK;
  as7341_intenab_reg_t inten;

  ret |= dev_handle->bus_read(NULL, AS7341_I2C_ADDR, INTENAB_REG_ADDR,
                              (uint8_t *)&inten, 1);

  inten.sien = enable;

  ret |= dev_handle->bus_write(NULL, AS7341_I2C_ADDR, INTENAB_REG_ADDR,
                               (uint8_t *)&inten, 1);

  return ret;
}

int32_t as7341_wait_config(dev_ctx_t *dev_handle, bool enable) {
  int32_t ret = AS7341_OK;
  as7341_enable_reg_t enable_reg;

  ret |= dev_handle->bus_read(NULL, AS7341_I2C_ADDR, ENABLE_REG_ADDR,
                              (uint8_t *)&enable_reg, 1);

  enable_reg.wen = enable;

  ret |= dev_handle->bus_write(NULL, AS7341_I2C_ADDR, ENABLE_REG_ADDR,
                               (uint8_t *)&enable_reg, 1);

  return ret;
}

int32_t as7341_spectral_meas_config(dev_ctx_t *dev_handle, bool enable) {
  int32_t ret = AS7341_OK;
  as7341_enable_reg_t enable_reg;

  ret |= dev_handle->bus_read(NULL, AS7341_I2C_ADDR, ENABLE_REG_ADDR,
                              (uint8_t *)&enable_reg, 1);

  enable_reg.sp_en = enable;

  ret |= dev_handle->bus_write(NULL, AS7341_I2C_ADDR, ENABLE_REG_ADDR,
                               (uint8_t *)&enable_reg, 1);

  return ret;
}

int32_t as7341_set_wait_time(dev_ctx_t *dev_handle, float wait_time_ms) {
  int32_t ret = AS7341_OK;
  as7341_wtime_reg_t wait_time;

  if (wait_time_ms < WTIME_MIN_VAL) {
    wait_time.wtime = 0;
  } else if (wait_time_ms > WTIME_MAX_VAL) {
    wait_time.wtime = 255;
  } else {
    wait_time.wtime = WTIME_FROM_MS(wait_time_ms);
  }

  ret |= dev_handle->bus_write(NULL, AS7341_I2C_ADDR, WTIME_REG_ADDR,
                               (uint8_t *)&wait_time, 1);

  return ret;
}

int32_t as7341_set_atime(dev_ctx_t *dev_handle, uint8_t atime) {
  int32_t ret = AS7341_OK;
  as7341_atime_reg_t time;

  time.atime = atime;

  ret |= dev_handle->bus_write(NULL, AS7341_I2C_ADDR, ATIME_REG_ADDR,
                               (uint8_t *)&time, 1);

  return ret;
}

int32_t as7341_set_astep(dev_ctx_t *dev_handle, uint16_t astep) {
  int32_t ret = AS7341_OK;
  as7341_astep_t step;

  step.astep = astep;

  ret |= dev_handle->bus_write(NULL, AS7341_I2C_ADDR, ASTEP_LOWER_REG_ADDR,
                               (uint8_t *)&step.astep_struct.astep_lower, 1);
  ret |= dev_handle->bus_write(NULL, AS7341_I2C_ADDR, ASTEP_UPPER_REG_ADDR,
                               (uint8_t *)&step.astep_struct.astep_upper, 1);

  return ret;
}

int32_t as7341_set_again(dev_ctx_t *dev_handle, as7341_again_t gain_setting) {
  int32_t ret = AS7341_OK;
  as7341_cfg1_reg_t cfg1;

  ret |= dev_handle->bus_read(NULL, AS7341_I2C_ADDR, CFG1_REG_ADDR,
                              (uint8_t *)&cfg1, 1);

  cfg1.again = gain_setting;

  ret |= dev_handle->bus_write(NULL, AS7341_I2C_ADDR, CFG1_REG_ADDR,
                               (uint8_t *)&cfg1, 1);

  return ret;
}

int32_t
as7341_get_all_channel_data(dev_ctx_t *dev_handle,
                            as7341_all_channel_data_struct *channel_data) {
  int32_t ret = AS7341_OK;

  ret |= dev_handle->bus_read(NULL, AS7341_I2C_ADDR, CH0_LOWER_REG_ADDR,
                              (uint8_t *)channel_data,
                              sizeof(as7341_all_channel_data_struct));

  return ret;
}

int32_t as7341_set_gpio_behaviour(dev_ctx_t *dev_handle,
                                  as7341_gpio_behavior_t behavior) {
  as7341_gpio_2_reg_t gpio_2 = {0};

  switch (behavior) {
  case GPIO_INPUT:
    gpio_2.gpio_in = 0b1;
    break;

  case GPIO_OUTPUT_NORMAL:
    gpio_2.gpio_out = 0b1;
    break;

  case GPIO_OUTPUT_INVERTED:
    gpio_2.gpio_out = 0b1;
    gpio_2.gpio_inv = 0b1;
    break;

  default:
    return AS7341_ERROR;
  }

  return dev_handle->bus_write(NULL, AS7341_I2C_ADDR, GPIO_2_REG_ADDR,
                               (uint8_t *)&gpio_2, 1);
}

int32_t as7341_get_initialization_status(dev_ctx_t *dev_handle,
                                         bool *device_is_initialized) {
  int32_t ret = AS7341_OK;
  as7341_status_6_reg_t status_6;

  ret |= dev_handle->bus_read(NULL, AS7341_I2C_ADDR, STATUS_6_REG_ADDR,
                              (uint8_t *)&status_6, 1);

  *device_is_initialized = !status_6.int_busy;

  return ret;
}

int32_t as7341_sleep_after_int_config(dev_ctx_t *dev_handle, bool enable) {
  int32_t ret = AS7341_OK;
  as7341_status_6_reg_t status_6;

  ret |= dev_handle->bus_read(NULL, AS7341_I2C_ADDR, STATUS_6_REG_ADDR,
                              (uint8_t *)&status_6, 1);

  status_6.sai_active = enable;

  ret |= dev_handle->bus_write(NULL, AS7341_I2C_ADDR, STATUS_6_REG_ADDR,
                               (uint8_t *)&status_6, 1);

  return ret;
}

int32_t as7341_int_sync_config(dev_ctx_t *dev_handle, bool enable) {
  int32_t ret = AS7341_OK;
  as7341_config_reg_t config;

  ret |= dev_handle->bus_read(NULL, AS7341_I2C_ADDR, CONFIG_REG_ADDR,
                              (uint8_t *)&config, 1);

  config.int_sel = enable;

  ret |= dev_handle->bus_write(NULL, AS7341_I2C_ADDR, CONFIG_REG_ADDR,
                               (uint8_t *)&config, 1);

  return ret;
}

int32_t as7341_get_data_ready(dev_ctx_t *dev_handle, bool *ready) {
  int32_t ret = AS7341_OK;
  as7341_status_2_reg_t status_2;

  ret |= dev_handle->bus_read(NULL, AS7341_I2C_ADDR, STATUS_2_REG_ADDR,
                              (uint8_t *)&status_2, 1);

  *ready = status_2.avalid;

  return ret;
}

int32_t as7341_auto_zero_config(dev_ctx_t *dev_handle,
                                as7341_az_iter_t az_periodicity) {
  int32_t ret = AS7341_OK;
  as7341_az_config_reg_t auto_zero;

  ret |= dev_handle->bus_read(NULL, AS7341_I2C_ADDR, AZ_CONFIG_REG_ADDR,
                              (uint8_t *)&auto_zero, 1);

  auto_zero.az_nth_iteration.nth_iteration = az_periodicity.nth_iteration;

  ret |= dev_handle->bus_write(NULL, AS7341_I2C_ADDR, AZ_CONFIG_REG_ADDR,
                               (uint8_t *)&auto_zero, 1);

  return ret;
}
