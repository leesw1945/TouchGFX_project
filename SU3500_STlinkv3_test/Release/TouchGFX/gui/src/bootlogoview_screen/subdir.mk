################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../TouchGFX/gui/src/bootlogoview_screen/BootLogoViewPresenter.cpp \
../TouchGFX/gui/src/bootlogoview_screen/BootLogoViewView.cpp 

OBJS += \
./TouchGFX/gui/src/bootlogoview_screen/BootLogoViewPresenter.o \
./TouchGFX/gui/src/bootlogoview_screen/BootLogoViewView.o 

CPP_DEPS += \
./TouchGFX/gui/src/bootlogoview_screen/BootLogoViewPresenter.d \
./TouchGFX/gui/src/bootlogoview_screen/BootLogoViewView.d 


# Each subdirectory must supply rules for building sources it contributes
TouchGFX/gui/src/bootlogoview_screen/%.o TouchGFX/gui/src/bootlogoview_screen/%.su TouchGFX/gui/src/bootlogoview_screen/%.cyclo: ../TouchGFX/gui/src/bootlogoview_screen/%.cpp TouchGFX/gui/src/bootlogoview_screen/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m4 -std=gnu++14 -DUSE_HAL_DRIVER -DSTM32F429xx -c -I../Core/Inc -I"C:/TouchGFXProjects/SU3500_TouchGFX/Drivers/CustomDriver" -I../TouchGFX/App -I../TouchGFX/target/generated -I../TouchGFX/target -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/ST/touchgfx/framework/include -I../TouchGFX/generated/fonts/include -I../TouchGFX/generated/gui_generated/include -I../TouchGFX/generated/images/include -I../TouchGFX/generated/texts/include -I../TouchGFX/generated/videos/include -I../TouchGFX/gui/include -Os -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -femit-class-debug-always -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-TouchGFX-2f-gui-2f-src-2f-bootlogoview_screen

clean-TouchGFX-2f-gui-2f-src-2f-bootlogoview_screen:
	-$(RM) ./TouchGFX/gui/src/bootlogoview_screen/BootLogoViewPresenter.cyclo ./TouchGFX/gui/src/bootlogoview_screen/BootLogoViewPresenter.d ./TouchGFX/gui/src/bootlogoview_screen/BootLogoViewPresenter.o ./TouchGFX/gui/src/bootlogoview_screen/BootLogoViewPresenter.su ./TouchGFX/gui/src/bootlogoview_screen/BootLogoViewView.cyclo ./TouchGFX/gui/src/bootlogoview_screen/BootLogoViewView.d ./TouchGFX/gui/src/bootlogoview_screen/BootLogoViewView.o ./TouchGFX/gui/src/bootlogoview_screen/BootLogoViewView.su

.PHONY: clean-TouchGFX-2f-gui-2f-src-2f-bootlogoview_screen

