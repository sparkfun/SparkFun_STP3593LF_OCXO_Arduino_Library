/*
    SparkFun STP3593LF OCXO Arduino Library

    Repository
    https://github.com/sparkfun/SparkFun_STP3593LF_OCXO_Arduino_Library

    SPDX-License-Identifier: MIT

    Copyright (c) 2024 SparkFun Electronics

    Name: SparkFun_STP3593LF.cpp

    Description:
    An Arduino Library for the STP3593LF Digitally-Controlled
    Oven-Compensated Crystal Oscillator from SiTime.
    Requires the SparkFun Toolkit:
    https://github.com/sparkfun/SparkFun_Toolkit

*/

#include "sfDevSTP3593LF.h"

/// @brief Begin communication with the STP3593LF. Read the registers.
/// @return true if readRegisters is successful.
sfTkError_t sfDevSTP3593LF::begin(sfTkII2C *commBus)
{
    if (commBus == nullptr)
        return ksfTkErrFail;

    _theBus = commBus;

    if (_theBus->ping() != ksfTkErrOk)
        return ksfTkErrFail;

    // Read the frequency control register twice - in case the user is using the emulator
    // (This ensures the emulator registerAddress points at 0x41 correctly)
    if (readFrequencyControlWord())
        return (readFrequencyControlWord() ? ksfTkErrOk : ksfTkErrFail);

    return ksfTkErrFail;
}

/// @brief Read the STP3593LF OCXO frequency control register and update the driver's internal copy
/// @return true if the read is successful
bool sfDevSTP3593LF::readFrequencyControlWord(void)
{
    uint8_t theBytes[4];
    size_t readBytes;

    // Read 4 bytes, starting at address kSfeSTP3593LFRegReadFrequencyControl (0x41)
    if (_theBus->readRegisterRegion(kSfeSTP3593LFRegReadFrequencyControl, (uint8_t *)&theBytes[0], 4, readBytes) !=
        ksfTkErrOk)
        return false;
    if (readBytes != 4)
        return false;

    // Extract the control word - MSB first
    uint32_t frequencyControl = (((uint32_t)theBytes[0]) << 24);
    frequencyControl |= (((uint32_t)theBytes[1]) << 16);
    frequencyControl |= (((uint32_t)theBytes[2]) << 8);
    frequencyControl |= (((uint32_t)theBytes[3]) << 0);

    // Check the control word is within bounds
    if (frequencyControl > kSfeSTP3593LFFreqControlMaxValue)
        return false;

    _frequencyControl = frequencyControl;

    return true;
}

/// @brief Get the 20-bit frequency control word - from the driver's internal copy
/// @return The 20-bit frequency control word as uint32_t (unsigned)
uint32_t sfDevSTP3593LF::getFrequencyControlWord(void)
{
    return _frequencyControl;
}

/// @brief Set the 20-bit frequency control word - and update the driver's internal copy
/// @param freq the frequency control word as uint32_t (unsigned)
/// @return true if the write is successful
bool sfDevSTP3593LF::setFrequencyControlWord(uint32_t freq)
{
    uint8_t theBytes[4];

    // Limit the control word if needed
    if (freq > kSfeSTP3593LFFreqControlMaxValue)
        freq = kSfeSTP3593LFFreqControlMaxValue;

    theBytes[0] = (uint8_t)((freq >> 24) & 0xFF); // MSB first
    theBytes[1] = (uint8_t)((freq >> 16) & 0xFF);
    theBytes[2] = (uint8_t)((freq >> 8) & 0xFF);
    theBytes[3] = (uint8_t)((freq >> 0) & 0xFF);

    if (_theBus->writeRegisterRegion(kSfeSTP3593LFRegWriteDAC, (const uint8_t *)&theBytes[0], 4) != ksfTkErrOk)
        return false; // Return false if the write failed

    _frequencyControl = freq; // Only update the driver's copy if the write was successful
    return true;
}

/// @brief Get the maximum frequency change in PPB
/// @return The maximum frequency change in PPB - from the driver's internal store
double sfDevSTP3593LF::getMaxFrequencyChangePPB(void)
{
    return _maxFrequencyChangePPB;
}

/// @brief Set the maximum frequency change in PPB - set the driver's internal _maxFrequencyChangePPB
/// @param ppb the maximum frequency change in PPB
void sfDevSTP3593LF::setMaxFrequencyChangePPB(double ppb)
{
    _maxFrequencyChangePPB = ppb;
}

/// @brief Set the frequency according to the GNSS receiver clock bias in milliseconds
/// @param bias the GNSS RX clock bias in milliseconds
/// @param Pk the Proportional term
/// @param Ik the Integral term
/// @return true if the write is successful
/// Note: the frequency change will be limited by: the pull range capabilities of the device;
///       and the setMaxFrequencyChangePPB.
bool sfDevSTP3593LF::setFrequencyByBiasMillis(double bias, double Pk, double Ik)
{
    static double I;
    static bool initialized = false;
    if (!initialized)
    {
        I = (double)_frequencyControl; // Initialize I with the current control word for a more reasonable startup
        initialized = true;
    }

    // Our setpoint is zero. Bias is the process value. Convert it to error
    double error = 0.0 - bias;

    // Convert error from millis to seconds
    error /= 1000.0;

    // Convert the error to control word LSBs
    double requiredChangeInLSBs = error / kSfeSTP3593LFFreqControlResolution;

    // Calculate the maximum change in control word LSBs
    double maxChangeInLSBs = _maxFrequencyChangePPB * 1.0e-9 / kSfeSTP3593LFFreqControlResolution;

    // Limit requiredChangeInLSBs to +/-maxChangeInLSBs
    if (requiredChangeInLSBs >= 0.0)
    {
        if (requiredChangeInLSBs > maxChangeInLSBs)
            requiredChangeInLSBs = maxChangeInLSBs;
    }
    else
    {
        if (requiredChangeInLSBs < (0.0 - maxChangeInLSBs))
            requiredChangeInLSBs = 0.0 - maxChangeInLSBs;
    }

    double P = requiredChangeInLSBs * Pk;
    double dI = requiredChangeInLSBs * Ik;
    I += dI; // Add the delta to the integral

    return setFrequencyControlWord((uint32_t)round(P + I)); // Set the control word to proportional plus integral
}

/// @brief Save the frequency control value - to be reloaded at start-up
/// @return true if the write is successful
bool sfDevSTP3593LF::saveFrequencyControlValue(void)
{
    bool result = true;
    result &= _theBus->writeByte(kSfeSTP3593LFRegSaveFrequency);
    if (result)
        result &= readFrequencyControlWord();
    return result;
}

/// @brief  PROTECTED: update the local pointer to the I2C bus.
/// @param  theBus Pointer to the bus object.
void sfDevSTP3593LF::setCommunicationBus(sfTkII2C *theBus)
{
    _theBus = theBus;
}
