/*
  Emulate the STP3593LF OCXO.

  By: Paul Clark
  SparkFun Electronics
  Date: 2024/11/21
  SparkFun code, firmware, and software is released under the MIT License.
  Please see LICENSE.md for further details.

  Note: This code doesn't update the registerAddress correctly - on ESP32 (3.0.1).
        registerAddress is only updated _after_ the registers have been read...

*/

#include <Wire.h>

#define I2C_DEV_ADDR 0x70

// Emulate a single 32-bit register for reading and writing
// Although, in reality, data is read from 0x41 and written to 0xA0
#define NUM_REG_BYTES (4)
volatile uint8_t registerBytes[NUM_REG_BYTES] = { 0x00, 0x07, 0xA1, 0x20 }; // Default to 500000 (MSB first)
volatile uint8_t registerAddress = 0;

// On Request:
// Write bytes from registerBytes
void requestHandler()
{
  // Ignore registerAddress
  for (int i = 0; i < NUM_REG_BYTES; i++)
    Wire.write(registerBytes[i]);
}

// On Receive:
// Copy the first incoming byte into registerAddress (see notes above)
// Copy the remaining bytes into registerBytes
void receiveHandler(int len)
{
  int count = -1;
  while (Wire.available())
  {
    uint8_t b = Wire.read();
    switch (count)
    {
      case -1:
        registerAddress = b;
        break;
      default:
        // Ignore registerAddress
        if (count < NUM_REG_BYTES)
          registerBytes[count] = b;
        break;
    }
    count++;
  }
}

void setup()
{
  delay(1000); // Allow time for the microcontroller to start up

  Serial.begin(115200); // Begin the Serial console
  while (!Serial)
  {
    delay(100); // Wait for the user to open the Serial Monitor
  }
  Serial.println("SparkFun STP3593LF Emulator");

  Wire.onReceive(receiveHandler);
  Wire.onRequest(requestHandler);
  Wire.begin((uint8_t)I2C_DEV_ADDR);
}

void loop()
{
  static unsigned long lastPrint = 0;

  if (millis() > (lastPrint + 1000))
  {
    lastPrint = millis();

    // Extract the Frequency Control Word
    uint32_t freqControl = ((uint32_t)registerBytes[0]) << 24;
    freqControl |= ((uint32_t)registerBytes[1]) << 16;
    freqControl |= ((uint32_t)registerBytes[2]) <<  8;
    freqControl |= ((uint32_t)registerBytes[3]) <<  0;
    
    Serial.print("Frequency control is ");
    Serial.println(freqControl);
  }
}