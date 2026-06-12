/*
 * vcnl4010_reg.c
 *
 *  Created on: Nov 5, 2024
 *      Author: philbush
 */

#include "vcnl4010_reg.h"

int32_t vcnl4010_register_io_functions ( dev_ctx_t *dev_handle, dev_init_ptr init_fn,
                                         dev_deinit_ptr deinit_fn, dev_write_ptr bus_write_fn,
                                         dev_read_ptr bus_read_fn, dev_ms_delay_ptr delay_fn )
{
  dev_handle->init = init_fn;
  dev_handle->deinit = deinit_fn;
  dev_handle->bus_read = bus_read_fn;
  dev_handle->bus_write = bus_write_fn;
  dev_handle->handle = (void*) NULL;
  dev_handle->delay = delay_fn;

  return (dev_handle->init != NULL) ?
      dev_handle->init () : VCNL4010_OK;
}

int32_t vcnl4010_get_id ( dev_ctx_t *dev_handle, uint8_t *id )
{
  return dev_handle->bus_read (NULL, VCNL4010_I2C_ADDR, PROD_ID_REV_REG_ADDR, id, 1);
}

int32_t vcnl4010_start_ambient_conversion ( dev_ctx_t *dev_handle )
{
  int32_t ret = VCNL4010_OK;
  vcnl4010_command_0_reg_t cmd0;

  ret |= dev_handle->bus_read (NULL, VCNL4010_I2C_ADDR, COMMAND_0_REG_ADDR, (uint8_t*) &cmd0, 1);

  cmd0.als_od = 0b1;

  ret |= dev_handle->bus_write (NULL, VCNL4010_I2C_ADDR, COMMAND_0_REG_ADDR, (uint8_t*) &cmd0, 1);

  return ret;
}

int32_t vcnl4010_start_prox_conversion ( dev_ctx_t *dev_handle )
{
  int32_t ret = VCNL4010_OK;
  vcnl4010_command_0_reg_t cmd0;

  ret |= dev_handle->bus_read (NULL, VCNL4010_I2C_ADDR, COMMAND_0_REG_ADDR, (uint8_t*) &cmd0, 1);

  cmd0.prox_od = 0b1;

  ret |= dev_handle->bus_write (NULL, VCNL4010_I2C_ADDR, COMMAND_0_REG_ADDR, (uint8_t*) &cmd0, 1);

  return ret;
}

int32_t vcnl4010_set_led_current ( dev_ctx_t *dev_handle, vcnl_led_current_t current )
{
  int32_t ret = VCNL4010_OK;
  vcnl4010_ir_led_current_reg_t current_reg;

  ret |= dev_handle->bus_read (NULL, VCNL4010_I2C_ADDR, IR_LED_CURRENT_REG_ADDR,
                               (uint8_t*) &current_reg, 1);

  current_reg.current_value = current;

  ret |= dev_handle->bus_write (NULL, VCNL4010_I2C_ADDR, IR_LED_CURRENT_REG_ADDR,
                                (uint8_t*) &current_reg, 1);

  return ret;
}

int32_t vcnl4010_get_ambient_data_ready ( dev_ctx_t *dev_handle, bool *ready )
{
  int32_t ret = VCNL4010_OK;
  vcnl4010_command_0_reg_t cmd0;

  ret |= dev_handle->bus_read (NULL, VCNL4010_I2C_ADDR, COMMAND_0_REG_ADDR, (uint8_t*) &cmd0, 1);

  *ready = cmd0.als_data_rdy;

  return ret;
}

int32_t vcnl4010_get_prox_data_ready ( dev_ctx_t *dev_handle, bool *ready )
{
  int32_t ret = VCNL4010_OK;
  vcnl4010_command_0_reg_t cmd0;

  ret |= dev_handle->bus_read (NULL, VCNL4010_I2C_ADDR, COMMAND_0_REG_ADDR, (uint8_t*) &cmd0, 1);

  *ready = cmd0.prox_data_rdy;

  return ret;
}

