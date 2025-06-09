#include "DHT.h"
#include <IRremote.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

Servo drehmotor;
int irPin = 8; // Pin für IR Receiver
const int ventilator = 5;
const int rotator = 3;

int speed = 0;
int active = 0;
int ausrichtung = 90;
int mindreh = 20;
int maxdreh = 160;
bool autodreh = false;
String drehrichtung = "l";

#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

IRrecv irrecv(irPin);
decode_results results;
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Profile
int aktivesProfil = 0; // 0 = kein Profil, 1 = Profil 1, 2 = Profil 2
const int profil1_winkel = 70;
const int profil1_speed = 85;    // PWM für Stufe 1
const int profil2_winkel = 125;
const int profil2_speed = 255;   // PWM für Stufe 3

// Stromsparmodus und Standby
bool stromsparmodus = false;
unsigned long letzteAktivitaet = 0;
const unsigned long timeoutStromspar = 60000; // 1 Minute
unsigned long standbyStart = 0;
bool standbyPhase = false;

// ===============================
// Hilfsfunktionen
// ===============================
void setProfil(int profil) {
  if (profil == 1) {
    ausrichtung = profil1_winkel;
    speed = profil1_speed;
    aktivesProfil = 1;
    autodreh = false;
  } else if (profil == 2) {
    ausrichtung = profil2_winkel;
    speed = profil2_speed;
    aktivesProfil = 2;
    autodreh = false;
  } else {
    aktivesProfil = 0;
  }
  drehmotor.write(ausrichtung);
  analogWrite(ventilator, speed);
}

void setStromsparmodus(bool modus) {
  stromsparmodus = modus;
  if (modus) {
    lcd.noBacklight();
    lcd.noDisplay();
    analogWrite(ventilator, 0); // Ventilator aus
    drehmotor.detach();         // Servo-Motor deaktivieren
  } else {
    lcd.backlight();
    lcd.display();
    drehmotor.attach(rotator);
    drehmotor.write(ausrichtung);
    analogWrite(ventilator, speed);
  }
}

// ===============================
// SETUP
// ===============================
void setup() {
  pinMode(irPin, INPUT);
  irrecv.enableIRIn();
  Serial.begin(9600);

  lcd.init();
  lcd.setCursor(0, 0);
  lcd.backlight();
  pinMode(ventilator, OUTPUT);
  pinMode(rotator, OUTPUT);
  dht.begin();
  lcd.clear();
  lcd.print("Whoop");

  drehmotor.attach(rotator);
  drehmotor.write(ausrichtung);
  letzteAktivitaet = millis();
}

