/*
  Set the frequency of the STP3593LF OCXO from RX clock delay.

  This example demonstrates how to set the frequency of the STP3593LF OCXO from a GNSS
  RX Clock Bias (in milliseconds).

  By: Paul Clark
  SparkFun Electronics
  Date: 2024/11/21
  SparkFun code, firmware, and software is released under the MIT License.
  Please see LICENSE.md for further details.

  Consider the STP3593LFAI-KYG33IV-10.000000:
  The operating temperature range is Industrial, -40 to 85°C (option "I").
  LVCMOS output (option "-").
  Frequency stability +/-1ppb (option "Y").
  It is DCOCXO with a configurable I2C address (option "G").
  Supply voltage 3.3V (option "33").
  Pin 1 is Output Enable (option "I"). (No software OE control).
  The pull range limit is 3.125ppm (option "V").
  Base frequency is 10.000000MHz.

  Consider this example:
  * The OCXO frequency has not yet been changed. It is running at the default 10.000000 MHz.
  * 10.000000 MHZ is the library default base frequency. But we could set it with setBaseFrequencyHz(10000000.0)
  * The available pull range is read during begin.
  * The mosaic-T manual states that the oscillator frequency should be changed by no more than 3ppb per second.
  * We tell the library this using setMaxFrequencyChangePPB(3.0)
  * The GNSS RxClkBias reports that receiver time is ahead of system time by 200 nanoseconds (+200ns).
  * We instruct the library to change the frequency using setFrequencyByBiasMillis(200.0e-6)
  * The OCXO clock period is 100ns.
  * The 200ns bias corresponds to 2 clock cycles.
  * To remove that bias in one second, the oscillator frequency would need to be reduced to 9.999998 MHz.
  * That is a change of 2 parts in 10000000, or 0.2ppm, or 200ppb.
  * The frequency change will be limited to 3ppb.
  * Since the STP3593LF maximum Pull Range is 800ppm, and the Pull Register is 39-bit signed, 3ppb corresponds to 1030792 LSB.
  * The firmware writes the value -1030792 to the Frequency Control Register, reducing the frequency to 9.99999997 MHz.
  * getFrequencyHz will return 9999999.97

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

  myOCXO.setBaseFrequencyHz(10000000.0); // Pass the oscillator base frequency into the driver

  Serial.print("Base frequency set to ");
  Serial.print(myOCXO.getBaseFrequencyHz());
  Serial.println(" Hz");

  myOCXO.setMaxFrequencyChangePPB(3.0); // Set the maximum frequency change in PPB

  Serial.print("Maximum frequency change set to ");
  Serial.print(myOCXO.getMaxFrequencyChangePPB());
  Serial.println(" PPB");

  Serial.print("Frequency control word should be 0. It is ");
  Serial.println(myOCXO.getFrequencyControlWord());

  Serial.println("Applying a clock bias of +200ns");
  // Set the frequency by clock bias (+200ns, +200e-6ms)
  // For this test, set the P term to 1.0 and the I term to 0.0
  myOCXO.setFrequencyByBiasMillis(200.0e-6, 1.0, 0.0);

  Serial.print("Frequency should be 9999999.97 Hz. It is ");
  Serial.print(myOCXO.getFrequencyHz());
  Serial.println(" Hz");

  Serial.print("Frequency control word should be -1030792. It is ");
  Serial.println(myOCXO.getFrequencyControlWord());
}

void loop()
{
  // Nothing to do here
}
