#include <Arduino.h>

const int pwmPin = 17;

void setup() {
  analogWriteFrequency(pwmPin, 25000);  // PWM à 25 kHz
  analogWriteResolution(8);             // Résolution de 0 à 255
  pinMode(pwmPin, OUTPUT);
}

void loop() {
  for (int duty = 0; duty <= 255; duty++) {
    analogWrite(pwmPin, duty);
    delay(10);
  }
  for (int duty = 255; duty >= 0; duty--) {
    analogWrite(pwmPin, duty);
    delay(10);
  }
}