// ===============================
// LOOP
// ===============================
void loop() {
  unsigned long now = millis();

  // --- Standby-Anzeige vor Stromsparmodus ---
  if (!stromsparmodus && !standbyPhase && (now - letzteAktivitaet > timeoutStromspar)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Standby");
    standbyStart = now;
    standbyPhase = true;
  }

  // Nach 3 Sekunden Standby in den Stromsparmodus gehen
  if (standbyPhase && (now - standbyStart > 3000)) {
    setStromsparmodus(true);
    standbyPhase = false;
  }

  // Während Standby: keine andere Funktion, außer IR abfragen (nur Power-Taste)
  if (standbyPhase) {
    if (irrecv.decode(&results)) {
      if (results.value != 0xFFFFFFFF) {
        String irinput = String(results.value, HEX);
        if (irinput == "FFFFE5D8") { // <--- On/Off Taste (dein Code ggf. anpassen!)
          lcd.clear();
          lcd.print("Whoop");
          standbyPhase = false;
          letzteAktivitaet = now;
        }
      }
      irrecv.resume();
    }
    return; // Nichts anderes tun während Standby!
  }

  // --- IR Befehle abfragen ---
  if (irrecv.decode(&results)) {
    if (results.value != 0xFFFFFFFF) {
      String irinput = String(results.value, HEX);
      Serial.println(irinput);

      // Im Stromsparmodus: nur Power-Taste reaktiviert alles!
      if (stromsparmodus) {
        if (irinput == "FFFFE5D8") {
          setStromsparmodus(false);
          lcd.clear();
          lcd.print("Whoop");
          letzteAktivitaet = now;
        }
        irrecv.resume();
        return; // Alle anderen IR-Befehle werden im Stromsparmodus ignoriert!
      }

      lcd.clear();
      lcd.print(irinput);

      // Jede Aktivität verhindert Standby/Stromsparmodus
      letzteAktivitaet = now;

      // === Profile ===
      if (irinput == "PROFIL1_CODE") { // <-- ersetze durch echten Code!
        setProfil(1);
        lcd.clear();
        lcd.print("Profil 1 aktiv");
      }
      else if (irinput == "PROFIL2_CODE") {
        setProfil(2);
        lcd.clear();
        lcd.print("Profil 2 aktiv");
      }
      else if (irinput == "PROFIL_AUS_CODE") {
        setProfil(0);
        lcd.clear();
        lcd.print("Profil aus");
      }
      // === Power ===
      else if (irinput == "FFFFE5D8") {
        if (active == 0) {
          active = 1;
          lcd.display();
        } else {
          active = 0;
          speed = 0;
          ausrichtung = 90;
          lcd.noDisplay();
          analogWrite(ventilator, 0);
          aktivesProfil = 0;
        }
      }
      // === Links drehen ===
      else if (irinput == "ff22dd") {
        if (ausrichtung > (mindreh + 9)) {
          ausrichtung = ausrichtung - 9;
        } else {
          ausrichtung = mindreh;
        }
        drehmotor.write(ausrichtung);
        autodreh = false;
        aktivesProfil = 0;
      }
      // === Rechts drehen ===
      else if (irinput == "ffc23d") {
        if (ausrichtung < (maxdreh - 9)) {
          ausrichtung = ausrichtung + 10;
        } else {
          ausrichtung = maxdreh;
        }
        drehmotor.write(ausrichtung);
        autodreh = false;
        aktivesProfil = 0;
      }
      // === Autodreh an/aus ===
      else if (irinput == "ff02fd") {
        autodreh = !autodreh;
        lcd.print(autodreh);
        aktivesProfil = 0;
      }
      // === Geschwindigkeit erhöhen ===
      else if (irinput == "SCHNELLER_CODE") {
        if (speed <= (255 - 85)) {
          speed = speed + 85;
        }
        analogWrite(ventilator, speed);
        aktivesProfil = 0;
      }
      // === Geschwindigkeit verringern ===
      else if (irinput == "LANGSAMER_CODE") {
        if (speed >= 85) {
          speed = speed - 85;
        }
        analogWrite(ventilator, speed);
        aktivesProfil = 0;
      }
      // === Manueller Stromsparmodus ===
      else if (irinput == "STROMSPAR_CODE") {
        lcd.clear();
        lcd.print("Standby");
        delay(3000);
        setStromsparmodus(true);
      }
    }
    irrecv.resume();
  }

  // --- Temperatur und Feuchte anzeigen (außer im Stromsparmodus/Standby) ---
  if (!stromsparmodus) {
    unsigned long time = millis();
    if (time % 10000 == 0 || time < 500) {
      float feucht = dht.readHumidity();
      float temperature = dht.readTemperature();
      temperature = temperature - 2;
      lcd.setCursor(0, 1);
      lcd.print(temperature, 2);
      lcd.print("C, ");
      lcd.print(feucht, 2);
      lcd.print("%");
      lcd.setCursor(0, 0);
    }
    // --- Autodreh ---
    if (autodreh && millis() % 50 == 0) {
      if (drehrichtung == "r") {
        if (ausrichtung > mindreh) {
          ausrichtung = ausrichtung - 1;
        } else {
          drehrichtung = "l";
        }
      } else if (drehrichtung == "l") {
        if (ausrichtung < maxdreh) {
          ausrichtung = ausrichtung + 1;
        } else {
          drehrichtung = "r";
        }
      }
      drehmotor.write(ausrichtung);
    }
  }
}
