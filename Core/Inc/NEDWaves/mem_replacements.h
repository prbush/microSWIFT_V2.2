/*
 * mem_replacements.h
 *
 *  Created on: Dec 29, 2022
 *      Author: veteran
 */

#ifndef INC_WAVES_MEM_REPLACEMENTS_H_
#define INC_WAVES_MEM_REPLACEMENTS_H_

#include "stddef.h"
#include "math.h"
#include "tx_api.h"
#include "NEDwaves_memlight_types.h"
#include "NEDwaves_memlight_emxAPI.h"
#include "configuration.h"

typedef struct
{
  TX_BYTE_POOL memory_pool;
  microSWIFT_configuration *configuration;
  emxArray_real32_T *north;
  emxArray_real32_T *east;
  emxArray_real32_T *down;
  bool init_success;
} NEDWaves_memory;

bool waves_memory_pool_init ( NEDWaves_memory *waves_mem_ptr,
                              microSWIFT_configuration *configuration,
                              VOID *pool_start,
                              size_t pool_size );
bool waves_memory_pool_delete ( void );
bool waves_memory_get_raw_data_pointers ( float **north, float **east, float **down );
void* malloc_replacement ( size_t size );
void* calloc_replacement ( size_t num, size_t size );
void free_replacement ( void *ptr );

// NEDWaves support
emxArray_real32_T* argInit_1xUnbounded_real32_T ( microSWIFT_configuration *config );
double argInit_real_T ( void );

#endif /* INC_WAVES_MEM_REPLACEMENTS_H_ */
