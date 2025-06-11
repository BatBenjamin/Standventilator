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
int maxdreh = 170;
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
const int profil1_winkel = 30;
const int profil1_speed = 90;    // PWM für Stufe 1
const int profil2_winkel = 125;
const int profil2_speed = 45;   // PWM für Stufe 3

// Stromsparmodus und Standby
bool stromsparmodus = false;
unsigned long letzteAktivitaet = 0;
const unsigned long timeoutStromspar = 60000; // 1 Minute
unsigned long standbyStart = 0;
bool standbyPhase = false;

// Standby-Countdown
int standbyCountdown = 0;
unsigned long standbyCountdownStart = 0;
bool standbyCountdownActive = false;

// === IR-Codes als HEX-Werte ===
unsigned long IR_POWER      = 0xFFA25D; 
unsigned long IR_PROFIL1    = 0xFf30cf;
unsigned long IR_PROFIL2    = 0xFf18e7;
unsigned long IR_PROFIL_AUS = 0xFFB04F; // Beispiel
unsigned long IR_LINKS      = 0xFF22DD;
unsigned long IR_RECHTS     = 0xFFC23D;
unsigned long IR_AUTODREH   = 0xFF02FD;
unsigned long IR_SCHNELLER  = 0xFf906f; 
unsigned long IR_LANGSAMER  = 0xFfe01f; 
unsigned long IR_STROMSPAR  = 0xFF9867; 

// Funktion Standby-Countdown deklarieren
void starteStandbyCountdown();

