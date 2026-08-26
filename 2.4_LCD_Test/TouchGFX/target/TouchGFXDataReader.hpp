/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : TouchGFXDataReader.hpp
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

#ifndef TouchGFXDataReader_HPP
#define TouchGFXDataReader_HPP

/* USER CODE BEGIN TouchGFXDataReader.hpp */

#include <TouchGFXGeneratedDataReader.hpp>

/**
 * This class is an abstract interface for a class reading data from a flash. The flash can be
 * any type, but is mostly used for flashes that are not memory mapped. Applications
 * must implement access to the flash through this interface.
 */
class TouchGFXDataReader : public TouchGFXGeneratedDataReader
{
public:
    /** Finalizes an instance of the TouchGFXDataReader class. */
    virtual ~TouchGFXDataReader()
    {
    }

    virtual bool addressIsAddressable(const void* address);

    virtual void copyData(const void* src, void* dst, uint32_t bytes);

    virtual void startFlashLineRead(const void* src, uint32_t bytes);

    virtual const uint8_t* waitFlashReadComplete();
};

/* USER CODE END TouchGFXDataReader.hpp */

#endif /* TouchGFXDataReader_HPP */
