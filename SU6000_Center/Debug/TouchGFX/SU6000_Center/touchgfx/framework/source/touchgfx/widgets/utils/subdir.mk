################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/utils/qrcodegen.cpp 

OBJS += \
./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/utils/qrcodegen.o 

CPP_DEPS += \
./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/utils/qrcodegen.d 


# Each subdirectory must supply rules for building sources it contributes
TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/utils/%.o TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/utils/%.su TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/utils/%.cyclo: ../TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/utils/%.cpp TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/utils/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m33 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32U5G9xx -DUX_INCLUDE_USER_DEFINE_FILE -c -I../Core/Inc -I../TouchGFX/App -I../TouchGFX/target/generated -I../TouchGFX/target -I../Drivers/STM32U5xx_HAL_Driver/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../Drivers/CMSIS/Include -I../USBX/App -I../USBX/Target -I../Middlewares/ST/usbx/common/core/inc -I../Middlewares/ST/usbx/ports/generic/inc -I../Middlewares/ST/usbx/common/usbx_stm32_device_controllers -I../Middlewares/ST/usbx/common/usbx_device_classes/inc -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-TouchGFX-2f-SU6000_Center-2f-touchgfx-2f-framework-2f-source-2f-touchgfx-2f-widgets-2f-utils

clean-TouchGFX-2f-SU6000_Center-2f-touchgfx-2f-framework-2f-source-2f-touchgfx-2f-widgets-2f-utils:
	-$(RM) ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/utils/qrcodegen.cyclo ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/utils/qrcodegen.d ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/utils/qrcodegen.o ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/utils/qrcodegen.su

.PHONY: clean-TouchGFX-2f-SU6000_Center-2f-touchgfx-2f-framework-2f-source-2f-touchgfx-2f-widgets-2f-utils

