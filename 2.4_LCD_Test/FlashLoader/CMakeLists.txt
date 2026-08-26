cmake_minimum_required(VERSION 3.22)

# STM32CubeProgrammer external loader: MX25L6433F on STM32G0B1 (SPI2).
# Standalone project - reuses the parent application's HAL drivers.
#
# Pin map: -DLOADER_PINMAP=1 (default, NUCLEO-G0B1RE + GFX01M2: MISO PC2/MOSI PC3)
#          -DLOADER_PINMAP=2 (custom G0B1CBT6 board: MISO PB14/MOSI PB15)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

project(MX25L6433F_G0B1_Loader C ASM)

set(APP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

if(NOT DEFINED LOADER_PINMAP)
    set(LOADER_PINMAP 1)
endif()

add_executable(loader
    Loader_Src.c
    Dev_Inf.c
    ${APP_DIR}/Core/Src/system_stm32g0xx.c
    ${APP_DIR}/Core/Src/syscalls.c
    ${APP_DIR}/Core/Src/sysmem.c
    ${APP_DIR}/Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal.c
    ${APP_DIR}/Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_cortex.c
    ${APP_DIR}/Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_gpio.c
    ${APP_DIR}/Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_rcc.c
    ${APP_DIR}/Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_rcc_ex.c
    ${APP_DIR}/Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_pwr.c
    ${APP_DIR}/Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_pwr_ex.c
    ${APP_DIR}/Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_dma.c
    ${APP_DIR}/Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_spi.c
    ${APP_DIR}/Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_spi_ex.c
)

target_include_directories(loader PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${APP_DIR}/Core/Inc
    ${APP_DIR}/Drivers/STM32G0xx_HAL_Driver/Inc
    ${APP_DIR}/Drivers/STM32G0xx_HAL_Driver/Inc/Legacy
    ${APP_DIR}/Drivers/CMSIS/Device/ST/STM32G0xx/Include
    ${APP_DIR}/Drivers/CMSIS/Include
)

target_compile_definitions(loader PRIVATE
    USE_HAL_DRIVER
    STM32G0B1xx
    LOADER_PINMAP=${LOADER_PINMAP}
)

target_link_options(loader PRIVATE "-T${CMAKE_CURRENT_SOURCE_DIR}/loader.ld" "-Wl,-Map=loader.map")

# Self-test firmware: same flash logic built as a normal internal-flash app.
# Flash it, let it run, read results at 0x20020000 over SWD.
add_executable(loader_selftest
    Loader_Src.c
    Dev_Inf.c
    ${APP_DIR}/Core/Src/system_stm32g0xx.c
    ${APP_DIR}/Core/Src/syscalls.c
    ${APP_DIR}/Core/Src/sysmem.c
    ${APP_DIR}/startup_stm32g0b1xx.s
    ${APP_DIR}/Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal.c
    ${APP_DIR}/Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_cortex.c
    ${APP_DIR}/Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_gpio.c
    ${APP_DIR}/Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_rcc.c
    ${APP_DIR}/Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_rcc_ex.c
    ${APP_DIR}/Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_pwr.c
    ${APP_DIR}/Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_pwr_ex.c
    ${APP_DIR}/Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_dma.c
    ${APP_DIR}/Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_spi.c
    ${APP_DIR}/Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_spi_ex.c
)
target_include_directories(loader_selftest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${APP_DIR}/Core/Inc
    ${APP_DIR}/Drivers/STM32G0xx_HAL_Driver/Inc
    ${APP_DIR}/Drivers/STM32G0xx_HAL_Driver/Inc/Legacy
    ${APP_DIR}/Drivers/CMSIS/Device/ST/STM32G0xx/Include
    ${APP_DIR}/Drivers/CMSIS/Include
)
target_compile_definitions(loader_selftest PRIVATE
    USE_HAL_DRIVER
    STM32G0B1xx
    LOADER_PINMAP=${LOADER_PINMAP}
    LOADER_SELFTEST
)
target_link_options(loader_selftest PRIVATE "-T${APP_DIR}/STM32G0B1xx_FLASH.ld" "-Wl,-Map=loader_selftest.map")

# The .stldr is simply the ELF with a different extension
if(LOADER_PINMAP EQUAL 2)
    set(STLDR_NAME MX25L6433F_2.4LCD_G0B1CB_CUSTOM.stldr)
else()
    set(STLDR_NAME MX25L6433F_2.4LCD_G0B1RE_NUCLEO.stldr)
endif()

add_custom_command(TARGET loader POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:loader> ${CMAKE_CURRENT_SOURCE_DIR}/${STLDR_NAME}
    COMMENT "Producing ${STLDR_NAME}"
)
