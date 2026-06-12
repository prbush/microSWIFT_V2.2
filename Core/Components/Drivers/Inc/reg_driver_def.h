/*
 * reg_driver_def.h
 *
 *  Created on: Oct 24, 2024
 *      Author: philbush
 */

#include "stdint.h"
#include "stddef.h"

#ifndef COMPONENTS_DRIVERS_INC_REG_DRIVER_DEF_H_
#define COMPONENTS_DRIVERS_INC_REG_DRIVER_DEF_H_

#ifndef DRV_BYTE_ORDER
#ifndef __BYTE_ORDER__

#define DRV_LITTLE_ENDIAN 1234
#define DRV_BIG_ENDIAN    4321

/** if _BYTE_ORDER is not defined, choose the endianness of your architecture
  * by uncommenting the define which fits your platform endianness
  */
//#define DRV_BYTE_ORDER    DRV_BIG_ENDIAN
#define DRV_BYTE_ORDER    DRV_LITTLE_ENDIAN

#else /* defined __BYTE_ORDER__ */

#define DRV_LITTLE_ENDIAN  __ORDER_LITTLE_ENDIAN__
#define DRV_BIG_ENDIAN     __ORDER_BIG_ENDIAN__
#define DRV_BYTE_ORDER     __BYTE_ORDER__

#endif /* __BYTE_ORDER__*/
#endif /* DRV_BYTE_ORDER */

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t bit0 :1;
  uint8_t bit1 :1;
  uint8_t bit2 :1;
  uint8_t bit3 :1;
  uint8_t bit4 :1;
  uint8_t bit5 :1;
  uint8_t bit6 :1;
  uint8_t bit7 :1;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t bit7 :1;
  uint8_t bit6 :1;
  uint8_t bit5 :1;
  uint8_t bit4 :1;
  uint8_t bit3 :1;
  uint8_t bit2 :1;
  uint8_t bit1 :1;
  uint8_t bit0 :1;
#endif /* DRV_BYTE_ORDER */
} bitwise_byte_t;

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t bit0 :1;
  uint8_t bit1 :1;
  uint8_t bit2 :1;
  uint8_t bit3 :1;
  uint8_t bit4 :1;
  uint8_t bit5 :1;
  uint8_t bit6 :1;
  uint8_t bit7 :1;
  uint8_t bit8 :1;
  uint8_t bit9 :1;
  uint8_t bit10 :1;
  uint8_t bit11 :1;
  uint8_t bit12 :1;
  uint8_t bit13 :1;
  uint8_t bit14 :1;
  uint8_t bit15 :1;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t bit15 :1;
  uint8_t bit14 :1;
  uint8_t bit13 :1;
  uint8_t bit12 :1;
  uint8_t bit11 :1;
  uint8_t bit10 :1;
  uint8_t bit9 :1;
  uint8_t bit8 :1;
  uint8_t bit7 :1;
  uint8_t bit6 :1;
  uint8_t bit5 :1;
  uint8_t bit4 :1;
  uint8_t bit3 :1;
  uint8_t bit2 :1;
  uint8_t bit1 :1;
  uint8_t bit0 :1;
#endif /* DRV_BYTE_ORDER */
} bitwise_short_t;

#define PROPERTY_DISABLE                (0U)
#define PROPERTY_ENABLE                 (1U)

/** @addtogroup  Interfaces_Functions
 * @brief       This section provide a set of functions used to read and
 *              write a generic register of the device.
 *              MANDATORY: return 0 -> no Error.
 * @{
 *
 */
/**************************************************************************************************/
/*********************** Required read/ write functions to be implemented *************************/
/**************************************************************************************************/

/* Note these are generic to either I2C or SPI */
typedef int32_t (*dev_init_ptr) ( void );
typedef int32_t (*dev_deinit_ptr) ( void );
// Note the i2c_addr can be NULL if running SPI
typedef int32_t (*dev_write_ptr) ( void *dev_ctx_handle, uint16_t i2c_addr, uint16_t reg_addr,
                                   uint8_t *write_buf, uint16_t buf_size );
// Note the i2c_addr can be NULL if running SPI
typedef int32_t (*dev_read_ptr) ( void *dev_ctx_handle, uint16_t i2c_addr, uint16_t reg_addr,
                                  uint8_t *read_buf, uint16_t buf_size );
typedef void (*dev_ms_delay_ptr) ( uint32_t delay );

/**************************************************************************************************/
/******************************** Basic I/O interface struct **************************************/
/**************************************************************************************************/
typedef struct
{
  /** Component mandatory fields **/
  dev_init_ptr init;
  dev_deinit_ptr deinit;
  dev_write_ptr bus_write;
  dev_read_ptr bus_read;
  dev_ms_delay_ptr delay;
  /** Customizable optional pointer **/
  void *handle;
} dev_ctx_t;

#endif /* COMPONENTS_DRIVERS_INC_REG_DRIVER_DEF_H_ */
