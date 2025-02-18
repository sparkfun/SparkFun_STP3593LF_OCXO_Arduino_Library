/*
    SparkFun STP3593LF OCXO Arduino Library

    Repository
    https://github.com/sparkfun/SparkFun_STP3593LF_OCXO_Arduino_Library

    SPDX-License-Identifier: MIT

    Copyright (c) 2024 SparkFun Electronics

    Name: SparkFun_STP3593LF.h

    Description:
    An Arduino Library for the STP3593LF Digitally-Controlled
    Double-Oven Crystal Oscillator from rakon.
    Requires the SparkFun Toolkit:
    https://github.com/sparkfun/SparkFun_Toolkit

    Notes - based on the _preliminary_ datasheet from rakon:
    Register 0x41 : Read Frequency Control : reports the 32-bit unsigned frequency control word
    Range is 0x00000000 to 0x000F4240 (1000000) with 8E-13 typical frequency variation per step
    Register 0xA0 : Write DAC 20 bits : allows the DAC value (frequency control) to be written
    Register 0xC2 : Save Frequency Control Value : saves the frequency control DAC value (0xA0)
                    so it can be reloaded on startup

    How It Works:

    On SiTime parts, the frequency control word is signed. The oscillator will output
    (close to) its base frequency when the control word is zero. The frequency can be
    pulled in either direction by changing the control word.

    On this rakon part, the frequency control word is unsigned and in the range
    0 - 1000000.

    We could assume that the oscillator will output (close to) its base
    frequency when the control word is 500000. But that's not necessarily true.

    Since we are continuously driving the frequency to 10MHz under GNSS control,
    we can assume that the saved control value - which gets reloaded on start-up -
    provides (very close to) 10MHz output. We could read the Read Frequency Control
    register at start-up and assume that value produces 10MHz. But that's not necessarily
    true either.

    All we need to know is that:
    Frequency Control can be adjusted in the range 0 - 1000000.
    The frequency resolution is 8E-13 per step / LSB.
    I.e. the frequency can be pulled in the range 0 - 800ppb; +/-400ppb.
    (The frequency calibration is quoted as +/-50ppb at time of shipment
     and the 10 year life-time accuracy is quoted as +/-350ppb.)

    For this oscillator, getFrequencyHz, setFrequencyHz, getBaseFrequencyHz,
    setBaseFrequencyHz don't apply.

*/

#pragma once

#include <stdint.h>

#include <Arduino.h>
// .. some header order issue right now...
// clang-format off
#include <SparkFun_Toolkit.h>
#include "sfTk/sfDevSTP3593LF.h"
// clang-format on

class SfeSTP3593LFArdI2C : public sfDevSTP3593LF
{
  public:
    SfeSTP3593LFArdI2C()
    {
    }

    /// @brief  Sets up Arduino I2C driver using the default I2C address then calls the super class begin.
    /// @return True if successful, false otherwise.
    // bool begin(void)
    // {
    //     if (_theI2CBus.init(kDefaultSTP3593LFAddr) != kSTkErrOk)
    //         return false;

    //     setCommunicationBus(&_theI2CBus);

    //     _theI2CBus.setStop(false); // Use restarts not stops for I2C reads

    //     return SfeSTP3593LFDriver::begin();
    // }

    /// @brief  Sets up Arduino I2C driver using the specified I2C address then calls the super class begin.
    /// @return True if successful, false otherwise.
    bool begin(const uint8_t address = kDefaultSTP3593LFAddr)
    {
        if (_theI2CBus.init(address) != ksfTkErrOk)
            return false;

        return beginDevice();
    }

    /// @brief  Sets up Arduino I2C driver using the specified I2C address then calls the super class begin.
    /// @return True if successful, false otherwise.
    bool begin(TwoWire &wirePort, const uint8_t address = kDefaultSTP3593LFAddr)
    {
        if (_theI2CBus.init(wirePort, address) != ksfTkErrOk)
            return false;

        return beginDevice();
    }

  private:
    bool beginDevice(void)
    {

        // the intent is that the bus is setup and we can see if the device is connected
        if (_theI2CBus.ping() == ksfTkErrOk)
            return false;

        _theI2CBus.setStop(false); // Use restarts not stops for I2C reads

        return sfDevSTP3593LF::begin(&_theI2CBus) == ksfTkErrOk;
    }

    sfTkArdI2C _theI2CBus;
};
