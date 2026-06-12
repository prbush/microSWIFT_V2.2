################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/Users/philbush/STM32Cube/Repository/STM32Cube_FW_U5_V1.6.0/Drivers/STM32U5xx_HAL_Driver/Src/stm32u5xx_hal_lptim.c 

OBJS += \
./Drivers/STM32U5xx_HAL_Driver/stm32u5xx_hal_lptim.o 

C_DEPS += \
./Drivers/STM32U5xx_HAL_Driver/stm32u5xx_hal_lptim.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/STM32U5xx_HAL_Driver/stm32u5xx_hal_lptim.o: /Users/philbush/STM32Cube/Repository/STM32Cube_FW_U5_V1.6.0/Drivers/STM32U5xx_HAL_Driver/Src/stm32u5xx_hal_lptim.c Drivers/STM32U5xx_HAL_Driver/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_NON_SECURE=1 -DUSE_HAL_DRIVER -DSTM32U5A5xx -DSTM32_THREAD_SAFE_STRATEGY=2 -DFX_INCLUDE_USER_DEFINE_FILE -c -I../Core/Inc -I../AZURE_RTOS/App -I../Core/ThreadSafe -I../FileX/App -I"/Users/philbush/STM32CubeIDE/microSWIFT/microSWIFT_V2.2/microSWIFT_V2.2/Core/Components/Inc" -I"/Users/philbush/STM32CubeIDE/microSWIFT/microSWIFT_V2.2/microSWIFT_V2.2/Core/Components/Drivers/Inc" -I../FileX/Target -I../Drivers/STM32U5xx_HAL_Driver/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../Middlewares/ST/threadx/common/inc -I../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../Middlewares/ST/filex/common/inc -I../Middlewares/ST/filex/ports/generic/inc -I../Middlewares/ST/threadx/ports/cortex_m33/gnu/inc -I../Middlewares/ST/threadx/utility/low_power -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-STM32U5xx_HAL_Driver

clean-Drivers-2f-STM32U5xx_HAL_Driver:
	-$(RM) ./Drivers/STM32U5xx_HAL_Driver/stm32u5xx_hal_lptim.cyclo ./Drivers/STM32U5xx_HAL_Driver/stm32u5xx_hal_lptim.d ./Drivers/STM32U5xx_HAL_Driver/stm32u5xx_hal_lptim.o ./Drivers/STM32U5xx_HAL_Driver/stm32u5xx_hal_lptim.su

.PHONY: clean-Drivers-2f-STM32U5xx_HAL_Driver

