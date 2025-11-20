################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../USB/Class/CDC/Src/usbd_cdc.c \
../USB/Class/CDC/Src/usbd_cdc_if_template.c 

C_DEPS += \
./USB/Class/CDC/Src/usbd_cdc.d \
./USB/Class/CDC/Src/usbd_cdc_if_template.d 

OBJS += \
./USB/Class/CDC/Src/usbd_cdc.o \
./USB/Class/CDC/Src/usbd_cdc_if_template.o 


# Each subdirectory must supply rules for building sources it contributes
USB/Class/CDC/Src/%.o USB/Class/CDC/Src/%.su USB/Class/CDC/Src/%.cyclo: ../USB/Class/CDC/Src/%.c USB/Class/CDC/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32U5G9xx -DUX_INCLUDE_USER_DEFINE_FILE -c -I../Core/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/ST/touchgfx/framework/include -I../TouchGFX/generated/fonts/include -I../TouchGFX/generated/gui_generated/include -I../TouchGFX/generated/images/include -I../TouchGFX/generated/texts/include -I../TouchGFX/generated/videos/include -I../TouchGFX/gui/include -I../TouchGFX/App -I../TouchGFX/target/generated -I../TouchGFX/target -I../USBX/App -I../USBX/Target -I../Middlewares/ST/usbx/common/core/inc -I../Middlewares/ST/usbx/ports/generic/inc -I../Middlewares/ST/usbx/common/usbx_stm32_device_controllers -I../Middlewares/ST/usbx/common/usbx_device_classes/inc -I"C:/TouchGFXProjects/SU6000_Center/USB/Class/CDC/Inc" -I"C:/TouchGFXProjects/SU6000_Center/USB/Core/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-USB-2f-Class-2f-CDC-2f-Src

clean-USB-2f-Class-2f-CDC-2f-Src:
	-$(RM) ./USB/Class/CDC/Src/usbd_cdc.cyclo ./USB/Class/CDC/Src/usbd_cdc.d ./USB/Class/CDC/Src/usbd_cdc.o ./USB/Class/CDC/Src/usbd_cdc.su ./USB/Class/CDC/Src/usbd_cdc_if_template.cyclo ./USB/Class/CDC/Src/usbd_cdc_if_template.d ./USB/Class/CDC/Src/usbd_cdc_if_template.o ./USB/Class/CDC/Src/usbd_cdc_if_template.su

.PHONY: clean-USB-2f-Class-2f-CDC-2f-Src

