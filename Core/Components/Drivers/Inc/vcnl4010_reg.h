/*
 * vcnl4010_reg.h
 *
 *  Created on: Nov 5, 2024
 *      Author: philbush
 */

#ifndef COMPONENTS_DRIVERS_INC_VCNL4010_REG_H_
#define COMPONENTS_DRIVERS_INC_VCNL4010_REG_H_
//@formatter:off

#include "stdint.h"
#include "stdbool.h"
#include "reg_driver_def.h"

/**************************************************************************************************/
/************************************** Misc Definitions ******************************************/
/**************************************************************************************************/
#define VCNL4010_I2C_ADDR 0b00100110 // 0x13 << 1

/**************************************************************************************************/
/**************************************** Return Codes ********************************************/
/**************************************************************************************************/
#define VCNL4010_OK 0
#define VCNL4010_ERROR -1

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*################################## Register Definitions ########################################*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/

/**************************************************************************************************/
/************************************ Command Register ********************************************/
/**************************************************************************************************/
#define COMMAND_0_REG_ADDR (0x80)
#define COMMAND_0_REG_RESET_VAL (0b00000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t   selftimed_en    :1;
  uint8_t   prox_en         :1;
  uint8_t   als_en          :1;
  uint8_t   prox_od         :1;
  uint8_t   als_od          :1;
  uint8_t   prox_data_rdy   :1;
  uint8_t   als_data_rdy    :1;
  uint8_t   config_lock     :1;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t   config_lock     :1;
  uint8_t   als_data_rdy    :1;
  uint8_t   prox_data_rdy   :1;
  uint8_t   als_od          :1;
  uint8_t   prox_od         :1;
  uint8_t   als_en          :1;
  uint8_t   prox_en         :1;
  uint8_t   selftimed_en    :1;
#endif /* DRV_BYTE_ORDER */
} vcnl4010_command_0_reg_t;

/**************************************************************************************************/
/************************ Product ID Revision Register ********************************************/
/**************************************************************************************************/
#define PROD_ID_REV_REG_ADDR (0x81)
#define PROD_ID_REV_REG_RESET_VAL (0b00100001)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t   revision_id     :4;
  uint8_t   product_id      :4;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t   product_id      :4;
  uint8_t   revision_id     :4;
#endif /* DRV_BYTE_ORDER */
} vcnl4010_prod_id_rev_reg_t;

/**************************************************************************************************/
/*************************** Proximity Rate Register **********************************************/
/**************************************************************************************************/
#define PROXIMITY_RATE_REG_ADDR (0x82)
#define PROXIMITY_RATE_REG_RESET_VAL (0b00000000)

typedef enum
{
  _1_95_MEAS_PER_SEC    = 0,
  _3_90625_MEAS_PER_SEC = 1,
  _7_8125_MEAS_PER_SEC  = 2,
  _16_625_MEAS_PER_SEC  = 3,
  _31_25_MEAS_PER_SEC   = 4,
} vcnl4010_proximity_rate_t;

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  vcnl4010_proximity_rate_t proximity_rate  :3;
  uint8_t                   reserved        :5;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t                   reserved        :5;
  vcnl4010_proximity_rate_t proximity_rate  :3;
#endif /* DRV_BYTE_ORDER */
} vcnl4010_proximity_rate_reg_t;

/**************************************************************************************************/
/******************************** IR LED Current Register *****************************************/
/**************************************************************************************************/
#define IR_LED_CURRENT_REG_ADDR (0x83)
#define IR_LED_CURRENT_REG_RESET_VAL (0b00000000)

typedef enum
{
  _0_MA     = 0,
  _10_MA    = 1,
  _20_MA    = 2,
  _30_MA    = 3,
  _40_MA    = 4,
  _50_MA    = 5,
  _60_MA    = 6,
  _70_MA    = 7,
  _80_MA    = 8,
  _90_MA    = 9,
  _100_MA   = 10,
  _110_MA   = 11,
  _120_MA   = 12,
  _130_MA   = 13,
  _140_MA   = 14,
  _150_MA   = 15,
  _160_MA   = 16,
  _170_MA   = 17,
  _180_MA   = 18,
  _190_MA   = 19,
  _200_MA   = 20,
} vcnl_led_current_t;

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  vcnl_led_current_t        current_value   :7;
  uint8_t                   fuse_prog_id    :1;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t                   fuse_prog_id    :1;
  vcnl_led_current_t        current_value   :7;
