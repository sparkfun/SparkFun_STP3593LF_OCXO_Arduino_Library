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
#include <SparkFun_Toolkit.h>

///////////////////////////////////////////////////////////////////////////////
// I2C Addressing
///////////////////////////////////////////////////////////////////////////////
// The STP3593LF has a fixed address of 0xE0 (shifted), 0x70 (unshifted).
const uint8_t kDefaultSTP3593LFAddr = 0x70;

///////////////////////////////////////////////////////////////////////////////
// 32-bit Register Addresses
///////////////////////////////////////////////////////////////////////////////

const uint8_t kSfeSTP3593LFRegReadFrequencyControl = 0x41; // Read Frequency Control
const uint8_t kSfeSTP3593LFRegWriteDAC = 0xA0; // Write DAC 20-bits (0-1000000)
const uint8_t kSfeSTP3593LFRegSaveFrequency = 0xC2; // Save Frequency Control Value

///////////////////////////////////////////////////////////////////////////////

const uint32_t kSfeSTP3593LFFreqControlMaxValue = 1000000;
const double kSfeSTP3593LFFreqControlResolution = 8e-13;

///////////////////////////////////////////////////////////////////////////////

class SfeSTP3593LFDriver
{
public:
    // @brief Constructor. Instantiate the driver object using the specified address (if desired).
    SfeSTP3593LFDriver()
        : _maxFrequencyChangePPB{400.0}
    {
    }

    /// @brief Begin communication with the STP3593LF. Read the registers.
    /// @return true if readRegisters is successful.
    bool begin(void);


    /// @brief Read the STP3593LF OCXO frequency control register and update the driver's internal copy
    /// @return true if the read is successful
    bool readFrequencyControlWord(void);

    /// @brief Get the 20-bit frequency control word - from the driver's internal copy
    /// @return The 20-bit frequency control word as uint32_t (unsigned)
    uint32_t getFrequencyControlWord(void);

    /// @brief Set the 20-bit frequency control word - and update the driver's internal copy
    /// @param freq the frequency control word as uint32_t (unsigned)
    /// @return true if the write is successful
    bool setFrequencyControlWord(uint32_t freq);


    /// @brief Get the maximum frequency change in PPB
    /// @return The maximum frequency change in PPB - from the driver's internal store
    double getMaxFrequencyChangePPB(void);

    /// @brief Set the maximum frequency change in PPB - set the driver's internal _maxFrequencyChangePPB
    /// @param ppb the maximum frequency change in PPB
    void setMaxFrequencyChangePPB(double ppb);


    /// @brief Set the frequency according to the GNSS receiver clock bias in milliseconds
    /// @param bias the GNSS RX clock bias in milliseconds
    /// @param Pk the Proportional term
    /// @param Ik the Integral term
    /// @return true if the write is successful
    /// Note: the frequency change will be limited by: the pull range capabilities of the device;
    ///       and the setMaxFrequencyChangePPB.
    /// The default values for Pk and Ik come from testing by Fugro:
    bool setFrequencyByBiasMillis(double bias, double Pk = 1.0 / 6.25, double Ik = (1.0 / 6.25) / 150.0);


    /// @brief Save the frequency control value - to be reloaded at start-up
    /// @return true if the write is successful
    bool saveFrequencyControlValue(void);

protected:
    /// @brief Sets the communication bus to the specified bus.
    /// @param theBus Bus to set as the communication devie.
    void setCommunicationBus(sfeTkArdI2C *theBus);

private:
    sfeTkArdI2C *_theBus; // Pointer to bus device.

    uint32_t _frequencyControl; // Local store for the frequency control word. 20-Bit
    double _maxFrequencyChangePPB; // The maximum frequency change in PPB for setFrequencyByBiasMillis
};

class SfeSTP3593LFArdI2C : public SfeSTP3593LFDriver
{
public:
    SfeSTP3593LFArdI2C()
    {
    }

    /// @brief  Sets up Arduino I2C driver using the default I2C address then calls the super class begin.
    /// @return True if successful, false otherwise.
    bool begin(void)
    {
        if (_theI2CBus.init(kDefaultSTP3593LFAddr) != kSTkErrOk)
            return false;

        setCommunicationBus(&_theI2CBus);

        _theI2CBus.setStop(false); // Use restarts not stops for I2C reads

        return SfeSTP3593LFDriver::begin();
    }

    /// @brief  Sets up Arduino I2C driver using the specified I2C address then calls the super class begin.
    /// @return True if successful, false otherwise.
    bool begin(const uint8_t &address)
    {
        if (_theI2CBus.init(address) != kSTkErrOk)
            return false;

        setCommunicationBus(&_theI2CBus);

        _theI2CBus.setStop(false); // Use restarts not stops for I2C reads

        return SfeSTP3593LFDriver::begin();
    }

    /// @brief  Sets up Arduino I2C driver using the specified I2C address then calls the super class begin.
    /// @return True if successful, false otherwise.
    bool begin(TwoWire &wirePort, const uint8_t &address)
    {
        if (_theI2CBus.init(wirePort, address) != kSTkErrOk)
            return false;

        setCommunicationBus(&_theI2CBus);

        _theI2CBus.setStop(false); // Use restarts not stops for I2C reads

        return SfeSTP3593LFDriver::begin();
    }

private:
    sfeTkArdI2C _theI2CBus;
};
