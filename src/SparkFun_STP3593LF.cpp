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

#include "SparkFun_STP3593LF.h"

/// @brief Begin communication with the STP3593LF. Read the registers.
/// @return true if readRegisters is successful.
bool SfeSTP3593LFDriver::begin()
{
    if (_theBus->ping() != kSTkErrOk)
        return false;

    // Read the Clip register twice - in case the user is using the emulator
    // (This ensures the emulator registerAddress points at 0x00 correctly)
    if (readClipRegister())
        if (readClipRegister())
            // Read the registers twice - in case the user is using the emulator
            // (This ensures the emulator registerAddress points at 0x0C correctly)
            if (readRegisters())
                return readRegisters();

    return false;
}

/// @brief Read the STP3593LF OCXO Clip register and update the driver's internal copy
/// @return true if the read is successful
bool SfeSTP3593LFDriver::readClipRegister(void)
{
    uint8_t theBytes[2];
    size_t readBytes;

    // Read 2 bytes, starting at address kSfeSTP3593LFRegClip (0x00)
    if (_theBus->readRegisterRegion(kSfeSTP3593LFRegClip, (uint8_t *)&theBytes[0], 2, readBytes) != kSTkErrOk)
        return false;
    if (readBytes != 2)
        return false;

    // Extract the register contents - MSB first
    uint16_t register00 = (((uint16_t)theBytes[0]) << 8) | ((uint16_t)theBytes[1]); // OCXO Clip

    // Extract the Clip bits
    sfe_STP3593LF_reg_clip_t clipReg;
    clipReg.word = register00;
    _clip = clipReg.clip;

    return true;
}

/// @brief Read the three STP3593LF frequency control registers and update the driver's internal copies
/// @return true if the read is successful
bool SfeSTP3593LFDriver::readRegisters(void)
{
    uint8_t theBytes[6];
    size_t readBytes;
    uint16_t register0C;
    uint16_t register0D;
    uint16_t register0E;

    // Read 6 bytes, starting at address kSfeSTP3593LFRegControlMSW (0x0C)
    if (_theBus->readRegisterRegion(kSfeSTP3593LFRegControlMSW, (uint8_t *)&theBytes[0], 6, readBytes) != kSTkErrOk)
        return false;
    if (readBytes != 6)
        return false;

    // Extract the three 16-bit registers - MSB first
    register0C = (((uint16_t)theBytes[0]) << 8) | ((uint16_t)theBytes[1]); // Frequency Control MSW
    register0D = (((uint16_t)theBytes[2]) << 8) | ((uint16_t)theBytes[3]); // Frequency Control NSW
    register0E = (((uint16_t)theBytes[4]) << 8) | ((uint16_t)theBytes[5]); // Frequency Control LSW

    // Extract the frequency control bits from register0E
    sfe_STP3593LF_reg_control_lsw_t controlLSW;
    controlLSW.word = register0E;

    union // Avoid any ambiguity when converting uint64_t to int64_t
    {
        uint64_t unsigned64;
        int64_t signed64;
    } unsignedSigned64;

    unsignedSigned64.unsigned64 = ((((uint64_t)register0C) << 23)
                                | (((uint64_t)register0D) << 7)
                                | ((uint64_t)controlLSW.freqControl));

    if ((unsignedSigned64.unsigned64 & 0x0000004000000000) != 0) // Two's complement
        unsignedSigned64.unsigned64 |= 0xFFFFFFC000000000;

    _frequencyControl = unsignedSigned64.signed64; // Store the two's complement frequency control word

    return true;
}

/// @brief Get the 39-bit frequency control word - from the driver's internal copy
/// @return The 39-bit frequency control word as int64_t (signed, two's complement)
int64_t SfeSTP3593LFDriver::getFrequencyControlWord(void)
{
    return _frequencyControl;
}