#endif /* DRV_BYTE_ORDER */
} vcnl4010_ir_led_current_reg_t;

/**************************************************************************************************/
/**************************** Ambient Light Parameter Register ************************************/
/**************************************************************************************************/
#define AMB_LIGHT_PARAM_REG_ADDR (0x84)
#define AMB_LIGHT_PARAM_REG_RESET_VAL (0b00000000)

typedef enum
{
  _1_CONV       = 0,
  _2_CONV       = 1,
  _4_CONV       = 2,
  _8_CONV       = 3,
  _16_CONV      = 4,
  _32_CONV      = 5,
  _64_CONV      = 6,
  _128_CONV     = 7
} vcnl4010_amb_avg_func_t;

typedef enum
{
  _1_HZ     = 0,
  _2_HZ     = 1,
  _3_HZ     = 2,
  _4_HZ     = 3,
  _5_HZ     = 4,
  _6_HZ     = 5,
  _8_HZ     = 6,
  _10_HZ    = 7
} vcnl4010_amb_light_meas_rate_t;

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  vcnl4010_amb_avg_func_t           averaging_function  :3;
  uint8_t                           auto_offset_comp    :1;
  vcnl4010_amb_light_meas_rate_t    amb_light_meas_rate :3;
  uint8_t                           cont_conv_mode      :1;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t                           cont_conv_mode      :1;
  vcnl4010_amb_light_meas_rate_t    amb_light_meas_rate :3;
  uint8_t                           auto_offset_comp    :1;
  vcnl4010_amb_avg_func_t           averaging_function  :3;
#endif /* DRV_BYTE_ORDER */
} vcnl4010_amb_light_param_reg_t;

/**************************************************************************************************/
/***************************** Ambient Light Result Registers *************************************/
/**************************************************************************************************/
#define AMB_LIGHT_RSLT_HIGH_BYTE_REG_ADDR (0x85)
#define AMB_LIGHT_RSLT_LOW_BYTE_REG_ADDR (0x86)
#define AMB_LIGHT_RSLT_HIGH_BYTE_REG_RESET_VAL (0b00000000)
#define AMB_LIGHT_RSLT_LOW_BYTE_REG_RESET_VAL (0b00000000)

/**************************************************************************************************/
/******************************* Proximity Result Registers ***************************************/
/**************************************************************************************************/
#define PROXIMITY_RSLT_HIGH_BYTE_REG_ADDR (0x87)
#define PROXIMITY_RSLT_LOW_BYTE_REG_ADDR (0x88)
#define PROXIMITY_RSLT_HIGH_BYTE_REG_RESET_VAL (0b00000000)
#define PROXIMITY_RSLT_LOW_BYTE_REG_RESET_VAL (0b00000000)

/**************************************************************************************************/
/******************************* Interrupt Control Register ***************************************/
/**************************************************************************************************/
#define INT_CTRL_REG_ADDR (0x89)
#define INT_CTRL_REG_RESET_VAL (0b00000000)

typedef enum
{
  _1_COUNT      = 0,
  _2_COUNT      = 1,
  _4_COUNT      = 2,
  _8_COUNT      = 3,
  _16_COUNT     = 4,
  _32_COUNT     = 5,
  _64_COUNT     = 6,
  _128_COUNT    = 7
} vcnl4010_int_cnt_exceed_t;

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t                       int_thres_sel   :1;
  uint8_t                       int_thres_en    :1;
  uint8_t                       int_als_rdy_en  :1;
  uint8_t                       int_prox_rdy_en :1;
  uint8_t                       reserved        :1;
  vcnl4010_int_cnt_exceed_t     int_cnt_exceed  :3;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  vcnl4010_int_cnt_exceed_t     int_cnt_exceed  :3;
  uint8_t                       reserved        :1;
  uint8_t                       int_prox_rdy_en :1;
  uint8_t                       int_als_rdy_en  :1;
  uint8_t                       int_thres_en    :1;
  uint8_t                       int_thres_sel   :1;
#endif /* DRV_BYTE_ORDER */
} vcnl4010_int_ctrl_reg_t;

