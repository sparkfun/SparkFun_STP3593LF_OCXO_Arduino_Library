/*
  Set the frequency of the STP3593LF OCXO from RX clock delay.

  This example demonstrates how to set the frequency of the STP3593LF OCXO from a GNSS
  RX Clock Bias (in milliseconds).

  By: Paul Clark
  SparkFun Electronics
  Date: 2024/11/21
  SparkFun code, firmware, and software is released under the MIT License.
  Please see LICENSE.md for further details.

  Consider this example:
  * The mosaic-T manual states that the oscillator frequency should be changed by no more than 3ppb per second.
  * We tell the library this using setMaxFrequencyChangePPB(3.0)
  * The GNSS RxClkBias reports that receiver time is ahead of system time by 200 nanoseconds (+200ns).
  * We instruct the library to change the frequency using setFrequencyByBiasMillis(200.0e-6)
  * The OCXO clock period is ~100ns.
  * The 200ns bias corresponds to 2 clock cycles.
  * To remove that bias in one second, the oscillator frequency would need to be reduced to 9999998 Hz.
  * That is a change of 2 parts in 10000000, or 0.2ppm, or 200ppb.
  * The frequency change will be limited to 3ppb.
  * Since the resolution of the frequency control word is 8E-13, the frequency control word should decrease by 3750

*/

// You will need the SparkFun Toolkit. Click here to get it: http://librarymanager/All#SparkFun_Toolkit

#include <SparkFun_STP3593LF.h> // Click here to get the library: http://librarymanager/All#SparkFun_STP3593LF

SfeSTP3593LFArdI2C myOCXO;

void setup()
{
  delay(1000); // Allow time for the microcontroller to start up

  Serial.begin(115200); // Begin the Serial console
  while (!Serial)
  {
    delay(100); // Wait for the user to open the Serial Monitor
  }
  Serial.println("SparkFun STP3593LF Example");

  Wire.begin(); // Begin the I2C bus

  if (!myOCXO.begin())
  {
    Serial.println("STP3593LF not detected! Please check the address and try again...");
    while (1); // Do nothing more
  }

  myOCXO.setMaxFrequencyChangePPB(3.0); // Set the maximum frequency change in PPB

  Serial.print("Maximum frequency change set to ");
  Serial.print(myOCXO.getMaxFrequencyChangePPB());
  Serial.println(" PPB");

  uint32_t controlWord = myOCXO.getFrequencyControlWord();
  Serial.print("Frequency control word is currently ");
  Serial.println(controlWord);

  Serial.println("Applying a clock bias of +200ns");
  // Set the frequency by clock bias (+200ns, +200e-6ms)
  // For this test, set the P term to 1.0 and the I term to 0.0
  myOCXO.setFrequencyByBiasMillis(200.0e-6, 1.0, 0.0);

  Serial.print("Frequency control word should be ");
  Serial.print(controlWord - 3750); // See notes above
  Serial.print(". It is ");
  Serial.println(myOCXO.getFrequencyControlWord());
}

void loop()
{
  // Nothing to do here
}
