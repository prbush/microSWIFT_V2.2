/*
 * error_handler.h
 *
 *  Created on: Aug 22, 2024
 *      Author: philbush
 */

#ifndef INC_ERROR_HANDLER_H_
#define INC_ERROR_HANDLER_H_

#include "stdbool.h"
#include "tx_api.h"

extern int skip_safe_mode;

void Error_Handler ( void );
void safe_mode ( void );

#endif /* INC_ERROR_HANDLER_H_ */
