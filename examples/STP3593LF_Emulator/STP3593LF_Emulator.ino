/*
  Emulate the STP3593LF OCXO.

  By: Paul Clark
  SparkFun Electronics
  Date: 2024/11/21
  SparkFun code, firmware, and software is released under the MIT License.
  Please see LICENSE.md for further details.

  Note: This code doesn't update the registerAddress correctly - on ESP32 (3.0.1).
        registerAddress is only updated _after_ the registers have been read...
        SfeSTP3593LFDriver::begin() calls readClipRegister() and readRegisters()
        twice, to ensure registerAddress points at 0x00 and 0x0C correctly.

*/

#include <Wire.h>

#define I2C_DEV_ADDR 0x50

// Emulate registers 0x00 (DCXO Clip) to 0x0E (DCXO LSW) (16-bit, MSB first)
#define CLIP 0x00 // Change to (e.g.) 0x08 to emulate 200ppm clip range. 
#define NUM_REG_BYTES (15*2)
volatile uint8_t registerBytes[NUM_REG_BYTES];
volatile uint8_t registerAddress = 0;

// On Request:
// Write bytes from registerBytes, starting at registerAddress * 2
void requestHandler()
{
  int i = registerAddress * 2;
  for (; i < NUM_REG_BYTES; i++)
    Wire.write(registerBytes[i]);
}

// On Receive:
// Copy the first incoming byte into registerAddress (see notes above)
// Copy the remaining bytes into registerBytes, starting at registerAddress * 2
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
        if (((registerAddress * 2) + count) < NUM_REG_BYTES)
          registerBytes[((registerAddress * 2) + count)] = b;
        break;
    }
    count++;
  }
}

void setup()
{
  // Initialize the register bytes
  for (int i = 0; i < NUM_REG_BYTES; i++)
    registerBytes[i] = 0;
  registerBytes[0] = CLIP; // Store in OCXO Clip MSB

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

    // Extract the 39-bit Frequency Control Word
    uint64_t freqControl = ((uint64_t)registerBytes[28]) >> 1;
    freqControl |= ((uint64_t)registerBytes[27]) << 7;
    freqControl |= ((uint64_t)registerBytes[26]) << 15;
    freqControl |= ((uint64_t)registerBytes[25]) << 23;
    freqControl |= ((uint64_t)registerBytes[24]) << 31;
    if (freqControl & 0x0000004000000000) // Correct two's complement
      freqControl |= 0xFFFFFFC000000000;
    
    union // Avoid any ambiguity when converting uint64_t to int64_t
    {
        uint64_t unsigned64;
        int64_t signed64;
    } unsignedSigned64;
    unsignedSigned64.unsigned64 = freqControl;

    Serial.print("Frequency control is ");
    Serial.println(unsignedSigned64.signed64);
  }
}