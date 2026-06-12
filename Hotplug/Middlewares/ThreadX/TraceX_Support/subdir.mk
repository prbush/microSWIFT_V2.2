################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/Users/philbush/STM32Cube/Repository/STM32Cube_FW_U5_V1.6.0/Middlewares/ST/filex/common/src/fx_trace_event_insert.c \
/Users/philbush/STM32Cube/Repository/STM32Cube_FW_U5_V1.6.0/Middlewares/ST/filex/common/src/fx_trace_event_update.c \
/Users/philbush/STM32Cube/Repository/STM32Cube_FW_U5_V1.6.0/Middlewares/ST/filex/common/src/fx_trace_object_register.c \
/Users/philbush/STM32Cube/Repository/STM32Cube_FW_U5_V1.6.0/Middlewares/ST/filex/common/src/fx_trace_object_unregister.c 

OBJS += \
./Middlewares/ThreadX/TraceX_Support/fx_trace_event_insert.o \
./Middlewares/ThreadX/TraceX_Support/fx_trace_event_update.o \
./Middlewares/ThreadX/TraceX_Support/fx_trace_object_register.o \
./Middlewares/ThreadX/TraceX_Support/fx_trace_object_unregister.o 

C_DEPS += \
./Middlewares/ThreadX/TraceX_Support/fx_trace_event_insert.d \
./Middlewares/ThreadX/TraceX_Support/fx_trace_event_update.d \
./Middlewares/ThreadX/TraceX_Support/fx_trace_object_register.d \
./Middlewares/ThreadX/TraceX_Support/fx_trace_object_unregister.d 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/ThreadX/TraceX_Support/fx_trace_event_insert.o: /Users/philbush/STM32Cube/Repository/STM32Cube_FW_U5_V1.6.0/Middlewares/ST/filex/common/src/fx_trace_event_insert.c Middlewares/ThreadX/TraceX_Support/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_NON_SECURE=1 -DUSE_HAL_DRIVER -DSTM32U5A5xx -DSTM32_THREAD_SAFE_STRATEGY=2 -DFX_INCLUDE_USER_DEFINE_FILE -c -I../Core/Inc -I../AZURE_RTOS/App -I../Core/ThreadSafe -I../FileX/App -I"/Users/philbush/STM32CubeIDE/microSWIFT/microSWIFT_V2.2/microSWIFT_V2.2/Core/Components/Inc" -I"/Users/philbush/STM32CubeIDE/microSWIFT/microSWIFT_V2.2/microSWIFT_V2.2/Core/Components/Drivers/Inc" -I../FileX/Target -I../Drivers/STM32U5xx_HAL_Driver/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../Middlewares/ST/threadx/common/inc -I../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../Middlewares/ST/filex/common/inc -I../Middlewares/ST/filex/ports/generic/inc -I../Middlewares/ST/threadx/ports/cortex_m33/gnu/inc -I../Middlewares/ST/threadx/utility/low_power -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Middlewares/ThreadX/TraceX_Support/fx_trace_event_update.o: /Users/philbush/STM32Cube/Repository/STM32Cube_FW_U5_V1.6.0/Middlewares/ST/filex/common/src/fx_trace_event_update.c Middlewares/ThreadX/TraceX_Support/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_NON_SECURE=1 -DUSE_HAL_DRIVER -DSTM32U5A5xx -DSTM32_THREAD_SAFE_STRATEGY=2 -DFX_INCLUDE_USER_DEFINE_FILE -c -I../Core/Inc -I../AZURE_RTOS/App -I../Core/ThreadSafe -I../FileX/App -I"/Users/philbush/STM32CubeIDE/microSWIFT/microSWIFT_V2.2/microSWIFT_V2.2/Core/Components/Inc" -I"/Users/philbush/STM32CubeIDE/microSWIFT/microSWIFT_V2.2/microSWIFT_V2.2/Core/Components/Drivers/Inc" -I../FileX/Target -I../Drivers/STM32U5xx_HAL_Driver/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../Middlewares/ST/threadx/common/inc -I../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../Middlewares/ST/filex/common/inc -I../Middlewares/ST/filex/ports/generic/inc -I../Middlewares/ST/threadx/ports/cortex_m33/gnu/inc -I../Middlewares/ST/threadx/utility/low_power -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Middlewares/ThreadX/TraceX_Support/fx_trace_object_register.o: /Users/philbush/STM32Cube/Repository/STM32Cube_FW_U5_V1.6.0/Middlewares/ST/filex/common/src/fx_trace_object_register.c Middlewares/ThreadX/TraceX_Support/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_NON_SECURE=1 -DUSE_HAL_DRIVER -DSTM32U5A5xx -DSTM32_THREAD_SAFE_STRATEGY=2 -DFX_INCLUDE_USER_DEFINE_FILE -c -I../Core/Inc -I../AZURE_RTOS/App -I../Core/ThreadSafe -I../FileX/App -I"/Users/philbush/STM32CubeIDE/microSWIFT/microSWIFT_V2.2/microSWIFT_V2.2/Core/Components/Inc" -I"/Users/philbush/STM32CubeIDE/microSWIFT/microSWIFT_V2.2/microSWIFT_V2.2/Core/Components/Drivers/Inc" -I../FileX/Target -I../Drivers/STM32U5xx_HAL_Driver/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../Middlewares/ST/threadx/common/inc -I../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../Middlewares/ST/filex/common/inc -I../Middlewares/ST/filex/ports/generic/inc -I../Middlewares/ST/threadx/ports/cortex_m33/gnu/inc -I../Middlewares/ST/threadx/utility/low_power -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Middlewares/ThreadX/TraceX_Support/fx_trace_object_unregister.o: /Users/philbush/STM32Cube/Repository/STM32Cube_FW_U5_V1.6.0/Middlewares/ST/filex/common/src/fx_trace_object_unregister.c Middlewares/ThreadX/TraceX_Support/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_NON_SECURE=1 -DUSE_HAL_DRIVER -DSTM32U5A5xx -DSTM32_THREAD_SAFE_STRATEGY=2 -DFX_INCLUDE_USER_DEFINE_FILE -c -I../Core/Inc -I../AZURE_RTOS/App -I../Core/ThreadSafe -I../FileX/App -I"/Users/philbush/STM32CubeIDE/microSWIFT/microSWIFT_V2.2/microSWIFT_V2.2/Core/Components/Inc" -I"/Users/philbush/STM32CubeIDE/microSWIFT/microSWIFT_V2.2/microSWIFT_V2.2/Core/Components/Drivers/Inc" -I../FileX/Target -I../Drivers/STM32U5xx_HAL_Driver/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../Middlewares/ST/threadx/common/inc -I../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../Middlewares/ST/filex/common/inc -I../Middlewares/ST/filex/ports/generic/inc -I../Middlewares/ST/threadx/ports/cortex_m33/gnu/inc -I../Middlewares/ST/threadx/utility/low_power -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Middlewares-2f-ThreadX-2f-TraceX_Support

