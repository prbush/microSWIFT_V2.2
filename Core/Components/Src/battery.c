/*
 * Battery.cpp
 *
 *  Created on: Oct 28, 2022
 *      Author: Phil
 */

#include "battery.h"
#include "adc.h"
#include "stdbool.h"
// Object intance pointer
static Battery *battery_self;

static uSWIFT_return_code_t _battery_get_voltage ( real16_T *voltage );
static uSWIFT_return_code_t __battery_start_conversion ( void );

static bool adc_init_status = false;

void battery_init ( Battery *struct_ptr, ADC_HandleTypeDef *adc_handle,
                    TX_EVENT_FLAGS_GROUP *irq_flags )
{
  battery_self = struct_ptr;
  battery_self->adc_handle = adc_handle;
  battery_self->irq_flags = irq_flags;
  battery_self->voltage = 0;

  battery_self->get_voltage = _battery_get_voltage;
}

void battery_deinit ( void )
{
  if ( adc_init_status )
  {
    (void) HAL_ADC_Stop_IT (battery_self->adc_handle);
    (void) HAL_ADC_DeInit (battery_self->adc_handle);

    (void) HAL_SYSCFG_DisableVREFBUF ();

    (void) __HAL_RCC_VREF_CLK_DISABLE();

    (void) HAL_PWREx_DisableVddA ();

    adc_init_status = false;
  }
}

static uSWIFT_return_code_t _battery_get_voltage ( real16_T *voltage )
{
  uint32_t max_ticks_to_get_voltage = TX_TIMER_TICKS_PER_SECOND;
  ULONG actual_flags;
  uSWIFT_return_code_t ret;

  ret = __battery_start_conversion ();
  if ( ret != uSWIFT_SUCCESS )
  {
    voltage->bitPattern = BATTERY_ERROR_VOLTAGE_VALUE;
    return ret;
  }

  // If it either takes too long or an EDC error is detected, set the battery voltage to the error value
  if ( (tx_event_flags_get (battery_self->irq_flags, BATTERY_CONVERSION_COMPLETE, TX_OR_CLEAR,
                            &actual_flags, max_ticks_to_get_voltage)
        != TX_SUCCESS) )
  {
    voltage->bitPattern = BATTERY_ERROR_VOLTAGE_VALUE;
    return uSWIFT_TIMEOUT;
  }

  *voltage = floatToHalf (battery_self->voltage);
  return uSWIFT_SUCCESS;
}

static uSWIFT_return_code_t __battery_start_conversion ( void )
{
  uSWIFT_return_code_t return_code;

  if ( !adc_init_status )
  {
    if ( !adc1_init () )
    {
      return uSWIFT_INITIALIZATION_ERROR;
    }

    adc_init_status = true;
  }

  /** Enable the VREF clock
   */
  __HAL_RCC_VREF_CLK_ENABLE();

  /** Configure the internal voltage reference buffer voltage scale
   */
  HAL_SYSCFG_VREFBUF_VoltageScalingConfig (SYSCFG_VREFBUF_VOLTAGE_SCALE2);

  /** Enable the Internal Voltage Reference buffer
   */
  HAL_SYSCFG_EnableVREFBUF ();

  /** Configure the internal voltage reference buffer high impedance mode
   */
  HAL_SYSCFG_VREFBUF_HighImpedanceConfig (SYSCFG_VREFBUF_HIGH_IMPEDANCE_ENABLE);

  tx_thread_sleep (1);

  if ( HAL_ADCEx_Calibration_Start (battery_self->adc_handle, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED)
       != HAL_OK )
  {
    return_code = uSWIFT_CALIBRATION_ERROR;
    return return_code;
  }
  // Need at least a 4 ADC clock cycle delay after calibration before doing anything else with the
  // ADC -- 10ms ought to do.
  tx_thread_sleep (TX_TIMER_TICKS_PER_SECOND / 10);
  battery_self->calibration_offset = HAL_ADCEx_Calibration_GetValue (battery_self->adc_handle,
  ADC_SINGLE_ENDED);

  if ( HAL_ADC_Start_IT (battery_self->adc_handle) != HAL_OK )
  {
    return_code = uSWIFT_INITIALIZATION_ERROR;
    return return_code;
  }

  return_code = uSWIFT_SUCCESS;
  return return_code;
}

void battery_set_voltage ( uint32_t adc_raw_counts )
{
  battery_self->voltage = ((float) ((adc_raw_counts * ADC_SLOPE) + ADC_OFFSET));
}

