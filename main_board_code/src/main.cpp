#include <Arduino.h>
#include <FlexCAN_T4.h>
#include <SD.h>

// ==== Pins ====
const int fanPwmPin       = 18;
const int fcTempPin       = 40;
const int fcVoltagePin    = 28;
const int fcCurrentPin    = 27;
const int evPurgePin      = 3;

// ==== CAN ====
FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> canBus;

// ==== Modes ====
enum SystemMode { STARTUP, NORMAL };
SystemMode mode = STARTUP;
unsigned long startupStartTime = 0;
const unsigned long startupDurationMs = 10000; // 10s

// ==== Purge cycle ====
unsigned long lastPurgeTime = 0;
const unsigned long purgeIntervalStartup = 10000;
const unsigned long purgeIntervalNormal  = 30000;
const unsigned long purgeDurationMs = 500;
bool purgeActive = false;
unsigned long purgeStartTime = 0;

// ==== Sécurité ====
const float MIN_VOLTAGE = 20.0;
const float MAX_TEMP = 70.0;

// ==== Logging ====
File logFile;
unsigned long lastLogTime = 0;
const unsigned long logIntervalMs = 500;

// ==== Lecture capteurs ====
float readVoltage() {
  int raw = analogRead(fcVoltagePin);
  return raw * (3.3 / 1023.0) * 11; // diviseur de tension
}

float readTemperature(int pin) {
  int raw = analogRead(pin);
  return (raw * 3.3 / 1023.0 - 0.5) * 100.0; // LM35
}

// ==== Contrôle ventilateur ====
void setFanSpeed(float temp) {
  int pwm = 0;
  if (mode == STARTUP) {
    pwm = 255;
  } else if (temp > 30) {
    pwm = map(temp, 30, 70, 100, 255);
    pwm = constrain(pwm, 100, 255);
  }
  analogWrite(fanPwmPin, pwm);
}

// ==== CAN ====
void sendTelemetry(float voltage, float temp) {
  CAN_message_t msg;
  msg.id = 0x101;
  msg.len = 4;
  msg.buf[0] = (uint8_t)(voltage * 10);
  msg.buf[1] = (uint8_t)(temp);
  msg.buf[2] = 0;
  msg.buf[3] = 0;
  canBus.write(msg);
}

void handleIncomingCAN() {
  CAN_message_t msg;
  if (canBus.read(msg)) {
    if (msg.id == 0x200) {
      // Traitement spécifique à faire ici
    }
  }
}

// ==== Sécurité ====
void checkSafety(float voltage, float temp) {
  if (voltage < MIN_VOLTAGE) {
    Serial.println("Tension trop basse !");
    analogWrite(fanPwmPin, 255);
    // TODO : désactiver sortie, alerte, LED...
  }

  if (temp > MAX_TEMP) {
    Serial.println("Température critique !");
    // TODO : alerte ou arrêt d'urgence
  }
}

// ==== Purge ====
void handlePurgeCycle() {
  unsigned long now = millis();
  unsigned long interval = (mode == STARTUP) ? purgeIntervalStartup : purgeIntervalNormal;

  if (!purgeActive && now - lastPurgeTime >= interval) {
    digitalWrite(evPurgePin, HIGH);
    purgeStartTime = now;
    purgeActive = true;
    Serial.println("Purge activée");
  }

  if (purgeActive && now - purgeStartTime >= purgeDurationMs) {
    digitalWrite(evPurgePin, LOW);
    purgeActive = false;
    lastPurgeTime = now;
    Serial.println("Purge terminée");
  }
}

// ==== Logging ====
void logData(unsigned long now, float voltage, float temp) {
  if (now - lastLogTime >= logIntervalMs && logFile) {
    logFile.printf("%lu,%.2f,%.2f\n", now, voltage, temp);
    logFile.flush();
    lastLogTime = now;
  }
}

// ==== Setup ====
void setup() {
  Serial.begin(115200);
  delay(500);

  // PWM ventilateur
  pinMode(fanPwmPin, OUTPUT);
  analogWriteFrequency(fanPwmPin, 25000);
  analogWrite(fanPwmPin, 0);

  // Purge
  pinMode(evPurgePin, OUTPUT);
  digitalWrite(evPurgePin, LOW);

  // CAN
  canBus.begin();
  canBus.setBaudRate(500000);

  // SD
  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("Carte SD non détectée !");
  } else {
    logFile = SD.open("log.csv", FILE_WRITE);
    if (logFile) {
      logFile.println("Temps(ms),Tension(V),Température(°C)");
      Serial.println("ogging activé");
    } else {
      Serial.println("Impossible d’ouvrir log.csv");
    }
  }

  // Temps de démarrage
  startupStartTime = millis();
  Serial.println("Système en mode STARTUP");
}

// ==== Loop ====
void loop() {
  unsigned long now = millis();

  // Changement de mode
  if (mode == STARTUP && now - startupStartTime >= startupDurationMs) {
    mode = NORMAL;
    Serial.println("Passage en mode NORMAL");
  }

  float voltage = readVoltage();
  float temp    = readTemperature(fcTempPin);

  setFanSpeed(temp);
  checkSafety(voltage, temp);
  handlePurgeCycle();
  sendTelemetry(voltage, temp);
  handleIncomingCAN();
  logData(now, voltage, temp);

  delay(100); // fréquence principale = 10 Hz
}
