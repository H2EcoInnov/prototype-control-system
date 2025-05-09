#include <Arduino.h>

const int fanPwm = 18;
const int fanSpeed = 26 ;
const int fcTemp = 40 ;
const int fcVoltage = 28 ;
const int fcCurrrent = 27 ;
const int evPurge = 3 ;
const int boardTemp = 10 ;
const int canRx = 30 :
const int canTx = 31 ;

void setup() {
  analogWriteFrequency(pwmPin, 25000);  // PWM à 25 kHz
  analogWriteResolution(8);             // Résolution de 0 à 255
  pinMode(pwmPin, OUTPUT);
}

void loop() {
  for (int duty = 0; duty <= 255; duty++) {
    analogWrite(pwmPin, duty);
    delay(100);
  }
  for (int duty = 255; duty >= 0; duty--) {
    analogWrite(pwmPin, duty);
    delay(100);
  }
}
