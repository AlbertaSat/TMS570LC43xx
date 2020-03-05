#include <Arduino.h>
/*
  Mega multple serial test
 
 Receives from the main serial port, sends to the others. 
 Receives from serial port 1, sends to the main serial (Serial 0).
 
 This example works only on the Arduino Mega
 
 The circuit: 
 * Any serial device attached to Serial port 1
 * Serial monitor open on Serial port 0:
 
 created 30 Dec. 2008
 by Tom Igoe
 
 This example code is in the public domain.
 
 */


void setup() {
  // initialize both serial ports:
  Serial.begin(9600);
  Serial1.begin(9600);
}

void loop() {
  // read from port 1, send to port serial monitor
  if (Serial1.available()) {
    int16_t inByte = Serial1.read();
    Serial.write(inByte); 
  }
  
  // read from serial monitor and send to port 1
  if (Serial.available()) {
    int16_t inByte = Serial.read();
    Serial1.write(inByte); 
  }
}
