/*
 * mem_replacements.c
 *
 *
 */

#include "NEDWaves/mem_replacements.h"
#include "threadx_support.h"
#include "app_threadx.h"

static NEDWaves_memory *waves_mem;

bool waves_memory_pool_init ( NEDWaves_memory *waves_mem_ptr,
                              microSWIFT_configuration *configuration,
                              VOID *pool_start,
                              size_t pool_size )
{
  UINT ret;

  waves_mem = waves_mem_ptr;

  waves_mem->init_success = false;

  waves_mem->configuration = configuration;

  ret = tx_byte_pool_create(&waves_mem->memory_pool, "waves mem pool", pool_start, pool_size);

  if ( ret == TX_SUCCESS )
  {
    waves_mem->north = argInit_1xUnbounded_real32_T (waves_mem->configuration);
    waves_mem->east = argInit_1xUnbounded_real32_T (waves_mem->configuration);
    waves_mem->down = argInit_1xUnbounded_real32_T (waves_mem->configuration);

    if ( (waves_mem->north != NULL) && (waves_mem->east != NULL) && (waves_mem->down != NULL) )
    {
      waves_mem->init_success = true;
    }
  }

  return waves_mem->init_success;
}

bool waves_memory_pool_delete ()
{
  UINT ret = tx_byte_pool_delete (&waves_mem->memory_pool);

  return (ret == TX_SUCCESS);
}

bool waves_memory_get_raw_data_pointers ( float **north, float **east, float **down )
{
  if ( waves_mem != NULL )
  {
    if ( waves_mem->init_success )
    {
      *north = waves_mem->north->data;
      *east = waves_mem->east->data;
      *down = waves_mem->down->data;

      return true;
    }
  }

  *north = NULL;
  *east = NULL;
  *down = NULL;

  return false;
}

void* malloc_replacement ( size_t size )
{
  CHAR *pointer = TX_NULL;
  if ( size > 0 )
  {
    UINT ret = tx_byte_allocate (&waves_mem->memory_pool, (VOID**) &pointer, (ULONG) size,
    TX_NO_WAIT);
    if ( ret != TX_SUCCESS )
    {
      waves_error_out (NED_WAVES_RAN_OUT_OF_MEM, &waves_thread,
                       "NED Waves memory buffer depleted, call to malloc"
                       " returned NULL ptr.");
    }
  }
  return (void*) pointer;
}

void* calloc_replacement ( size_t num, size_t size )
{
  CHAR *pointer = TX_NULL;
  if ( size > 0 )
  {
    UINT ret = tx_byte_allocate (&waves_mem->memory_pool, (VOID**) &pointer, (ULONG) (num * size),
    TX_NO_WAIT);
    if ( ret != TX_SUCCESS )
    {
      waves_error_out (NED_WAVES_RAN_OUT_OF_MEM, &waves_thread,
                       "NED Waves memory buffer depleted, call to calloc"
                       " returned NULL ptr.");
    }

    memset (pointer, 0, (num * size));
  }
  return (void*) pointer;
}

void free_replacement ( VOID *ptr )
{
  tx_byte_release (ptr);
}

emxArray_real32_T* argInit_1xUnbounded_real32_T ( microSWIFT_configuration *config )
{
  emxArray_real32_T *result;
  result = emxCreate_real32_T (1, config->gnss_samples_per_window);
  return result;
}

double argInit_real_T ( void )
{
  return 0.0;
}

