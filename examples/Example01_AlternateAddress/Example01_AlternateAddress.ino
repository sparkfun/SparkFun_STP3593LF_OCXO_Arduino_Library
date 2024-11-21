/*
  Using alternate I2C addresses for the STP3593LF OCXO.

  This example shows how to use an alternate address and TwoWire port for the OCXO.

  By: Paul Clark
  SparkFun Electronics
  Date: 2024/11/21
  SparkFun code, firmware, and software is released under the MIT License.
  Please see LICENSE.md for further details.

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

  bool begun;
  begun = myOCXO.begin(Wire, 0x50); // Initialize the STP3593LF - using a custom bus and address
  begun = myOCXO.begin(0x50); // This is also possible. It defaults to Wire
  begun = myOCXO.begin(); // This is also possible. It defaults to Wire and address 0x50

  if (!begun)
  {
    Serial.println("STP3593LF not detected! Please check the address and try again...");
    while (1); // Do nothing more
  }

  // Read the frequency control word - should be zero initially
  int64_t fcw = myOCXO.getFrequencyControlWord();
  Serial.print("The frequency control word is: ");
  Serial.println(fcw);

  // Read the available (clipped) pull range
  double pullAvailable = myOCXO.getMaxPullAvailable();
  Serial.printf("Maximum frequency pull is: %e\r\n", pullAvailable);
}

void loop()
{
  // Nothing to do here
}
