#include "DHT.h"
#include <IRremote.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

Servo drehmotor;
const int irPin      = 8;
const int ventilator = 5;
const int rotator    = 3;

// ---------- Grundwerte ----------
int   speed        = 0;
int   active       = 0;          // 0 = aus, 1 = an
int   ausrichtung  = 90;
const int standard_winkel = 90;  // Ziel beim Ausschalten
int   mindreh      = 20;
int   maxdreh      = 170;
bool  autodreh     = false;
String drehrichtung = "l";

#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

IRrecv irrecv(irPin);
decode_results results;
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------- Profile ----------
int aktivesProfil = 0;        // 0 = Profil 0, 1 = P1, 2 = P2, 3 = P3
const int profil1_winkel = 30;
const int profil1_speed  = 90;
const int profil2_winkel = 125;
const int profil2_speed  = 70;   // P3 nutzt speed von P1

// ---------- Stromspar & Standby ----------
bool           stromsparmodus    = false;
unsigned long  letzteAktivitaet  = 0;
const unsigned long timeoutStromspar = 60000;   // 1 min
unsigned long  standbyStart      = 0;
bool           standbyPhase      = false;

// ---------- Standby-Countdown ----------
int            standbyCountdown       = 0;
unsigned long  standbyCountdownStart  = 0;
bool           standbyCountdownActive = false;

// ---------- IR-Codes ----------
unsigned long IR_POWER      = 0xFFA25D;
unsigned long IR_PROFIL0    = 0xFF6897;
unsigned long IR_PROFIL1    = 0xFF30CF;
unsigned long IR_PROFIL2    = 0xFF18E7;
unsigned long IR_PROFIL3    = 0xFF7A85;
unsigned long IR_LINKS      = 0xFF22DD;
unsigned long IR_RECHTS     = 0xFFC23D;
unsigned long IR_AUTODREH   = 0xFF02FD;
unsigned long IR_SCHNELLER  = 0xFF906F;  // Beschleunigen
unsigned long IR_LANGSAMER  = 0xFFE01F;
unsigned long IR_STROMSPAR  = 0xFF9867;

// ---------- Vorwärts-Deklarationen ----------
void starteStandbyCountdown();
void setProfil(int profil);
void setStromsparmodus(bool modus);
void moveServoSlow(int ziel);

// ---------- Servo langsam bewegen ----------
void moveServoSlow(int ziel) {
  int schritt = (ziel > ausrichtung) ? 1 : -1;
  for (int pos = ausrichtung; pos != ziel; pos += schritt) {
    drehmotor.write(pos);
    delay(15);                              // ≈1,3 s für 90 °
  }
  drehmotor.write(ziel);
  ausrichtung = ziel;
}

// ---------- Profil-Funktion ----------
void setProfil(int profil) {
  if (profil == 0) {                         // PROFIL 0
    aktivesProfil = 0;
    speed         = 0;
    autodreh      = false;
    ausrichtung   = 1;
    drehmotor.write(ausrichtung);
    analogWrite(ventilator, 0);

  } else if (profil == 1) {
    ausrichtung   = profil1_winkel;
    speed         = profil1_speed;
    aktivesProfil = 1;
    autodreh      = false;
    drehmotor.write(ausrichtung);
    analogWrite(ventilator, speed);

  } else if (profil == 2) {
    ausrichtung   = profil2_winkel;
    speed         = profil2_speed;
    aktivesProfil = 2;
    autodreh      = false;
    drehmotor.write(ausrichtung);
    analogWrite(ventilator, speed);

  } else if (profil == 3) {                  // PROFIL 3
    ausrichtung   = mindreh;
    speed         = profil1_speed;
    aktivesProfil = 3;
    autodreh      = true;
    drehrichtung  = "l";
    drehmotor.write(ausrichtung);
    analogWrite(ventilator, speed);
  }
}

// ---------- Stromsparmodus ----------
void setStromsparmodus(bool modus) {
  stromsparmodus = modus;
  if (modus) {
    lcd.noBacklight();
    lcd.noDisplay();
    analogWrite(ventilator, 0);
    drehmotor.detach();
  } else {
    lcd.backlight();
    lcd.display();
    drehmotor.attach(rotator);
    drehmotor.write(ausrichtung);
    analogWrite(ventilator, speed);
  }
}

// ---------- Standby-Countdown ----------
void starteStandbyCountdown() {
  standbyCountdown       = 5;
  standbyCountdownStart  = millis();
  standbyCountdownActive = true;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Standby in ");
  lcd.print(standbyCountdown);
  lcd.print("...");
}

// ---------- Setup ----------
void setup() {
  pinMode(irPin, INPUT);
  irrecv.enableIRIn();
  Serial.begin(9600);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Whoop");

  pinMode(ventilator, OUTPUT);
  pinMode(rotator,    OUTPUT);

  dht.begin();

  drehmotor.attach(rotator);
  drehmotor.write(ausrichtung);

  letzteAktivitaet = millis();
}

