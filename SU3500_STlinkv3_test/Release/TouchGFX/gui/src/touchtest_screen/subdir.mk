################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../TouchGFX/gui/src/touchtest_screen/touchTestPresenter.cpp \
../TouchGFX/gui/src/touchtest_screen/touchTestView.cpp 

OBJS += \
./TouchGFX/gui/src/touchtest_screen/touchTestPresenter.o \
./TouchGFX/gui/src/touchtest_screen/touchTestView.o 

CPP_DEPS += \
./TouchGFX/gui/src/touchtest_screen/touchTestPresenter.d \
./TouchGFX/gui/src/touchtest_screen/touchTestView.d 


# Each subdirectory must supply rules for building sources it contributes
TouchGFX/gui/src/touchtest_screen/%.o TouchGFX/gui/src/touchtest_screen/%.su TouchGFX/gui/src/touchtest_screen/%.cyclo: ../TouchGFX/gui/src/touchtest_screen/%.cpp TouchGFX/gui/src/touchtest_screen/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m4 -std=gnu++14 -DUSE_HAL_DRIVER -DSTM32F429xx -c -I../Core/Inc -I../TouchGFX/App -I../TouchGFX/target/generated -I../TouchGFX/target -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/ST/touchgfx/framework/include -I../TouchGFX/generated/fonts/include -I../TouchGFX/generated/gui_generated/include -I../TouchGFX/generated/images/include -I../TouchGFX/generated/texts/include -I../TouchGFX/generated/videos/include -I../TouchGFX/gui/include -Os -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -femit-class-debug-always -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-TouchGFX-2f-gui-2f-src-2f-touchtest_screen

clean-TouchGFX-2f-gui-2f-src-2f-touchtest_screen:
	-$(RM) ./TouchGFX/gui/src/touchtest_screen/touchTestPresenter.cyclo ./TouchGFX/gui/src/touchtest_screen/touchTestPresenter.d ./TouchGFX/gui/src/touchtest_screen/touchTestPresenter.o ./TouchGFX/gui/src/touchtest_screen/touchTestPresenter.su ./TouchGFX/gui/src/touchtest_screen/touchTestView.cyclo ./TouchGFX/gui/src/touchtest_screen/touchTestView.d ./TouchGFX/gui/src/touchtest_screen/touchTestView.o ./TouchGFX/gui/src/touchtest_screen/touchTestView.su

.PHONY: clean-TouchGFX-2f-gui-2f-src-2f-touchtest_screen

