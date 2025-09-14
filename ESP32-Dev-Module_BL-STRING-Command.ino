// Dependencies:
//  • “BluetoothSerial.h” (built‐in for ESP32 Arduino core)

#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

// Define the PINs over here, that bool value can be changed to toggle between pin ON/OFF (List of used Pins)
// → Those Pins can be turned ON/OFF later on.
int controllablePins[] = {2, 4, 13, 14, 27};  

void setup() {
  Serial.begin(115200);

  // Initializing Pins
  for (int i = 0; i < sizeof(controllablePins)/sizeof(controllablePins[0]); i++) {
    pinMode(controllablePins[i], OUTPUT);
    digitalWrite(controllablePins[i], LOW); // Standard Mode: OFF
  }

  // Start Bluetooth
  if (!SerialBT.begin("ESP-Alarm")) {
    Serial.println("Fehler beim Starten von Bluetooth!");
    while (true);
  }
  Serial.println("Bluetooth gestartet. Geräte-Name: ESP-Alarm");
}

void loop() {
  if (SerialBT.available()) {
    String command = SerialBT.readStringUntil('\n');
    command.trim();
    Serial.println("Empfangen: " + command);

    // Expected Format: PINXX_ON or PINXX_OFF
    if (command.startsWith("PIN")) {
      // 1. read GPIO-Number
      int pin = command.substring(3, command.indexOf('_')).toInt();

      // 2. read action (all after "_")
      String action = command.substring(command.indexOf('_') + 1);

      // 3. Check if this Pin is in the allowed-list (controllablePins)
      bool validPin = false;
      for (int i = 0; i < sizeof(controllablePins)/sizeof(controllablePins[0]); i++) {
        if (controllablePins[i] == pin) {
          validPin = true;
          break;
        }
      }

      if (validPin) {
        // Action based on command
        if (action == "ON") {
          digitalWrite(pin, HIGH);    // turn Pin on
          SerialBT.println("Pin " + String(pin) + " eingeschaltet");
        } else if (action == "OFF") {
          digitalWrite(pin, LOW);     // turn Pin off
          SerialBT.println("Pin " + String(pin) + " ausgeschaltet");
        } else {
          SerialBT.println("Unbekannte Aktion: " + action);
        }
      } else {
        SerialBT.println("Pin " + String(pin) + " ist nicht erlaubt!");
      }
    } else {
      SerialBT.println("Falsches Format! Nutze z.B. PIN13_ON oder PIN25_OFF");
    }
  }
}
