 // Wiring & Pin Assignments:
//  --------------------------------------------------------
//  Relay (alarm activation)          → GPIO 4
//  Buzzer output                     → GPIO 13
//  Alarm‐off button (active LOW)     → GPIO 14
//  Snooze button (active LOW)        → GPIO 27
//  Buzzer‐toggle button (active LOW) → GPIO 2
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

// === Pin-Definitions ===
const int PIN_BATTERY   = 35;  // ADC-Pin D35
const int PIN_BUTTON    = 12;  // Knopf mit Pull-Up
const int LED_GREEN     = 5;   // Grün
const int LED_YELLOW    = 18;  // Gelb
const int LED_ORANGE    = 19;  // Orange
const int LED_RED       = 21;  // Rot

// VoltageDivider-Ratio (R1=100k, R2=220k)
const float R1 = 100000.0;
const float R2 = 220000.0;

// ESP32 ADC-Reference Voltage
const float ADC_REF = 3.3;
const int ADC_MAX = 4095;

unsigned long ledOnTime = 0;
bool ledsActive = false;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#include "BluetoothSerial.h"
#include "ArduinoJson.h"
BluetoothSerial SerialBT;

// Define the PINs over here, that bool value can be changed to toggle between pin ON/OFF (List of used Pins)
// → Those Pins can be turned ON/OFF later on.
int controllablePins[] = {2, 4, 13, 14, 27};  

void setup() {
  Serial.begin(115200);

  // for the Battery Stage Display (4 LEDs)
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_ORANGE, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  turnOffLeds();

  Serial.begin(115200);

  // Initializing Pins
  for (int i = 0; i < sizeof(controllablePins)/sizeof(controllablePins[0]); i++) {
    pinMode(controllablePins[i], OUTPUT);
    digitalWrite(controllablePins[i], LOW); // Anfangszustand aus
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
      int pin = command.substring(3, command.indexOf('_')).toInt(); // Nummer nach "PIN" auslesen
      String action = command.substring(command.indexOf('_') + 1);  // "ON" oder "OFF"

      // Consider if the Pin is allowed in the set list above
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
    delay(50);
    

  while(PIN_BATTERY == 35) {
  // Button pressed? (LOW because Pull-Up)
  if (digitalRead(PIN_BUTTON) == LOW && !ledsActive) {
    float vBatt = readBatteryVoltage();
    showBatteryStatus(vBatt);
    ledsActive = true;
    ledOnTime = millis();
    Serial.printf("Batteriespannung: %.2f V\n", vBatt);

    // Possibility for later, Charge-State can be send over Bluetooth
    // sendBatteryStatusToApp(vBatt);
  }

  // Turn the LEDs off after 1 Second
  if (ledsActive && millis() - ledOnTime > 1000) {
    turnOffLeds();
    ledsActive = false;
  }
}

}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// for the Battery Stage Display (4 LEDs)

// === Functions ===
float readBatteryVoltage() {
  int adcVal = analogRead(PIN_BATTERY);
  float vOut = (adcVal * ADC_REF) / ADC_MAX; 
  float vBatt = vOut * ((R1 + R2) / R2);
  return vBatt;
}

void showBatteryStatus(float v) {
  turnOffLeds();
  if (v >= 4.0) {
    digitalWrite(LED_GREEN, HIGH);
  } else if (v >= 3.7) {
    digitalWrite(LED_YELLOW, HIGH);
  } else if (v >= 3.5) {
    digitalWrite(LED_ORANGE, HIGH);
  } else if (v >= 3.3) {
    digitalWrite(LED_RED, HIGH);
  } else {
    // Warn-Blinken bei <3.3V
    for (int i = 0; i < 3; i++) {
      digitalWrite(LED_RED, HIGH);
      delay(150);
      digitalWrite(LED_RED, LOW);
      delay(150);
    }
  }
}

void turnOffLeds() {
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_ORANGE, LOW);
  digitalWrite(LED_RED, LOW);
}
//---------------------------------
// 📡 Integration into an App (later on)
// Send Bluetooth: Replace the Comment // sendBatteryStatusToApp(vBatt); with personalized Bluetooth-Send-Logic.
// Send e.g. a JSON-Object:

// String msg = "{\"battery\": " + String(vBatt, 2) + "}";
// Serial.println(msg); // oder über BluetoothSerial.write()

// App-Site: In the Android-App the JSON-String can be recieved, then thw Worth calculated into %, then show it in the UI