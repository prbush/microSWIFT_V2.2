/*
 * rf_switch.c
 *
 *  Created on: May 10, 2023
 *      Author: Phil
 */

#include "rf_switch.h"
#include "logger.h"
// Internal object pointer
static RF_Switch *rf_self;
// Object functions
//static void rf_switch_power_on ( void );
//static void rf_switch_power_off ( void );
static void rf_switch_set_gnss_port ( void );
static void rf_switch_set_iridium_port ( void );

/**
 * Initialize the RF switch struct. RF switch will initialize to the GNSS port.
 *
 * @return gnss_error_code_t
 */
void rf_switch_init ( RF_Switch *struct_ptr )
{
  // Assign object pointer
  rf_self = struct_ptr;

//  rf_self->en_gpio_group = RF_SWITCH_EN_GPIO_Port;
  rf_self->vctl_gpio_group = RF_SWITCH_VCTL_GPIO_Port;
//  rf_self->en_gpio_pin = RF_SWITCH_EN_Pin;
  rf_self->vctl_gpio_pin = RF_SWITCH_VCTL_Pin;

//  rf_self->power_on = rf_switch_power_on;
//  rf_self->power_off = rf_switch_power_off;
  rf_self->set_gnss_port = rf_switch_set_gnss_port;
  rf_self->set_iridium_port = rf_switch_set_iridium_port;
}

///**
// * @brief	Turn on the rf switch FET
// *
// * @return 	void
// */
//static void rf_switch_power_on ( void )
//{
//  rf_self->en_pin_current_state = GPIO_PIN_SET;
//
//  HAL_GPIO_WritePin (rf_self->en_gpio_group, rf_self->en_gpio_pin, rf_self->en_pin_current_state);
//}
//
///**
// * @brief	Turn off the rf switch FET
// *
// * @return 	void
// */
//static void rf_switch_power_off ( void )
//{
//  rf_self->en_pin_current_state = GPIO_PIN_RESET;
//
//  HAL_GPIO_WritePin (rf_self->en_gpio_group, rf_self->en_gpio_pin, rf_self->en_pin_current_state);
//}

/**
 * @brief	Command the RF switch to RF1 (GNSS) port
 *
 * @return 	void
 */
static void rf_switch_set_gnss_port ( void )
{
  rf_self->vctl_pin_current_state = GPIO_PIN_SET;

  HAL_GPIO_WritePin (rf_self->vctl_gpio_group, rf_self->vctl_gpio_pin,
                     rf_self->vctl_pin_current_state);
  tx_thread_sleep (1);

  LOG("RF switch set to GNSS port.");

  rf_self->current_port = RF_GNSS_PORT;
}

/**
 * @brief	Command the RF switch to RF2 (Iridium) port
 *
 * @return 	void
 */
static void rf_switch_set_iridium_port ( void )
{
  rf_self->vctl_pin_current_state = GPIO_PIN_RESET;

  HAL_GPIO_WritePin (rf_self->vctl_gpio_group, rf_self->vctl_gpio_pin,
                     rf_self->vctl_pin_current_state);
  tx_thread_sleep (1);

  LOG("RF switch set to Iridium port.");

  rf_self->current_port = RF_IRIDIUM_PORT;
}

