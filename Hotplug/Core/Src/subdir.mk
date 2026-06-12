################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/adc.c \
../Core/Src/app_threadx.c \
../Core/Src/byte_array.c \
../Core/Src/controller.c \
../Core/Src/error_handler.c \
../Core/Src/gpdma.c \
../Core/Src/gpio.c \
../Core/Src/i2c.c \
../Core/Src/icache.c \
../Core/Src/interrupt_callbacks.c \
../Core/Src/linked_list.c \
../Core/Src/main.c \
../Core/Src/octospi.c \
../Core/Src/persistent_ram.c \
../Core/Src/sdmmc.c \
../Core/Src/spi.c \
../Core/Src/stm32u5xx_hal_msp.c \
../Core/Src/stm32u5xx_hal_timebase_tim.c \
../Core/Src/stm32u5xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32u5xx.c \
../Core/Src/testing_hooks.c \
../Core/Src/threadx_support.c \
../Core/Src/usart.c \
../Core/Src/watchdog.c 

S_UPPER_SRCS += \
../Core/Src/tx_initialize_low_level.S 

OBJS += \
./Core/Src/adc.o \
./Core/Src/app_threadx.o \
./Core/Src/byte_array.o \
./Core/Src/controller.o \
./Core/Src/error_handler.o \
./Core/Src/gpdma.o \
./Core/Src/gpio.o \
./Core/Src/i2c.o \
./Core/Src/icache.o \
./Core/Src/interrupt_callbacks.o \
./Core/Src/linked_list.o \
./Core/Src/main.o \
./Core/Src/octospi.o \
./Core/Src/persistent_ram.o \
./Core/Src/sdmmc.o \
./Core/Src/spi.o \
./Core/Src/stm32u5xx_hal_msp.o \
./Core/Src/stm32u5xx_hal_timebase_tim.o \
./Core/Src/stm32u5xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32u5xx.o \
./Core/Src/testing_hooks.o \
./Core/Src/threadx_support.o \
./Core/Src/tx_initialize_low_level.o \
./Core/Src/usart.o \
./Core/Src/watchdog.o 

S_UPPER_DEPS += \
./Core/Src/tx_initialize_low_level.d 

