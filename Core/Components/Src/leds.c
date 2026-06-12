/*
 * leds.c
 *
 *  Created on: Dec 13, 2024
 *      Author: philbush
 */

#include "leds.h"

static LEDs *led_self;

static void __leds_play_sequence ( led_sequence_t sequence, uint32_t duration );

void leds_init ( LEDs *struct_ptr, TX_TIMER *led_duration_timer, TX_QUEUE *led_queue )
{
  led_self = struct_ptr;

  led_self->red_led.pin = LED_RED_Pin;
  led_self->red_led.port = LED_RED_GPIO_Port;
  led_self->green_led.pin = LED_GREEN_Pin;
  led_self->green_led.port = LED_GREEN_GPIO_Port;

  led_self->duration_timer = led_duration_timer;
  led_self->led_queue = led_queue;

  led_self->stop = false;
  led_self->timer_timeout = false;

  led_self->current_sequence = LIGHTS_OFF;

  led_self->play_sequence = __leds_play_sequence;
}

void led_stop ( void )
{
  led_self->stop = true;
}

void led_timer_expired ( ULONG expiration_input )
{
  led_self->timer_timeout = true;
}

void led_light_sequence ( led_sequence_t sequence, uint32_t duration )
{
  led_message leds_msg =
    { sequence, duration };

  (void) tx_queue_send (led_self->led_queue, &leds_msg, TX_NO_WAIT);
}

/**
 * @brief
 *
 * @retval Void
 */
// Input param duration is in secs
static void __leds_play_sequence ( led_sequence_t sequence, uint32_t duration )
{

  if ( (sequence != LIGHTS_OFF) && (sequence != LIGHTS_ON) )
  {
    (void) tx_timer_change (led_self->duration_timer, duration * TX_TIMER_TICKS_PER_SECOND, 0);
    (void) tx_timer_activate (led_self->duration_timer);
  }

  led_self->current_sequence = sequence;

  while ( !(led_self->stop || led_self->timer_timeout) )
  {
    switch ( led_self->current_sequence )
    {
      case INITIAL_LED_SEQUENCE:
        HAL_GPIO_WritePin (led_self->red_led.port, led_self->red_led.pin, GPIO_PIN_SET);
        tx_thread_sleep (INITIAL_SEQUENCE_DELAY_TICKS);
        HAL_GPIO_WritePin (led_self->green_led.port, led_self->green_led.pin, GPIO_PIN_SET);
        tx_thread_sleep (INITIAL_SEQUENCE_DELAY_TICKS);
        HAL_GPIO_WritePin (led_self->red_led.port, led_self->red_led.pin, GPIO_PIN_RESET);
        tx_thread_sleep (INITIAL_SEQUENCE_DELAY_TICKS);
        HAL_GPIO_WritePin (led_self->green_led.port, led_self->green_led.pin, GPIO_PIN_RESET);
        tx_thread_sleep (INITIAL_SEQUENCE_DELAY_TICKS);
        break;

      case TEST_PASSED_LED_SEQUENCE:
        HAL_GPIO_WritePin (led_self->green_led.port, led_self->green_led.pin, GPIO_PIN_RESET);
        tx_thread_sleep (PASSED_SEQUENCE_DELAY_TICKS);
        HAL_GPIO_WritePin (led_self->green_led.port, led_self->green_led.pin, GPIO_PIN_SET);
        tx_thread_sleep (PASSED_SEQUENCE_DELAY_TICKS);
        break;

      case TEST_FAILED_LED_SEQUENCE:
        HAL_GPIO_WritePin (led_self->red_led.port, led_self->red_led.pin, GPIO_PIN_RESET);
        tx_thread_sleep (FAILED_SEQUENCE_DELAY_TICKS);
        HAL_GPIO_WritePin (led_self->red_led.port, led_self->red_led.pin, GPIO_PIN_SET);
        tx_thread_sleep (FAILED_SEQUENCE_DELAY_TICKS);
        break;

      case HEARTBEAT_SEQUENCE:
        HAL_GPIO_WritePin (led_self->green_led.port, led_self->green_led.pin, GPIO_PIN_SET);
        tx_thread_sleep (HEARTBEAT_ON_TICKS);
        HAL_GPIO_WritePin (led_self->green_led.port, led_self->green_led.pin, GPIO_PIN_RESET);
        tx_thread_sleep (HEARTBEAT_OFF_TICKS);
        break;

      case LIGHTS_OFF:
        HAL_GPIO_WritePin (led_self->red_led.port, led_self->red_led.pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin (led_self->green_led.port, led_self->green_led.pin, GPIO_PIN_RESET);
        led_self->stop = true;
        break;

      case LIGHTS_ON:
        HAL_GPIO_WritePin (led_self->red_led.port, led_self->red_led.pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin (led_self->green_led.port, led_self->green_led.pin, GPIO_PIN_SET);
        led_self->stop = true;
        break;

      default:
        break;
    }
  }

  if ( led_self->current_sequence != LIGHTS_ON )
  {
    HAL_GPIO_WritePin (led_self->red_led.port, led_self->red_led.pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin (led_self->green_led.port, led_self->green_led.pin, GPIO_PIN_RESET);
  }

  led_self->stop = false;
  led_self->timer_timeout = false;
}
