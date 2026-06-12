/*
 * ext_psram.h
 *
 *  Created on: Dec 12, 2024
 *      Author: philbush
 */

#ifndef COMPONENTS_INC_EXT_PSRAM_H_
#define COMPONENTS_INC_EXT_PSRAM_H_

#include "stdbool.h"

// @formatter:off
#define DUMMY_CYCLES_FAST_READ_SPI_SINGLE   8
#define DUMMY_CYCLES_FAST_READ_SPI_QUAD     6
#define DUMMY_CYCLES_FAST_READ_QUAD         4
#define DUMMY_CYCLES_FASTEST_QUAD           6
#define DUMMY_CYCLES_WRITE                  0

typedef enum
{
  PSRAM_READ                    = 0X03,
  PSRAM_FAST_READ               = 0X0B,
  PSRAM_FAST_READ_QUAD          = 0XEB,
  PSRAM_WRITE                   = 0X02,
  PSRAM_WRITE_QUAD              = 0X38,
  PSRAM_ENTER_QUAD_MODE         = 0X35,
  PSRAM_EXIT_QUAD_MODE          = 0XF5,
  PSRAM_RESET_ENABLE            = 0X66,
  PSRAM_RESET                   = 0X99,
  PSRAM_WRAP_BOUNDARY_TOGGLE    = 0XC0,
  PSRAM_READ_ID                 = 0X9F
} APS6404L_command_t;

bool initialize_psram(void);

// @formatter:on
#endif /* COMPONENTS_INC_EXT_PSRAM_H_ */