/// @brief Set the 39-bit frequency control word - and update the driver's internal copy
/// @param freq the frequency control word as int64_t (signed, two's complement)
/// @return true if the write is successful
bool SfeSTP3593LFDriver::setFrequencyControlWord(int64_t freq)
{
    uint8_t theBytes[6];

    union // Avoid any ambiguity when converting uint64_t to int64_t
    {
        uint64_t unsigned64;
        int64_t signed64;
    } unsignedSigned64;
    unsignedSigned64.signed64 = freq;

    theBytes[0] = (uint8_t)((unsignedSigned64.unsigned64 >> 31) & 0xFF); // MSW MSB
    theBytes[1] = (uint8_t)((unsignedSigned64.unsigned64 >> 23) & 0xFF); // MSW LSB
    theBytes[2] = (uint8_t)((unsignedSigned64.unsigned64 >> 15) & 0xFF); // NSW MSB
    theBytes[3] = (uint8_t)((unsignedSigned64.unsigned64 >> 7) & 0xFF); // NSW LSB
    theBytes[4] = (uint8_t)((unsignedSigned64.unsigned64 << 1) & 0xFF); // LSW MSB
    theBytes[5] = 0; // LSW LSB

    if (_theBus->writeRegisterRegion(kSfeSTP3593LFRegControlMSW, (const uint8_t *)&theBytes[0], 6) != kSTkErrOk)
        return false; // Return false if the write failed

    _frequencyControl = freq; // Only update the driver's copy if the write was successful
    return true;
}

/// @brief Get the 13-bit clip value - from the driver's internal copy
/// @return The 13-bit clip as uint16_t
uint16_t SfeSTP3593LFDriver::getPullRangeClip(void)
{
    return _clip;
}

/// @brief Get the clip value - from the driver's internal copy. Convert to maximum pull available
/// @return The clip value converted to maximum pull available (double)
double SfeSTP3593LFDriver::getMaxPullAvailable(void)
{
    // If the DCXO_Clip value is 0, the DCOCXO pull range is ±800 ppm
    if (_clip == 0)
        return _maxPullRange;

    double clip = _clip; // Convert _clip to double
    clip /= pow(2, 13); // Convert to fraction of 2^13
    clip *= _maxPullRange; // Convert to maximum pull range
    return clip;
}

/// @brief Get the base oscillator frequency - from the driver's internal copy
/// @return The oscillator base frequency as double
double SfeSTP3593LFDriver::getBaseFrequencyHz(void)
{
    return _baseFrequencyHz;
}

/// @brief Set the base oscillator frequency in Hz - set the driver's internal _baseFrequencyHz
/// @param freq the base frequency in Hz
void SfeSTP3593LFDriver::setBaseFrequencyHz(double freq)
{
    _baseFrequencyHz = freq;
}

/// @brief Get the oscillator frequency based on the base frequency and control word
/// @return The oscillator frequency as double
double SfeSTP3593LFDriver::getFrequencyHz(void)
{
    double freqControl = (double)_frequencyControl; // Convert signed to double

    if (freqControl >= 0.0)
        freqControl /= pow(2, 38) - 1.; // Scale 0.0 to 1.0
    else
        freqControl /= pow(2, 38); // Scale 0.0 to -1.0

    double freqOffsetHz = _baseFrequencyHz * freqControl * _maxPullRange;

    double freqHz = _baseFrequencyHz + freqOffsetHz;

    return freqHz;
}