// ---------- Loop ----------
void loop() {
  unsigned long now = millis();

  // ----- Auto-Standby nach Inaktivität -----
  if (!stromsparmodus && !standbyPhase && !standbyCountdownActive &&
      (now - letzteAktivitaet > timeoutStromspar)) {
    starteStandbyCountdown();
  }

  // ----- Countdown läuft -----
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
        // --- Standby beginnt ---
        setProfil(0);                       // alles aus
        lcd.noBacklight();                  // Bildschirm aus!
        lcd.noDisplay();
        standbyCountdownActive = false;
        standbyPhase           = true;
        standbyStart           = now;
      }
    }

    // während Countdown nur POWER zulassen
    if (irrecv.decode(&results)) {
      if (results.value == IR_POWER) {
        standbyCountdownActive = false;
        lcd.backlight();
        lcd.display();
        lcd.clear();
        lcd.print("Whoop");
        letzteAktivitaet = now;
      }
      irrecv.resume();
    }
    return;
  }

  // ----- Standby-Phase → Stromspar (nach 3 s für Sensor usw.) -----
  if (standbyPhase) {
    if (irrecv.decode(&results)) {
      if (results.value == IR_POWER) {
        setStromsparmodus(false);
        standbyPhase = false;
        lcd.clear();
        lcd.print("Whoop");
        letzteAktivitaet = now;
      }
      irrecv.resume();
    }
    if (now - standbyStart > 3000) {        // nach 3 s wirklich Schlaf
      setStromsparmodus(true);
      standbyPhase = false;
    }
    return;
  }

  // ----- IR-Befehle -----
  if (irrecv.decode(&results)) {
    Serial.print("IR empfangen: 0x");
    Serial.println(results.value, HEX);

    // Im Stromsparmodus nur POWER zulassen
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
    letzteAktivitaet = now;

    // ---- Profile ----
    if (results.value == IR_PROFIL0)      { setProfil(0); lcd.clear(); lcd.print("Profil 0 aktiv"); }
    else if (results.value == IR_PROFIL1) { setProfil(1); lcd.clear(); lcd.print("Profil 1 aktiv"); }
    else if (results.value == IR_PROFIL2) { setProfil(2); lcd.clear(); lcd.print("Profil 2 aktiv"); }
    else if (results.value == IR_PROFIL3) { setProfil(3); lcd.clear(); lcd.print("Profil 3 aktiv"); }

    // ---- POWER ----
    else if (results.value == IR_POWER) {
      if (active == 0) {                   // Einschalten
        active = 1;
        lcd.display();
        lcd.backlight();
      } else {                             // Ausschalten
        active = 0;
        speed  = 0;
        autodreh = false;
        analogWrite(ventilator, 0);
        moveServoSlow(standard_winkel);    // Servo langsam auf 90 °
        aktivesProfil = 0;
        lcd.noDisplay();
        lcd.noBacklight();
      }
    }

    // ---- Servo manuell ----
    else if (results.value == IR_LINKS) {
      if (ausrichtung > (mindreh + 9)) ausrichtung -= 9; else ausrichtung = mindreh;
      drehmotor.write(ausrichtung);
      autodreh = false; aktivesProfil = 0;
    }
    else if (results.value == IR_RECHTS) {
      if (ausrichtung < (maxdreh - 9)) ausrichtung += 10; else ausrichtung = maxdreh;
      drehmotor.write(ausrichtung);
      autodreh = false; aktivesProfil = 0;
    }

    // ---- Auto-Dreh an/aus ----
    else if (results.value == IR_AUTODREH) {
      autodreh = !autodreh; aktivesProfil = 0;
      lcd.print(autodreh ? "Auto ON" : "Auto OFF");
    }

    // ---- Beschleunigen (Speed +9, Start bei 20) ----
    else if (results.value == IR_SCHNELLER) {
      if (speed == 0)            speed = 20;
      else if (speed <= 255 - 9) speed += 9;
      analogWrite(ventilator, speed);
      aktivesProfil = 0;
    }

    // ---- Abbremsen (optional weiterhin Schritte 9) ----
    else if (results.value == IR_LANGSAMER) {
      if (speed >= 9) speed -= 9;
      else            speed = 0;
      analogWrite(ventilator, speed);
      aktivesProfil = 0;
    }

    // ---- Manueller Stromspar-Countdown ----
    else if (results.value == IR_STROMSPAR) {
      if (!stromsparmodus && !standbyPhase && !standbyCountdownActive) {
        starteStandbyCountdown();
      }
    }
    irrecv.resume();
  }

  // ===== Anzeige & Auto-Dreh =====
  if (!stromsparmodus && !standbyPhase && !standbyCountdownActive) {
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 10000 || millis() < 500) {
      lastUpdate = millis();
      float feucht = dht.readHumidity();
      float temp   = dht.readTemperature() - 2;
      lcd.setCursor(0, 1);
      lcd.print(temp, 2); lcd.print("C, ");
      lcd.print(feucht, 2); lcd.print("%  ");
      lcd.setCursor(0, 0);
    }

    // ---- Auto-Dreh ----
    static unsigned long lastDreh = 0;
    if (autodreh && millis() - lastDreh > 50) {
      lastDreh = millis();
      if (drehrichtung == "r") {
        if (ausrichtung > mindreh) ausrichtung--; else drehrichtung = "l";
      } else {
        if (ausrichtung < maxdreh) ausrichtung++; else drehrichtung = "r";
      }
      drehmotor.write(ausrichtung);
    }
  }
}
