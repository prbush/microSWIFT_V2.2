/*
 * testing_hooks.h
 *
 *  Created on: Aug 1, 2024
 *      Author: philbush
 */

#ifndef COMPONENTS_INC_TESTING_HOOKS_H_
#define COMPONENTS_INC_TESTING_HOOKS_H_

#include "stdbool.h"

typedef bool (*testing_funct) ( void* );

typedef struct
{
  testing_funct main_test;
  testing_funct threadx_init_test;
  testing_funct control_test;
  testing_funct gnss_thread_test;
  testing_funct ct_thread_test;
  testing_funct temperature_thread_test;
  testing_funct light_thread_test;
  testing_funct turbidity_thread_test;
  testing_funct waves_thread_test;
  testing_funct iridium_thread_test;
  testing_funct filex_test;
  testing_funct shutdown_test;
} testing_hooks;

extern testing_hooks tests;

void tests_init ( void );

#endif /* COMPONENTS_INC_TESTING_HOOKS_H_ */
