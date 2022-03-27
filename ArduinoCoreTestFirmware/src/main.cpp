#include "Arduino.h"

void setup()
{
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // prints title with ending line break
  Serial.println("ArduinoCoreTestFirmware");
}

int loopnum=0;

void loop()
{
  Serial.print("Loop ");
  Serial.print(loopnum++);
  Serial.println(" output");
  delay(900);
}
