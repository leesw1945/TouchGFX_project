################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../USB/Core/Src/usbd_conf.c \
../USB/Core/Src/usbd_core.c \
../USB/Core/Src/usbd_ctlreq.c \
../USB/Core/Src/usbd_desc_template.c \
../USB/Core/Src/usbd_ioreq.c 

C_DEPS += \
./USB/Core/Src/usbd_conf.d \
./USB/Core/Src/usbd_core.d \
./USB/Core/Src/usbd_ctlreq.d \
./USB/Core/Src/usbd_desc_template.d \
./USB/Core/Src/usbd_ioreq.d 

OBJS += \
./USB/Core/Src/usbd_conf.o \
./USB/Core/Src/usbd_core.o \
./USB/Core/Src/usbd_ctlreq.o \
./USB/Core/Src/usbd_desc_template.o \
./USB/Core/Src/usbd_ioreq.o 


# Each subdirectory must supply rules for building sources it contributes
USB/Core/Src/%.o USB/Core/Src/%.su USB/Core/Src/%.cyclo: ../USB/Core/Src/%.c USB/Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32U5G9xx -DUX_INCLUDE_USER_DEFINE_FILE -c -I../Core/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/ST/touchgfx/framework/include -I../TouchGFX/generated/fonts/include -I../TouchGFX/generated/gui_generated/include -I../TouchGFX/generated/images/include -I../TouchGFX/generated/texts/include -I../TouchGFX/generated/videos/include -I../TouchGFX/gui/include -I../TouchGFX/App -I../TouchGFX/target/generated -I../TouchGFX/target -I../USBX/App -I../USBX/Target -I../Middlewares/ST/usbx/common/core/inc -I../Middlewares/ST/usbx/ports/generic/inc -I../Middlewares/ST/usbx/common/usbx_stm32_device_controllers -I../Middlewares/ST/usbx/common/usbx_device_classes/inc -I"C:/TouchGFXProjects/SU6000_Center/USB/Class/CDC/Inc" -I"C:/TouchGFXProjects/SU6000_Center/USB/Core/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-USB-2f-Core-2f-Src

clean-USB-2f-Core-2f-Src:
	-$(RM) ./USB/Core/Src/usbd_conf.cyclo ./USB/Core/Src/usbd_conf.d ./USB/Core/Src/usbd_conf.o ./USB/Core/Src/usbd_conf.su ./USB/Core/Src/usbd_core.cyclo ./USB/Core/Src/usbd_core.d ./USB/Core/Src/usbd_core.o ./USB/Core/Src/usbd_core.su ./USB/Core/Src/usbd_ctlreq.cyclo ./USB/Core/Src/usbd_ctlreq.d ./USB/Core/Src/usbd_ctlreq.o ./USB/Core/Src/usbd_ctlreq.su ./USB/Core/Src/usbd_desc_template.cyclo ./USB/Core/Src/usbd_desc_template.d ./USB/Core/Src/usbd_desc_template.o ./USB/Core/Src/usbd_desc_template.su ./USB/Core/Src/usbd_ioreq.cyclo ./USB/Core/Src/usbd_ioreq.d ./USB/Core/Src/usbd_ioreq.o ./USB/Core/Src/usbd_ioreq.su

.PHONY: clean-USB-2f-Core-2f-Src