C_DEPS += \
./Core/Src/adc.d \
./Core/Src/app_threadx.d \
./Core/Src/byte_array.d \
./Core/Src/controller.d \
./Core/Src/error_handler.d \
./Core/Src/gpdma.d \
./Core/Src/gpio.d \
./Core/Src/i2c.d \
./Core/Src/icache.d \
./Core/Src/interrupt_callbacks.d \
./Core/Src/linked_list.d \
./Core/Src/main.d \
./Core/Src/octospi.d \
./Core/Src/persistent_ram.d \
./Core/Src/sdmmc.d \
./Core/Src/spi.d \
./Core/Src/stm32u5xx_hal_msp.d \
./Core/Src/stm32u5xx_hal_timebase_tim.d \
./Core/Src/stm32u5xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32u5xx.d \
./Core/Src/testing_hooks.d \
./Core/Src/threadx_support.d \
./Core/Src/usart.d \
./Core/Src/watchdog.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_NON_SECURE=1 -DUSE_HAL_DRIVER -DSTM32U5A5xx -DSTM32_THREAD_SAFE_STRATEGY=2 -DFX_INCLUDE_USER_DEFINE_FILE -c -I../Core/Inc -I../AZURE_RTOS/App -I../Core/ThreadSafe -I../FileX/App -I"/Users/philbush/STM32CubeIDE/microSWIFT/microSWIFT_V2.2/microSWIFT_V2.2/Core/Components/Inc" -I"/Users/philbush/STM32CubeIDE/microSWIFT/microSWIFT_V2.2/microSWIFT_V2.2/Core/Components/Drivers/Inc" -I../FileX/Target -I../Drivers/STM32U5xx_HAL_Driver/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../Middlewares/ST/threadx/common/inc -I../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../Middlewares/ST/filex/common/inc -I../Middlewares/ST/filex/ports/generic/inc -I../Middlewares/ST/threadx/ports/cortex_m33/gnu/inc -I../Middlewares/ST/threadx/utility/low_power -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/%.o: ../Core/Src/%.S Core/Src/subdir.mk
	arm-none-eabi-gcc -mcpu=cortex-m33 -g3 -DTX_SINGLE_MODE_NON_SECURE=1 -DDEBUG -c -I../Core/Inc -I../FileX/App -I../AZURE_RTOS/App -I../FileX/Target -I../Drivers/STM32U5xx_HAL_Driver/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../Middlewares/ST/threadx/common/inc -I../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../Middlewares/ST/filex/common/inc -I../Middlewares/ST/filex/ports/generic/inc -I../Middlewares/ST/threadx/ports/cortex_m33/gnu/inc -I../Middlewares/ST/threadx/utility/low_power -I../Drivers/CMSIS/Include -x assembler-with-cpp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@" "$<"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/adc.cyclo ./Core/Src/adc.d ./Core/Src/adc.o ./Core/Src/adc.su ./Core/Src/app_threadx.cyclo ./Core/Src/app_threadx.d ./Core/Src/app_threadx.o ./Core/Src/app_threadx.su ./Core/Src/byte_array.cyclo ./Core/Src/byte_array.d ./Core/Src/byte_array.o ./Core/Src/byte_array.su ./Core/Src/controller.cyclo ./Core/Src/controller.d ./Core/Src/controller.o ./Core/Src/controller.su ./Core/Src/error_handler.cyclo ./Core/Src/error_handler.d ./Core/Src/error_handler.o ./Core/Src/error_handler.su ./Core/Src/gpdma.cyclo ./Core/Src/gpdma.d ./Core/Src/gpdma.o ./Core/Src/gpdma.su ./Core/Src/gpio.cyclo ./Core/Src/gpio.d ./Core/Src/gpio.o ./Core/Src/gpio.su ./Core/Src/i2c.cyclo ./Core/Src/i2c.d ./Core/Src/i2c.o ./Core/Src/i2c.su ./Core/Src/icache.cyclo ./Core/Src/icache.d ./Core/Src/icache.o ./Core/Src/icache.su ./Core/Src/interrupt_callbacks.cyclo ./Core/Src/interrupt_callbacks.d ./Core/Src/interrupt_callbacks.o ./Core/Src/interrupt_callbacks.su ./Core/Src/linked_list.cyclo ./Core/Src/linked_list.d ./Core/Src/linked_list.o ./Core/Src/linked_list.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/octospi.cyclo ./Core/Src/octospi.d ./Core/Src/octospi.o ./Core/Src/octospi.su ./Core/Src/persistent_ram.cyclo ./Core/Src/persistent_ram.d ./Core/Src/persistent_ram.o ./Core/Src/persistent_ram.su ./Core/Src/sdmmc.cyclo ./Core/Src/sdmmc.d ./Core/Src/sdmmc.o ./Core/Src/sdmmc.su ./Core/Src/spi.cyclo ./Core/Src/spi.d ./Core/Src/spi.o ./Core/Src/spi.su ./Core/Src/stm32u5xx_hal_msp.cyclo ./Core/Src/stm32u5xx_hal_msp.d ./Core/Src/stm32u5xx_hal_msp.o ./Core/Src/stm32u5xx_hal_msp.su ./Core/Src/stm32u5xx_hal_timebase_tim.cyclo ./Core/Src/stm32u5xx_hal_timebase_tim.d ./Core/Src/stm32u5xx_hal_timebase_tim.o ./Core/Src/stm32u5xx_hal_timebase_tim.su ./Core/Src/stm32u5xx_it.cyclo ./Core/Src/stm32u5xx_it.d ./Core/Src/stm32u5xx_it.o ./Core/Src/stm32u5xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32u5xx.cyclo ./Core/Src/system_stm32u5xx.d ./Core/Src/system_stm32u5xx.o ./Core/Src/system_stm32u5xx.su ./Core/Src/testing_hooks.cyclo ./Core/Src/testing_hooks.d ./Core/Src/testing_hooks.o ./Core/Src/testing_hooks.su ./Core/Src/threadx_support.cyclo ./Core/Src/threadx_support.d ./Core/Src/threadx_support.o ./Core/Src/threadx_support.su ./Core/Src/tx_initialize_low_level.d ./Core/Src/tx_initialize_low_level.o ./Core/Src/usart.cyclo ./Core/Src/usart.d ./Core/Src/usart.o ./Core/Src/usart.su ./Core/Src/watchdog.cyclo ./Core/Src/watchdog.d ./Core/Src/watchdog.o ./Core/Src/watchdog.su

.PHONY: clean-Core-2f-Src

