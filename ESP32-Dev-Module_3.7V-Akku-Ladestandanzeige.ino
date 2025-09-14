// === Pin-Definitionen ===
const int PIN_BATTERY   = 35;  // ADC-Pin D35
const int PIN_BUTTON    = 12;  // Knopf mit Pull-Up
const int LED_GREEN     = 5;   // Grün
const int LED_YELLOW    = 18;  // Gelb
const int LED_ORANGE    = 19;  // Orange
const int LED_RED       = 21;  // Rot

// Spannungsteiler-Verhältnis (R1=100k, R2=220k)
const float R1 = 100000.0;
const float R2 = 220000.0;

// ESP32 ADC-Referenzspannung
const float ADC_REF = 3.3;
const int ADC_MAX = 4095;

unsigned long ledOnTime = 0;
bool ledsActive = false;

void setup() {
  Serial.begin(115200);

  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_ORANGE, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  turnOffLeds();
}

void loop() {
  // Knopf gedrückt? (LOW wegen Pull-Up)
  if (digitalRead(PIN_BUTTON) == LOW && !ledsActive) {
    float vBatt = readBatteryVoltage();
    showBatteryStatus(vBatt);
    ledsActive = true;
    ledOnTime = millis();
    Serial.printf("Batteriespannung: %.2f V\n", vBatt);

    // Hier kannst du zusätzlich via Bluetooth senden
    // sendBatteryStatusToApp(vBatt);
  }

  // LEDs nach 1 Sekunde ausschalten
  if (ledsActive && millis() - ledOnTime > 1000) {
    turnOffLeds();
    ledsActive = false;
  }

  delay(50);
}

// === Funktionen ===
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
// 📡 Integration in deine App
// Bluetooth-Senden: Ersetze den Kommentar // sendBatteryStatusToApp(vBatt); durch deine Bluetooth-Sende-Logik.
// Sende z. B. ein JSON-Objekt:

// String msg = "{\"battery\": " + String(vBatt, 2) + "}";
// Serial.println(msg); // oder über BluetoothSerial.write()

// App-Seite: In deiner Android-App kannst du den JSON-String empfangen, den Wert in Prozent umrechnen und im UI darstellen.

