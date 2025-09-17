################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/AbstractDataGraph.cpp \
../TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/Graph.cpp \
../TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/GraphElements.cpp \
../TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/GraphLabels.cpp \
../TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/GraphScroll.cpp \
../TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/GraphWrapAndClear.cpp \
../TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/GraphWrapAndOverwrite.cpp 

OBJS += \
./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/AbstractDataGraph.o \
./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/Graph.o \
./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/GraphElements.o \
./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/GraphLabels.o \
./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/GraphScroll.o \
./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/GraphWrapAndClear.o \
./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/GraphWrapAndOverwrite.o 

CPP_DEPS += \
./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/AbstractDataGraph.d \
./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/Graph.d \
./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/GraphElements.d \
./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/GraphLabels.d \
./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/GraphScroll.d \
./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/GraphWrapAndClear.d \
./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/GraphWrapAndOverwrite.d 


# Each subdirectory must supply rules for building sources it contributes
TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/%.o TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/%.su TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/%.cyclo: ../TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/%.cpp TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m33 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32U5G9xx -DUX_INCLUDE_USER_DEFINE_FILE -c -I../Core/Inc -I../TouchGFX/App -I../TouchGFX/target/generated -I../TouchGFX/target -I../Drivers/STM32U5xx_HAL_Driver/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../Drivers/CMSIS/Include -I../USBX/App -I../USBX/Target -I../Middlewares/ST/usbx/common/core/inc -I../Middlewares/ST/usbx/ports/generic/inc -I../Middlewares/ST/usbx/common/usbx_stm32_device_controllers -I../Middlewares/ST/usbx/common/usbx_device_classes/inc -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-TouchGFX-2f-SU6000_Center-2f-touchgfx-2f-framework-2f-source-2f-touchgfx-2f-widgets-2f-graph

clean-TouchGFX-2f-SU6000_Center-2f-touchgfx-2f-framework-2f-source-2f-touchgfx-2f-widgets-2f-graph:
	-$(RM) ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/AbstractDataGraph.cyclo ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/AbstractDataGraph.d ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/AbstractDataGraph.o ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/AbstractDataGraph.su ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/Graph.cyclo ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/Graph.d ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/Graph.o ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/Graph.su ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/GraphElements.cyclo ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/GraphElements.d ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/GraphElements.o ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/GraphElements.su ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/GraphLabels.cyclo ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/GraphLabels.d ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/GraphLabels.o ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/GraphLabels.su ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/GraphScroll.cyclo ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/GraphScroll.d ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/GraphScroll.o ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/GraphScroll.su ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/GraphWrapAndClear.cyclo ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/GraphWrapAndClear.d ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/GraphWrapAndClear.o ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/GraphWrapAndClear.su ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/GraphWrapAndOverwrite.cyclo ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/GraphWrapAndOverwrite.d ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/GraphWrapAndOverwrite.o ./TouchGFX/SU6000_Center/touchgfx/framework/source/touchgfx/widgets/graph/GraphWrapAndOverwrite.su

.PHONY: clean-TouchGFX-2f-SU6000_Center-2f-touchgfx-2f-framework-2f-source-2f-touchgfx-2f-widgets-2f-graph

