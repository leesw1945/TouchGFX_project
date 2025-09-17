################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../TouchGFX/SU6000_Center/touchgfx/framework/source/platform/hal/simulator/sdl2/HALSDL2.cpp \
../TouchGFX/SU6000_Center/touchgfx/framework/source/platform/hal/simulator/sdl2/HALSDL2_icon.cpp \
../TouchGFX/SU6000_Center/touchgfx/framework/source/platform/hal/simulator/sdl2/OSWrappers.cpp 

OBJS += \
./TouchGFX/SU6000_Center/touchgfx/framework/source/platform/hal/simulator/sdl2/HALSDL2.o \
./TouchGFX/SU6000_Center/touchgfx/framework/source/platform/hal/simulator/sdl2/HALSDL2_icon.o \
./TouchGFX/SU6000_Center/touchgfx/framework/source/platform/hal/simulator/sdl2/OSWrappers.o 

CPP_DEPS += \
./TouchGFX/SU6000_Center/touchgfx/framework/source/platform/hal/simulator/sdl2/HALSDL2.d \
./TouchGFX/SU6000_Center/touchgfx/framework/source/platform/hal/simulator/sdl2/HALSDL2_icon.d \
./TouchGFX/SU6000_Center/touchgfx/framework/source/platform/hal/simulator/sdl2/OSWrappers.d 


# Each subdirectory must supply rules for building sources it contributes
TouchGFX/SU6000_Center/touchgfx/framework/source/platform/hal/simulator/sdl2/%.o TouchGFX/SU6000_Center/touchgfx/framework/source/platform/hal/simulator/sdl2/%.su TouchGFX/SU6000_Center/touchgfx/framework/source/platform/hal/simulator/sdl2/%.cyclo: ../TouchGFX/SU6000_Center/touchgfx/framework/source/platform/hal/simulator/sdl2/%.cpp TouchGFX/SU6000_Center/touchgfx/framework/source/platform/hal/simulator/sdl2/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m33 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32U5G9xx -DUX_INCLUDE_USER_DEFINE_FILE -c -I../Core/Inc -I../TouchGFX/App -I../TouchGFX/target/generated -I../TouchGFX/target -I../Drivers/STM32U5xx_HAL_Driver/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../Drivers/CMSIS/Include -I../USBX/App -I../USBX/Target -I../Middlewares/ST/usbx/common/core/inc -I../Middlewares/ST/usbx/ports/generic/inc -I../Middlewares/ST/usbx/common/usbx_stm32_device_controllers -I../Middlewares/ST/usbx/common/usbx_device_classes/inc -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-TouchGFX-2f-SU6000_Center-2f-touchgfx-2f-framework-2f-source-2f-platform-2f-hal-2f-simulator-2f-sdl2

clean-TouchGFX-2f-SU6000_Center-2f-touchgfx-2f-framework-2f-source-2f-platform-2f-hal-2f-simulator-2f-sdl2:
	-$(RM) ./TouchGFX/SU6000_Center/touchgfx/framework/source/platform/hal/simulator/sdl2/HALSDL2.cyclo ./TouchGFX/SU6000_Center/touchgfx/framework/source/platform/hal/simulator/sdl2/HALSDL2.d ./TouchGFX/SU6000_Center/touchgfx/framework/source/platform/hal/simulator/sdl2/HALSDL2.o ./TouchGFX/SU6000_Center/touchgfx/framework/source/platform/hal/simulator/sdl2/HALSDL2.su ./TouchGFX/SU6000_Center/touchgfx/framework/source/platform/hal/simulator/sdl2/HALSDL2_icon.cyclo ./TouchGFX/SU6000_Center/touchgfx/framework/source/platform/hal/simulator/sdl2/HALSDL2_icon.d ./TouchGFX/SU6000_Center/touchgfx/framework/source/platform/hal/simulator/sdl2/HALSDL2_icon.o ./TouchGFX/SU6000_Center/touchgfx/framework/source/platform/hal/simulator/sdl2/HALSDL2_icon.su ./TouchGFX/SU6000_Center/touchgfx/framework/source/platform/hal/simulator/sdl2/OSWrappers.cyclo ./TouchGFX/SU6000_Center/touchgfx/framework/source/platform/hal/simulator/sdl2/OSWrappers.d ./TouchGFX/SU6000_Center/touchgfx/framework/source/platform/hal/simulator/sdl2/OSWrappers.o ./TouchGFX/SU6000_Center/touchgfx/framework/source/platform/hal/simulator/sdl2/OSWrappers.su

.PHONY: clean-TouchGFX-2f-SU6000_Center-2f-touchgfx-2f-framework-2f-source-2f-platform-2f-hal-2f-simulator-2f-sdl2