/**************************************************************************************************/
/********************************* Low Threshold Registers ****************************************/
/**************************************************************************************************/
#define LOW_THRESH_HIGH_BYTE_REG_ADDR (0x8A)
#define LOW_THRESH_LOW_BYTE_REG_ADDR (0x8B)
#define LOW_THRESH_HIGH_BYTE_REG_RESET_VAL (0b00000000)
#define LOW_THRESH_LOW_BYTE_REG_RESET_VAL (0b00000000)

/**************************************************************************************************/
/******************************** HIGH Threshold Registers ****************************************/
/**************************************************************************************************/
#define HIGH_THRESH_HIGH_BYTE_REG_ADDR (0x8C)
#define HIGH_THRESH_LOW_BYTE_REG_ADDR (0x8D)
#define HIGH_THRESH_HIGH_BYTE_REG_RESET_VAL (0b00000000)
#define HIGH_THRESH_LOW_BYTE_REG_RESET_VAL (0b00000000)

/**************************************************************************************************/
/******************************** Interrupt Status Register ***************************************/
/**************************************************************************************************/
#define INT_STATUS_REG_ADDR (0x8E)
#define INT_STATUS_REG_RESET_VAL (0b00000000)

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t   int_th_hi   :1;
  uint8_t   int_th_low  :1;
  uint8_t   int_als_rdy :1;
  uint8_t   int_prox_rdy:1;
  uint8_t   reserved    :4;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t   reserved    :4;
  uint8_t   int_prox_rdy:1;
  uint8_t   int_als_rdy :1;
  uint8_t   int_th_low  :1;
  uint8_t   int_th_hi   :1;
#endif /* DRV_BYTE_ORDER */
} vcnl4010_int_status_reg_t;

/**************************************************************************************************/
/********************** Proximity Modulator Timing Adjustment Register ****************************/
/**************************************************************************************************/
#define PROX_MOD_TIMING_REG_ADDR (0x8F)
#define PROX_MOD_TIMING_REG_RESET_VAL (0b00000000)

typedef enum
{
  _390_625_kHz      = 0,
  _781_25_kHz       = 1,
  _1_5625_MHz       = 2,
  _3_125_MHz        = 3
} vcnl4010_prox_freq_t;

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t               mod_dead_time   :3;
  vcnl4010_prox_freq_t  prox_freq       :2;
  uint8_t               mod_delay_time  :3;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t               mod_delay_time  :3;
  vcnl4010_prox_freq_t  prox_freq       :2;
  uint8_t               mod_dead_time   :3;
#endif /* DRV_BYTE_ORDER */
} vcnl4010_prox_mod_timing_reg_t;

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*################################## Function Declarations #######################################*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/

int32_t vcnl4010_register_io_functions      (dev_ctx_t *dev_handle, dev_init_ptr init_fn,
                                             dev_deinit_ptr deinit_fn, dev_write_ptr bus_write_fn,
                                             dev_read_ptr bus_read_fn, dev_ms_delay_ptr delay_fn);
int32_t vcnl4010_get_id                     (dev_ctx_t *dev_handle, uint8_t *id);
int32_t vcnl4010_start_ambient_conversion   (dev_ctx_t *dev_handle);
int32_t vcnl4010_start_prox_conversion      (dev_ctx_t *dev_handle);
int32_t vcnl4010_set_led_current            (dev_ctx_t *dev_handle, vcnl_led_current_t current);
int32_t vcnl4010_get_ambient_data_ready     (dev_ctx_t *dev_handle, bool *ready);
int32_t vcnl4010_get_prox_data_ready        (dev_ctx_t *dev_handle, bool *ready);
int32_t vcnl4010_get_ambient_reading        (dev_ctx_t *dev_handle, uint16_t *ambient);
int32_t vcnl4010_get_proximity_reading      (dev_ctx_t *dev_handle, uint16_t *proximity);
int32_t vcnl4010_cont_conv_config           (dev_ctx_t *dev_handle, bool enable);
int32_t vcnl4010_get_cont_conv              (dev_ctx_t *dev_handle, bool *enabled);
int32_t vcnl4010_auto_offset_comp_config    (dev_ctx_t *dev_handle, bool enable);
int32_t vcnl4010_get_auto_offset_comp       (dev_ctx_t *dev_handle, bool *enabled);
int32_t vcnl4010_set_proximity_frequency    (dev_ctx_t *dev_handle, vcnl4010_prox_freq_t prox_freq);

//@formatter:on
#endif /* COMPONENTS_DRIVERS_INC_VCNL4010_REG_H_ */