clean-Middlewares-2f-ThreadX-2f-TraceX_Support:
	-$(RM) ./Middlewares/ThreadX/TraceX_Support/fx_trace_event_insert.cyclo ./Middlewares/ThreadX/TraceX_Support/fx_trace_event_insert.d ./Middlewares/ThreadX/TraceX_Support/fx_trace_event_insert.o ./Middlewares/ThreadX/TraceX_Support/fx_trace_event_insert.su ./Middlewares/ThreadX/TraceX_Support/fx_trace_event_update.cyclo ./Middlewares/ThreadX/TraceX_Support/fx_trace_event_update.d ./Middlewares/ThreadX/TraceX_Support/fx_trace_event_update.o ./Middlewares/ThreadX/TraceX_Support/fx_trace_event_update.su ./Middlewares/ThreadX/TraceX_Support/fx_trace_object_register.cyclo ./Middlewares/ThreadX/TraceX_Support/fx_trace_object_register.d ./Middlewares/ThreadX/TraceX_Support/fx_trace_object_register.o ./Middlewares/ThreadX/TraceX_Support/fx_trace_object_register.su ./Middlewares/ThreadX/TraceX_Support/fx_trace_object_unregister.cyclo ./Middlewares/ThreadX/TraceX_Support/fx_trace_object_unregister.d ./Middlewares/ThreadX/TraceX_Support/fx_trace_object_unregister.o ./Middlewares/ThreadX/TraceX_Support/fx_trace_object_unregister.su

.PHONY: clean-Middlewares-2f-ThreadX-2f-TraceX_Support