void setProfil(int profil) {
  if (profil == 1) {
    ausrichtung = profil1_winkel;
    speed = profil1_speed;
    aktivesProfil = 1;
    autodreh = false;
    drehmotor.write(ausrichtung);
    analogWrite(ventilator, speed);
  } else if (profil == 2) {
    ausrichtung = profil2_winkel;
    speed = profil2_speed;
    aktivesProfil = 2;
    autodreh = false;
    drehmotor.write(ausrichtung);
    analogWrite(ventilator, speed);
  } else {
    aktivesProfil = 0;
    speed = 0;
    autodreh = false;
    analogWrite(ventilator, 0);    // Ventilator aus
    // Keine Ausrichtung setzen, Servo bleibt wie er ist!
  }
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

// Standby-Countdown Funktion
void starteStandbyCountdown() {
  standbyCountdown = 5;
  standbyCountdownStart = millis();
  standbyCountdownActive = true;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Standby in ");
  lcd.print(standbyCountdown);
  lcd.print("...");
}

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

void loop() {
  unsigned long now = millis();

  // === Automatischer Standby-Countdown nach Inaktivität ===
  if (!stromsparmodus && !standbyPhase && !standbyCountdownActive && (now - letzteAktivitaet > timeoutStromspar)) {
    starteStandbyCountdown();
  }

  // === Standby-Countdown läuft ===
  if (standbyCountdownActive) {
    if (now - standbyCountdownStart >= 1000) {
      standbyCountdown--;
      standbyCountdownStart = now;
      lcd.clear();
      lcd.setCursor(0, 0);
      if (standbyCountdown > 0) {
        lcd.print("Standby in ");
        lcd.print(standbyCountdown);
        lcd.print("...");
      } else {
        lcd.print("Standby");
        standbyCountdownActive = false;
        standbyPhase = true;
        standbyStart = now;
        setProfil(0);   // Profil 0 aktivieren: alles aus!
      }
    }
    // Während Countdown: Nur auf Power-Taste reagieren
    if (irrecv.decode(&results)) {
      if (results.value == IR_POWER) {
        standbyCountdownActive = false;
        lcd.clear();
        lcd.print("Whoop");
        letzteAktivitaet = now;
      }
      irrecv.resume();
    }
    return;
  }

  // === Standby-Phase (3 Sekunden "Standby" anzeigen), dann Stromsparmodus ===
  if (standbyPhase) {
    if (irrecv.decode(&results)) {
      if (results.value == IR_POWER) {
        setStromsparmodus(false);     // Alles aktivieren!
        lcd.clear();
        lcd.print("Whoop");
        standbyPhase = false;
        letzteAktivitaet = now;
      }
      irrecv.resume();
    }
    // Nach 3 Sekunden Standby-Anzeige in den Stromsparmodus wechseln
    if (now - standbyStart > 3000) {
      setStromsparmodus(true);
      standbyPhase = false;
    }
    return;
  }

  // === IR-Befehle abfragen ===
  if (irrecv.decode(&results)) {
    Serial.print("IR empfangen: 0x");
    Serial.println(results.value, HEX);

    // Im Stromsparmodus: nur Power-Taste reaktiviert alles!
    if (stromsparmodus) {
      if (results.value == IR_POWER) {
        setStromsparmodus(false);
        lcd.clear();
        lcd.print("Whoop");
        letzteAktivitaet = now;
      }
      irrecv.resume();
      return;
    }

    lcd.clear();
    lcd.print("IR:");
    lcd.print(results.value, HEX);

    // Jede Aktivität verhindert Standby/Stromsparmodus
    letzteAktivitaet = now;

    // === Profile ===
    if (results.value == IR_PROFIL1) {
      setProfil(1);
      lcd.clear();
      lcd.print("Profil 1 aktiv");
    }
    else if (results.value == IR_PROFIL2) {
      setProfil(2);
      lcd.clear();
      lcd.print("Profil 2 aktiv");
    }
    else if (results.value == IR_PROFIL_AUS) {
      setProfil(0);
      lcd.clear();
      lcd.print("Profil aus");
    }
    // === Power ===
    else if (results.value == IR_POWER) {
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
    else if (results.value == IR_LINKS) {
      if (ausrichtung > (mindreh + 9)) {
        ausrichtung -= 9;
      } else {
        ausrichtung = mindreh;
      }
      drehmotor.write(ausrichtung);
      autodreh = false;
      aktivesProfil = 0;
    }
    // === Rechts drehen ===
    else if (results.value == IR_RECHTS) {
      if (ausrichtung < (maxdreh - 9)) {
        ausrichtung += 10;
      } else {
        ausrichtung = maxdreh;
      }
      drehmotor.write(ausrichtung);
      autodreh = false;
      aktivesProfil = 0;
    }
    // === Autodreh an/aus ===
    else if (results.value == IR_AUTODREH) {
      autodreh = !autodreh;
      lcd.print(autodreh ? "Auto ON" : "Auto OFF");
      aktivesProfil = 0;
    }
    // === Geschwindigkeit erhöhen ===
    else if (results.value == IR_SCHNELLER) {
      if (speed <= (255 - 85)) {
        speed += 85;
      }
      analogWrite(ventilator, speed);
      aktivesProfil = 0;
    }
    // === Geschwindigkeit verringern ===
    else if (results.value == IR_LANGSAMER) {
      if (speed >= 85) {
        speed -= 85;
      }
      analogWrite(ventilator, speed);
      aktivesProfil = 0;
    }
    // === Manueller Stromsparmodus über Fernbedienung mit Countdown ===
    else if (results.value == IR_STROMSPAR) {
      if (!stromsparmodus && !standbyPhase && !standbyCountdownActive) {
        starteStandbyCountdown();
      }
    }

    irrecv.resume();
  }

  // === Temperatur und Feuchte anzeigen (außer im Stromsparmodus/Standby/Countdown) ===
  if (!stromsparmodus && !standbyPhase && !standbyCountdownActive) {
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 10000 || millis() < 500) {
      lastUpdate = millis();
      float feucht = dht.readHumidity();
      float temperature = dht.readTemperature();
      temperature = temperature - 2;
      lcd.setCursor(0, 1);
      lcd.print(temperature, 2);
      lcd.print("C, ");
      lcd.print(feucht, 2);
      lcd.print("%  "); // Leerzeichen, falls Werte kürzer werden
      lcd.setCursor(0, 0);
    }
    // --- Autodreh ---
    static unsigned long lastDreh = 0;
    if (autodreh && (millis() - lastDreh > 50)) {
      lastDreh = millis();
      if (drehrichtung == "r") {
        if (ausrichtung > mindreh) {
          ausrichtung -= 1;            
        } else {
          drehrichtung = "l";
        }
      } else if (drehrichtung == "l") {
        if (ausrichtung < maxdreh) {
          ausrichtung += 1;
        } else {
          drehrichtung = "r";
        }
      }
      drehmotor.write(ausrichtung);
    }
  }
}
