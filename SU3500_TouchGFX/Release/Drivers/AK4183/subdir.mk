################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/AK4183/ak4183.c 

C_DEPS += \
./Drivers/AK4183/ak4183.d 

OBJS += \
./Drivers/AK4183/ak4183.o 


# Each subdirectory must supply rules for building sources it contributes
Drivers/AK4183/%.o Drivers/AK4183/%.su Drivers/AK4183/%.cyclo: ../Drivers/AK4183/%.c Drivers/AK4183/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32F429xx -c -I../Core/Inc -I"C:/TouchGFXProjects/SU3500_TouchGFX/Drivers/AK4183" -I../TouchGFX/App -I../TouchGFX/target/generated -I../TouchGFX/target -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/ST/touchgfx/framework/include -I../TouchGFX/generated/fonts/include -I../TouchGFX/generated/gui_generated/include -I../TouchGFX/generated/images/include -I../TouchGFX/generated/texts/include -I../TouchGFX/generated/videos/include -I../TouchGFX/gui/include -I"C:/TouchGFXProjects/SU3500_TouchGFX/Drivers/AK4183" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-AK4183

clean-Drivers-2f-AK4183:
	-$(RM) ./Drivers/AK4183/ak4183.cyclo ./Drivers/AK4183/ak4183.d ./Drivers/AK4183/ak4183.o ./Drivers/AK4183/ak4183.su

.PHONY: clean-Drivers-2f-AK4183

