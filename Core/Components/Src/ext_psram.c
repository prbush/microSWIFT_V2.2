/*
 * ext_psram.c
 *
 *  Created on: Dec 12, 2024
 *      Author: philbush
 */

#include "ext_psram.h"
#include "octospi.h"

static bool __setup_psram ( void );
static bool __enable_memory_mapped_mode ( void );

bool initialize_psram ( void )
{
  if ( !octospi1_init () )
  {
    return false;
  }

  // Give time for PSRAM to boot
  HAL_Delay (1);

  if ( !__setup_psram () )
  {
    return false;
  }

  return __enable_memory_mapped_mode ();
}

static bool __setup_psram ( void )
{
  OSPI_RegularCmdTypeDef cmd =
    { 0 };
  uint8_t dummy_data = 0;

  // Enable quad mode
  cmd.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
  cmd.Instruction = PSRAM_ENTER_QUAD_MODE;
  cmd.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
  cmd.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
  cmd.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  cmd.AddressMode = HAL_OSPI_ADDRESS_NONE;
  cmd.AddressSize = HAL_OSPI_ADDRESS_24_BITS;
  cmd.AddressDtrMode = HAL_OSPI_ADDRESS_DTR_DISABLE;
  cmd.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  cmd.DataMode = HAL_OSPI_DATA_1_LINE;
  cmd.DataDtrMode = HAL_OSPI_DATA_DTR_DISABLE;
  cmd.NbData = 1;
  cmd.DummyCycles = DUMMY_CYCLES_WRITE;
  cmd.DQSMode = HAL_OSPI_DQS_DISABLE;
  cmd.SIOOMode = HAL_OSPI_SIOO_INST_ONLY_FIRST_CMD;

  // Config the command
  if ( HAL_OSPI_Command (&hospi1, &cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK )
  {
    return false;
  }

  // Transmit the command
  if ( HAL_OSPI_Transmit (&hospi1, &dummy_data, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK )
  {
    return false;
  }

  return true;
}

static bool __enable_memory_mapped_mode ( void )
{
  OSPI_MemoryMappedTypeDef mem_mapped_config =
    { 0 };
  OSPI_RegularCmdTypeDef cmd =
    { 0 };

  cmd.OperationType = HAL_OSPI_OPTYPE_WRITE_CFG;
  cmd.Instruction = PSRAM_WRITE_QUAD;
  cmd.InstructionMode = HAL_OSPI_INSTRUCTION_4_LINES;
  cmd.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
  cmd.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  cmd.AddressMode = HAL_OSPI_ADDRESS_4_LINES;
  cmd.AddressSize = HAL_OSPI_ADDRESS_24_BITS;
  cmd.AddressDtrMode = HAL_OSPI_ADDRESS_DTR_DISABLE;
  cmd.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  cmd.DataMode = HAL_OSPI_DATA_4_LINES;
  cmd.DataDtrMode = HAL_OSPI_DATA_DTR_DISABLE;
  cmd.DummyCycles = DUMMY_CYCLES_WRITE;
  cmd.DQSMode = HAL_OSPI_DQS_ENABLE;
  cmd.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

  if ( HAL_OSPI_Command (&hospi1, &cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK )
  {
    return false;
  }

  cmd.OperationType = HAL_OSPI_OPTYPE_READ_CFG;
  cmd.Instruction = PSRAM_FAST_READ_QUAD;
  cmd.DummyCycles = DUMMY_CYCLES_FAST_READ_QUAD;
  cmd.DQSMode = HAL_OSPI_DQS_DISABLE;

  if ( HAL_OSPI_Command (&hospi1, &cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK )
  {
    return false;
  }

  mem_mapped_config.TimeOutActivation = HAL_OSPI_TIMEOUT_COUNTER_ENABLE;
  mem_mapped_config.TimeOutPeriod = 0x34;

  if ( HAL_OSPI_MemoryMapped (&hospi1, &mem_mapped_config) != HAL_OK )
  {
    return false;
  }

  return true;
}