int32_t vcnl4010_get_ambient_reading ( dev_ctx_t *dev_handle, uint16_t *ambient )
{
  int32_t ret = VCNL4010_OK;

  ret |= dev_handle->bus_read (NULL, VCNL4010_I2C_ADDR, AMB_LIGHT_RSLT_HIGH_BYTE_REG_ADDR,
                               (uint8_t*) ambient, 2);

  return ret;
}

int32_t vcnl4010_get_proximity_reading ( dev_ctx_t *dev_handle, uint16_t *proximity )
{
  int32_t ret = VCNL4010_OK;

  ret |= dev_handle->bus_read (NULL, VCNL4010_I2C_ADDR, PROXIMITY_RSLT_HIGH_BYTE_REG_ADDR,
                               (uint8_t*) proximity, 2);

  return ret;
}

int32_t vcnl4010_cont_conv_config ( dev_ctx_t *dev_handle, bool enable )
{
  int32_t ret = VCNL4010_OK;
  vcnl4010_amb_light_param_reg_t amb_light_param;

  ret |= dev_handle->bus_read (NULL, VCNL4010_I2C_ADDR, AMB_LIGHT_PARAM_REG_ADDR,
                               (uint8_t*) &amb_light_param, 1);

  amb_light_param.cont_conv_mode = enable;

  ret |= dev_handle->bus_write (NULL, VCNL4010_I2C_ADDR, AMB_LIGHT_PARAM_REG_ADDR,
                                (uint8_t*) &amb_light_param, 1);

  return ret;
}

int32_t vcnl4010_get_cont_conv ( dev_ctx_t *dev_handle, bool *enabled )
{
  int32_t ret = VCNL4010_OK;
  vcnl4010_amb_light_param_reg_t amb_light_param;

  ret |= dev_handle->bus_read (NULL, VCNL4010_I2C_ADDR, AMB_LIGHT_PARAM_REG_ADDR,
                               (uint8_t*) &amb_light_param, 1);

  *enabled = amb_light_param.cont_conv_mode;

  return ret;
}

int32_t vcnl4010_auto_offset_comp_config ( dev_ctx_t *dev_handle, bool enable )
{
  int32_t ret = VCNL4010_OK;
  vcnl4010_amb_light_param_reg_t amb_light_param;

  ret |= dev_handle->bus_read (NULL, VCNL4010_I2C_ADDR, AMB_LIGHT_PARAM_REG_ADDR,
                               (uint8_t*) &amb_light_param, 1);

  amb_light_param.auto_offset_comp = enable;

  ret |= dev_handle->bus_write (NULL, VCNL4010_I2C_ADDR, AMB_LIGHT_PARAM_REG_ADDR,
                                (uint8_t*) &amb_light_param, 1);

  return ret;
}

int32_t vcnl4010_get_auto_offset_comp ( dev_ctx_t *dev_handle, bool *enabled )
{
  int32_t ret = VCNL4010_OK;
  vcnl4010_amb_light_param_reg_t amb_light_param;

  ret |= dev_handle->bus_read (NULL, VCNL4010_I2C_ADDR, AMB_LIGHT_PARAM_REG_ADDR,
                               (uint8_t*) &amb_light_param, 1);

  *enabled = amb_light_param.auto_offset_comp;

  return ret;
}

int32_t vcnl4010_set_proximity_frequency ( dev_ctx_t *dev_handle, vcnl4010_prox_freq_t prox_freq )
{
  int32_t ret = VCNL4010_OK;
  vcnl4010_prox_mod_timing_reg_t prox_mod;

  ret |= dev_handle->bus_read (NULL, VCNL4010_I2C_ADDR, PROX_MOD_TIMING_REG_ADDR,
                               (uint8_t*) &prox_mod, 1);

  prox_mod.prox_freq = prox_freq;

  ret |= dev_handle->bus_write (NULL, VCNL4010_I2C_ADDR, PROX_MOD_TIMING_REG_ADDR,
                                (uint8_t*) &prox_mod, 1);
  return ret;
}
