/*
 * Battery.h
 *
 *  Created on: Oct 28, 2022
 *      Author: Phil
 */

#ifndef SRC_BATTERY_H_
#define SRC_BATTERY_H_

#include "stm32u5xx_hal.h"
#include "tx_api.h"
#include "NEDWaves/rtwhalf.h"
#include "app_threadx.h"
#include "microSWIFT_return_codes.h"

#define NUMBER_OF_ADC_SAMPLES 400
#define BATTERY_CALIBRATION_OFFSET 0
#define VSNS_V_PER_V 0.1335f
#define MICROVOLTS_PER_VOLT 1000000
#define BATTERY_ERROR_VOLTAGE_VALUE 0x70E2
// Obtained from multi-point calibration
#define ADC_SLOPE 0.001531f
#define ADC_OFFSET -3.2343f

// @formatter:off

typedef struct Battery
{
  ADC_HandleTypeDef     *adc_handle;
  TX_EVENT_FLAGS_GROUP  *irq_flags;

  float                 voltage;
  uint32_t              calibration_offset;

  uSWIFT_return_code_t  (*get_voltage) ( real16_T *voltage );
} Battery;

void battery_init ( Battery *struct_ptr, ADC_HandleTypeDef *adc_handle,
                    TX_EVENT_FLAGS_GROUP *irq_flags );
void battery_deinit (void);
void battery_set_voltage (uint32_t adc_raw_counts);

// @formatter:on

#endif /* SRC_BATTERY_H_ */