/// @brief Set the oscillator frequency based on the base frequency and pull range
/// @param freq the oscillator frequency in Hz
/// @return true if the write is successful
/// Note: The frequency change will be limited by the pull range capabilities of the device.
///       Call getFrequencyHz to read the frequency set.
/// Note: setFrequencyHz ignores _maxFrequencyChangePPB.
bool SfeSTP3593LFDriver::setFrequencyHz(double freq)
{
    // Calculate the frequency offset from the base frequency
    double freqOffsetHz = freq - _baseFrequencyHz;

    // Calculate the maximum frequency offset in Hz, based on the maximum pull range
    double maxPullHz = _baseFrequencyHz * _maxPullRange;

    // Calculate the maximum frequency offset in Hz, based on the available pull range
    double maxPullClippedHz = _baseFrequencyHz * getMaxPullAvailable();

    // Limit freqOffsetHz to maxPullClippedHz
    if (freqOffsetHz >= 0.0)
    {
        if (freqOffsetHz > maxPullClippedHz)
            freqOffsetHz = maxPullClippedHz;
    }
    else
    {
        if (freqOffsetHz < (0.0 - maxPullClippedHz))
            freqOffsetHz = 0.0 - maxPullClippedHz;
    }

    double freqControl = freqOffsetHz / maxPullHz;

    if (freqControl >= 0.0)
    {
        if (freqControl > 1.0)
            freqControl = 1.0;

        freqControl *= pow(2, 38) - 1;
    }
    else
    {
        if (freqControl < -1.0)
            freqControl = -1.0;

        freqControl *= pow(2, 38);
    }

    int64_t freqControlInt = (int64_t)freqControl;

    // Just in case, ensure freqControlInt is limited to 2^38 (39-bits signed)
    if (freqControlInt > 274877906943)
        freqControlInt = 274877906943;

    if (freqControlInt < -274877906944)
        freqControlInt = -274877906944;

    return setFrequencyControlWord(freqControlInt);
}

/// @brief Get the maximum frequency change in PPB
/// @return The maximum frequency change in PPB - from the driver's internal store
double SfeSTP3593LFDriver::getMaxFrequencyChangePPB(void)
{
    return _maxFrequencyChangePPB;
}

/// @brief Set the maximum frequency change in PPB - set the driver's internal _maxFrequencyChangePPB
/// @param ppb the maximum frequency change in PPB
void SfeSTP3593LFDriver::setMaxFrequencyChangePPB(double ppb)
{
    _maxFrequencyChangePPB = ppb;
}

/// @brief Set the frequency according to the GNSS receiver clock bias in milliseconds
/// @param bias the GNSS RX clock bias in milliseconds
/// @param Pk the Proportional term
/// @param Ik the Integral term
/// @return true if the write is successful
/// Note: the frequency change will be limited by: the pull range capabilities of the device;
///       and the setMaxFrequencyChangePPB. Call getFrequencyHz to read the frequency set.
/// The default values for Pk and Ik come from very approximate Ziegler-Nichols tuning:
/// oscillation starts when Pk is TODO; with a period of TODO seconds.
bool SfeSTP3593LFDriver::setFrequencyByBiasMillis(double bias, double Pk, double Ik)
{
    double freq = getFrequencyHz();

    static double I;
    static bool initialized = false;
    if (!initialized)
    {
        I = freq; // Initialize I with the current frequency for a more reasonable startup
        initialized = true;
    }

    double clockInterval_s = 1.0 / freq; // Convert freq to interval in seconds

    // Our setpoint is zero. Bias is the process value. Convert it to error
    double error = 0.0 - bias;

    double errorInClocks = error / 1000.0; // Convert error from millis to seconds
    errorInClocks /= clockInterval_s; // Convert error to clock cycles

    // Calculate the maximum frequency change in clock cycles
    double maxChangeInClocks = freq * _maxFrequencyChangePPB * 1.0e-9;

    // Limit errorInClocks to +/-maxChangeInClocks
    if (errorInClocks >= 0.0)
    {
        if (errorInClocks > maxChangeInClocks)
            errorInClocks = maxChangeInClocks;
    }
    else
    {
        if (errorInClocks < (0.0 - maxChangeInClocks))
            errorInClocks = 0.0 - maxChangeInClocks;
    }

    double P = errorInClocks * Pk;
    double dI = errorInClocks * Ik;
    I += dI; // Add the delta to the integral

    return setFrequencyHz(P + I); // Set the frequency to proportional plus integral
}

/// @brief  PROTECTED: update the local pointer to the I2C bus.
/// @param  theBus Pointer to the bus object.
void SfeSTP3593LFDriver::setCommunicationBus(sfeTkArdI2C *theBus)
{
    _theBus = theBus;
}
