/*
 * leds.h
 *
 *  Created on: Dec 13, 2024
 *      Author: philbush
 */
// clang-format off

#ifndef COMPONENTS_INC_LEDS_H_
#define COMPONENTS_INC_LEDS_H_

#include "stdbool.h"
#include "gpio.h"
#include "tx_api.h"

#define HEARTBEAT_ON_TICKS 25U
#define HEARTBEAT_OFF_TICKS (TX_TIMER_TICKS_PER_SECOND - HEARTBEAT_ON_TICKS)
#define INITIAL_SEQUENCE_DELAY_TICKS (TX_TIMER_TICKS_PER_SECOND / 4U)
#define PASSED_SEQUENCE_DELAY_TICKS (TX_TIMER_TICKS_PER_SECOND)
#define FAILED_SEQUENCE_DELAY_TICKS (TX_TIMER_TICKS_PER_SECOND / 2U)

#define LED_SEQUENCE_FOREVER 0xFFFFFFFF

#define LED_MESSAGE_QUEUE_LENGTH 4

//@formatter:off
typedef enum led_sequence
{
  // Red on, green on, red off, green off (full cycle is 1 Hz)
  INITIAL_LED_SEQUENCE      = 1,
  // Green off/on (full cycle is 0.5 Hz; equal time)
  TEST_PASSED_LED_SEQUENCE  = 2,
  // Red off/on (full cycle is 1 Hz)
  TEST_FAILED_LED_SEQUENCE  = 3,
  // Green off/on (full cycle is 1 Hz; on for 25% of that)
  HEARTBEAT_SEQUENCE        = 4,
  LIGHTS_OFF                = 5,
  LIGHTS_ON                 = 6
} led_sequence_t;

typedef struct
{
  led_sequence_t    sequence;
  uint32_t          duration_sec;
} led_message;

typedef struct
{
  gpio_pin_struct   red_led;
  gpio_pin_struct   green_led;
  TX_TIMER          *duration_timer;
  TX_QUEUE          *led_queue;

  bool              stop;
  bool              timer_timeout;

  led_sequence_t    current_sequence;

  void              (*play_sequence) ( led_sequence_t sequence, uint32_t duration );
} LEDs;

void leds_init ( LEDs *struct_ptr, TX_TIMER *led_durartion_timer, TX_QUEUE *led_queue );
void leds_stop (void);
void led_timer_expired ( ULONG expiration_input );
void led_light_sequence ( led_sequence_t sequence, uint32_t duration );
//@formatter:on
#endif /* COMPONENTS_INC_LEDS_H_ */
