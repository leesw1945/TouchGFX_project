################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../TouchGFX/SU6000_Center/gui/src/screen1_screen/Screen1Presenter.cpp \
../TouchGFX/SU6000_Center/gui/src/screen1_screen/Screen1View.cpp 

OBJS += \
./TouchGFX/SU6000_Center/gui/src/screen1_screen/Screen1Presenter.o \
./TouchGFX/SU6000_Center/gui/src/screen1_screen/Screen1View.o 

CPP_DEPS += \
./TouchGFX/SU6000_Center/gui/src/screen1_screen/Screen1Presenter.d \
./TouchGFX/SU6000_Center/gui/src/screen1_screen/Screen1View.d 


# Each subdirectory must supply rules for building sources it contributes
TouchGFX/SU6000_Center/gui/src/screen1_screen/%.o TouchGFX/SU6000_Center/gui/src/screen1_screen/%.su TouchGFX/SU6000_Center/gui/src/screen1_screen/%.cyclo: ../TouchGFX/SU6000_Center/gui/src/screen1_screen/%.cpp TouchGFX/SU6000_Center/gui/src/screen1_screen/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m33 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32U5G9xx -DUX_INCLUDE_USER_DEFINE_FILE -c -I../Core/Inc -I../TouchGFX/App -I../TouchGFX/target/generated -I../TouchGFX/target -I../Drivers/STM32U5xx_HAL_Driver/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../Drivers/CMSIS/Include -I../USBX/App -I../USBX/Target -I../Middlewares/ST/usbx/common/core/inc -I../Middlewares/ST/usbx/ports/generic/inc -I../Middlewares/ST/usbx/common/usbx_stm32_device_controllers -I../Middlewares/ST/usbx/common/usbx_device_classes/inc -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-TouchGFX-2f-SU6000_Center-2f-gui-2f-src-2f-screen1_screen

clean-TouchGFX-2f-SU6000_Center-2f-gui-2f-src-2f-screen1_screen:
	-$(RM) ./TouchGFX/SU6000_Center/gui/src/screen1_screen/Screen1Presenter.cyclo ./TouchGFX/SU6000_Center/gui/src/screen1_screen/Screen1Presenter.d ./TouchGFX/SU6000_Center/gui/src/screen1_screen/Screen1Presenter.o ./TouchGFX/SU6000_Center/gui/src/screen1_screen/Screen1Presenter.su ./TouchGFX/SU6000_Center/gui/src/screen1_screen/Screen1View.cyclo ./TouchGFX/SU6000_Center/gui/src/screen1_screen/Screen1View.d ./TouchGFX/SU6000_Center/gui/src/screen1_screen/Screen1View.o ./TouchGFX/SU6000_Center/gui/src/screen1_screen/Screen1View.su

.PHONY: clean-TouchGFX-2f-SU6000_Center-2f-gui-2f-src-2f-screen1_screen

