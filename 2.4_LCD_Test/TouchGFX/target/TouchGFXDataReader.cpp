/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : TouchGFXDataReader.cpp
  ******************************************************************************
  * This file was created by TouchGFX Generator 4.26.1. This file is only
  * generated once! Delete this file from your project and re-generate code
  * using STM32CubeMX or change this file manually to update it.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#include <TouchGFXDataReader.hpp>

/* USER CODE BEGIN TouchGFXDataReader.cpp */

/* The C hooks required by the DataReader strategy -
 *   DataReader_WaitForReceiveDone()
 *   DataReader_ReadData(uint32_t address24, uint8_t* buffer, uint32_t length)
 *   DataReader_StartDMAReadData(uint32_t address24, uint8_t* buffer, uint32_t length)
 * - are implemented in Core/Src/flash_driver.c (MX25L6433F over SPI2 + DMA). */

bool TouchGFXDataReader::addressIsAddressable(const void* address)
{
    return TouchGFXGeneratedDataReader::addressIsAddressable(address);
}

void TouchGFXDataReader::copyData(const void* src, void* dst, uint32_t bytes)
{
    TouchGFXGeneratedDataReader::copyData(src, dst, bytes);
}

void TouchGFXDataReader::startFlashLineRead(const void* src, uint32_t bytes)
{
    TouchGFXGeneratedDataReader::startFlashLineRead(src, bytes);
}

const uint8_t* TouchGFXDataReader::waitFlashReadComplete()
{
    return TouchGFXGeneratedDataReader::waitFlashReadComplete();
}

/* USER CODE END TouchGFXDataReader.cpp */
