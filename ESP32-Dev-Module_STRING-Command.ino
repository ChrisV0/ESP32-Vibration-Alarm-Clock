 // Wiring & Pin Assignments:
//  --------------------------------------------------------
//  Relay (alarm activation)          → GPIO 4
//  Speaker 1 enable                  → GPIO 25
//  Speaker 2 enable                  → GPIO 26
//  Buzzer output                     → GPIO 13
//  Alarm‐off button (active LOW)     → GPIO 14
//  Snooze button (active LOW)        → GPIO 27
//  Buzzer‐toggle button (active LOW) → GPIO 2
//  Mute button (active LOW)          → GPIO 33
//  Volume pot (ADC)                  → GPIO 34
//  Battery sense ADC                 → GPIO 35
//  Battery‐LED‐button (active LOW)   → GPIO 12
//  LED green (≥ 4.0 V)               → GPIO 5
//  LED yellow (≥ 3.7 V)              → GPIO 18
//  LED orange (≥ 3.5 V)              → GPIO 19
//  LED red (≥ 3.3 V, blink if < 3.3 V) → GPIO 21
//  Feedback LED (on‐board LED)       → LED_BUILTIN = GPIO 32 = D2
//  --------------------------------------------------------
// 
// Notes on ADC:
//  • Battery voltage divider: 
//      Vin → R1 = 100 kΩ → ADC (D35) → R2 = 220 kΩ → GND
//    ADC measurement range: 0–3.3 V. 
//    Actual battery voltage = Vadc * (R1 + R2) / R2.
//    Ratio = (100 k + 220 k) / 220 k ≈ 1.454545
// 
//  • Potentiometer on D34 is assumed to be wired between 3.3 V and GND, wiper to ADC.
//    Raw ADC (0–4095) → map to volume 0–100%.
// 
// Dependencies:
//  • “BluetoothSerial.h” (built‐in for ESP32 Arduino core)
//  • “ArduinoJson.h” (install via Library Manager; tested with v6.x)

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Pins für die Ladestand-Anzeige definieren
#define LED_GREEN 5
#define LED_YELLOW 18
#define LED_ORANGE 19
#define LED_RED 21
#define BATTERY_ADC_PIN 35
#define LED_BATTERY_KNOPF 12
#define LED_INTERNAL 2
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#include "BluetoothSerial.h"
BluetoothSerial SerialBT;

// Liste deiner verwendeten Pins hier definieren
// → diese Pins kannst du später über Befehle ansteuern
int controllablePins[] = {2, 13, 25, 26};  

void setup() {
  Serial.begin(115200);

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // Initialisiere Pins für die Ladestand-Anzeige
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_ORANGE, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_INTERNAL, OUTPUT);

  pinMode(LED_BATTERY_KNOPF, INPUT_PULLUP);
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  // Pins initialisieren
  for (int i = 0; i < sizeof(controllablePins)/sizeof(controllablePins[0]); i++) {
    pinMode(controllablePins[i], OUTPUT);
    digitalWrite(controllablePins[i], LOW); // Anfangszustand aus
  }

  // Bluetooth starten
  if (!SerialBT.begin("ESP32_Control")) {
    Serial.println("Fehler beim Starten von Bluetooth!");
    while (true);
  }
  Serial.println("Bluetooth gestartet. Geräte-Name: ESP32_Control");
}

void loop() {
  if (SerialBT.available()) {
    String command = SerialBT.readStringUntil('\n');
    command.trim();
    Serial.println("Empfangen: " + command);

    // Erwartetes Format: PINXX_ON oder PINXX_OFF
    if (command.startsWith("PIN")) {
      int pin = command.substring(3, command.indexOf('_')).toInt(); // Nummer nach "PIN" auslesen
      String action = command.substring(command.indexOf('_') + 1);  // "ON" oder "OFF"

      // prüfen ob der Pin in deiner Liste erlaubt ist
      bool validPin = false;
      for (int i = 0; i < sizeof(controllablePins)/sizeof(controllablePins[0]); i++) {
        if (controllablePins[i] == pin) {
          validPin = true;
          break;
        }
      }

      if (validPin) {
        if (action == "ON") {
          digitalWrite(pin, HIGH);
          SerialBT.println("Pin " + String(pin) + " eingeschaltet");
        } else if (action == "OFF") {
          digitalWrite(pin, LOW);
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



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // Physische Tasten verarbeiten
void handleButtons() {
  if (digitalRead(LED_BATTERY_KNOPF) == LOW) {
    showBatteryLEDs();
  }
}

// Kurzes LED-Blinken zur Bestätigung eines Befehls
void flashInternalLED() {
  digitalWrite(LED_INTERNAL, HIGH);
  delay(200);
  digitalWrite(LED_INTERNAL, LOW);
}

// Misst Batteriespannung über Spannungsteiler
float readBatteryVoltage() {
  int raw = analogRead(BATTERY_ADC_PIN);
  float voltage = (raw / 4095.0) * 3.3 * (100 + 220) / 220.0;
  return voltage;
}

// Wandelt Spannung in prozentuale Batterieanzeige um
int voltageToPercent(float v) {
  if (v >= 4.0) return 100;
  if (v >= 3.7) return 75;
  if (v >= 3.5) return 50;
  if (v >= 3.3) return 25;
  return 10;
}

// Zeigt Ladezustand über LEDs an
void showBatteryLEDs() {
  float v = readBatteryVoltage();
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_ORANGE, LOW);
  digitalWrite(LED_RED, LOW);

  if (v >= 4.0) digitalWrite(LED_GREEN, HIGH);
  else if (v >= 3.7) digitalWrite(LED_YELLOW, HIGH);
  else if (v >= 3.5) digitalWrite(LED_ORANGE, HIGH);
  else if (v >= 3.3) digitalWrite(LED_RED, HIGH);
  else {
    for (int i = 0; i < 5; i++) {
      digitalWrite(LED_RED, HIGH);
      delay(200);
      digitalWrite(LED_RED, LOW);
      delay(200);
    }
  }

  delay(1000);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_ORANGE, LOW);
  digitalWrite(LED_RED, LOW);
}
