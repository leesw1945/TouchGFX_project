################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/CacheableContainer.cpp \
../TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/Container.cpp \
../TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/ListLayout.cpp \
../TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/ModalWindow.cpp \
../TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/ScrollableContainer.cpp \
../TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/SlideMenu.cpp \
../TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/Slider.cpp \
../TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/SwipeContainer.cpp \
../TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/ZoomAnimationImage.cpp 

OBJS += \
./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/CacheableContainer.o \
./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/Container.o \
./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/ListLayout.o \
./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/ModalWindow.o \
./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/ScrollableContainer.o \
./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/SlideMenu.o \
./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/Slider.o \
./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/SwipeContainer.o \
./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/ZoomAnimationImage.o 

CPP_DEPS += \
./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/CacheableContainer.d \
./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/Container.d \
./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/ListLayout.d \
./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/ModalWindow.d \
./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/ScrollableContainer.d \
./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/SlideMenu.d \
./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/Slider.d \
./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/SwipeContainer.d \
./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/ZoomAnimationImage.d 


# Each subdirectory must supply rules for building sources it contributes
TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/%.o TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/%.su TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/%.cyclo: ../TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/%.cpp TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m33 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32U5G9xx -DUX_INCLUDE_USER_DEFINE_FILE -c -I../Core/Inc -I../TouchGFX/App -I../TouchGFX/target/generated -I../TouchGFX/target -I../Drivers/STM32U5xx_HAL_Driver/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../Drivers/CMSIS/Include -I../USBX/App -I../USBX/Target -I../Middlewares/ST/usbx/common/core/inc -I../Middlewares/ST/usbx/ports/generic/inc -I../Middlewares/ST/usbx/common/usbx_stm32_device_controllers -I../Middlewares/ST/usbx/common/usbx_device_classes/inc -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-TouchGFX-2f-SU6000_Center-2f-touchgfx-2f-framework-2f-source-2f-touchgfx-2f-containers

clean-TouchGFX-2f-SU6000_Center-2f-touchgfx-2f-framework-2f-source-2f-touchgfx-2f-containers:
	-$(RM) ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/CacheableContainer.cyclo ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/CacheableContainer.d ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/CacheableContainer.o ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/CacheableContainer.su ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/Container.cyclo ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/Container.d ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/Container.o ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/Container.su ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/ListLayout.cyclo ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/ListLayout.d ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/ListLayout.o ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/ListLayout.su ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/ModalWindow.cyclo ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/ModalWindow.d ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/ModalWindow.o ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/ModalWindow.su ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/ScrollableContainer.cyclo ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/ScrollableContainer.d ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/ScrollableContainer.o ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/ScrollableContainer.su ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/SlideMenu.cyclo ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/SlideMenu.d ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/SlideMenu.o ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/SlideMenu.su ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/Slider.cyclo ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/Slider.d ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/Slider.o ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/Slider.su ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/SwipeContainer.cyclo ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/SwipeContainer.d ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/SwipeContainer.o ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/SwipeContainer.su ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/ZoomAnimationImage.cyclo ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/ZoomAnimationImage.d ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/ZoomAnimationImage.o ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/containers/ZoomAnimationImage.su

.PHONY: clean-TouchGFX-2f-SU6000_Center-2f-touchgfx-2f-framework-2f-source-2f-touchgfx-2f-containers

