################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Components/Src/battery.c \
../Core/Components/Src/ct_sensor.c \
../Core/Components/Src/ext_psram.c \
../Core/Components/Src/ext_rtc.c \
../Core/Components/Src/ext_rtc_server.c \
../Core/Components/Src/file_system.c \
../Core/Components/Src/file_system_server.c \
../Core/Components/Src/gnss.c \
../Core/Components/Src/iridium.c \
../Core/Components/Src/leds.c \
../Core/Components/Src/light_sensor.c \
../Core/Components/Src/logger.c \
../Core/Components/Src/rf_switch.c \
../Core/Components/Src/shared_i2c_bus.c \
../Core/Components/Src/temp_sensor.c \
../Core/Components/Src/turbidity_sensor.c 

OBJS += \
./Core/Components/Src/battery.o \
./Core/Components/Src/ct_sensor.o \
./Core/Components/Src/ext_psram.o \
./Core/Components/Src/ext_rtc.o \
./Core/Components/Src/ext_rtc_server.o \
./Core/Components/Src/file_system.o \
./Core/Components/Src/file_system_server.o \
./Core/Components/Src/gnss.o \
./Core/Components/Src/iridium.o \
./Core/Components/Src/leds.o \
./Core/Components/Src/light_sensor.o \
./Core/Components/Src/logger.o \
./Core/Components/Src/rf_switch.o \
./Core/Components/Src/shared_i2c_bus.o \
./Core/Components/Src/temp_sensor.o \
./Core/Components/Src/turbidity_sensor.o 

C_DEPS += \
./Core/Components/Src/battery.d \
./Core/Components/Src/ct_sensor.d \
./Core/Components/Src/ext_psram.d \
./Core/Components/Src/ext_rtc.d \
./Core/Components/Src/ext_rtc_server.d \
./Core/Components/Src/file_system.d \
./Core/Components/Src/file_system_server.d \
./Core/Components/Src/gnss.d \
./Core/Components/Src/iridium.d \
./Core/Components/Src/leds.d \
./Core/Components/Src/light_sensor.d \
./Core/Components/Src/logger.d \
./Core/Components/Src/rf_switch.d \
./Core/Components/Src/shared_i2c_bus.d \
./Core/Components/Src/temp_sensor.d \
./Core/Components/Src/turbidity_sensor.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Components/Src/%.o Core/Components/Src/%.su Core/Components/Src/%.cyclo: ../Core/Components/Src/%.c Core/Components/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_NON_SECURE=1 -DUSE_HAL_DRIVER -DSTM32U5A5xx -DSTM32_THREAD_SAFE_STRATEGY=2 -DFX_INCLUDE_USER_DEFINE_FILE -c -I../Core/Inc -I../AZURE_RTOS/App -I../Core/ThreadSafe -I../FileX/App -I"/Users/philbush/STM32CubeIDE/microSWIFT/microSWIFT_V2.2/microSWIFT_V2.2/Core/Components/Inc" -I"/Users/philbush/STM32CubeIDE/microSWIFT/microSWIFT_V2.2/microSWIFT_V2.2/Core/Components/Drivers/Inc" -I../FileX/Target -I../Drivers/STM32U5xx_HAL_Driver/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../Middlewares/ST/threadx/common/inc -I../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../Middlewares/ST/filex/common/inc -I../Middlewares/ST/filex/ports/generic/inc -I../Middlewares/ST/threadx/ports/cortex_m33/gnu/inc -I../Middlewares/ST/threadx/utility/low_power -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Components-2f-Src

clean-Core-2f-Components-2f-Src:
	-$(RM) ./Core/Components/Src/battery.cyclo ./Core/Components/Src/battery.d ./Core/Components/Src/battery.o ./Core/Components/Src/battery.su ./Core/Components/Src/ct_sensor.cyclo ./Core/Components/Src/ct_sensor.d ./Core/Components/Src/ct_sensor.o ./Core/Components/Src/ct_sensor.su ./Core/Components/Src/ext_psram.cyclo ./Core/Components/Src/ext_psram.d ./Core/Components/Src/ext_psram.o ./Core/Components/Src/ext_psram.su ./Core/Components/Src/ext_rtc.cyclo ./Core/Components/Src/ext_rtc.d ./Core/Components/Src/ext_rtc.o ./Core/Components/Src/ext_rtc.su ./Core/Components/Src/ext_rtc_server.cyclo ./Core/Components/Src/ext_rtc_server.d ./Core/Components/Src/ext_rtc_server.o ./Core/Components/Src/ext_rtc_server.su ./Core/Components/Src/file_system.cyclo ./Core/Components/Src/file_system.d ./Core/Components/Src/file_system.o ./Core/Components/Src/file_system.su ./Core/Components/Src/file_system_server.cyclo ./Core/Components/Src/file_system_server.d ./Core/Components/Src/file_system_server.o ./Core/Components/Src/file_system_server.su ./Core/Components/Src/gnss.cyclo ./Core/Components/Src/gnss.d ./Core/Components/Src/gnss.o ./Core/Components/Src/gnss.su ./Core/Components/Src/iridium.cyclo ./Core/Components/Src/iridium.d ./Core/Components/Src/iridium.o ./Core/Components/Src/iridium.su ./Core/Components/Src/leds.cyclo ./Core/Components/Src/leds.d ./Core/Components/Src/leds.o ./Core/Components/Src/leds.su ./Core/Components/Src/light_sensor.cyclo ./Core/Components/Src/light_sensor.d ./Core/Components/Src/light_sensor.o ./Core/Components/Src/light_sensor.su ./Core/Components/Src/logger.cyclo ./Core/Components/Src/logger.d ./Core/Components/Src/logger.o ./Core/Components/Src/logger.su ./Core/Components/Src/rf_switch.cyclo ./Core/Components/Src/rf_switch.d ./Core/Components/Src/rf_switch.o ./Core/Components/Src/rf_switch.su ./Core/Components/Src/shared_i2c_bus.cyclo ./Core/Components/Src/shared_i2c_bus.d ./Core/Components/Src/shared_i2c_bus.o ./Core/Components/Src/shared_i2c_bus.su ./Core/Components/Src/temp_sensor.cyclo ./Core/Components/Src/temp_sensor.d ./Core/Components/Src/temp_sensor.o ./Core/Components/Src/temp_sensor.su ./Core/Components/Src/turbidity_sensor.cyclo ./Core/Components/Src/turbidity_sensor.d ./Core/Components/Src/turbidity_sensor.o ./Core/Components/Src/turbidity_sensor.su

.PHONY: clean-Core-2f-Components-2f-Src

