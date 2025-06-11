#include "DHT.h"
#include <IRremote.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

Servo drehmotor;
int irPin = 8; // Pin am Arduino UNO für den IR Receiver
const int ventilator = 5;
const int rotator = 3;
int ir = 8;
int speed = 0;
int active = 0;
int ausrichtung = 90;
int mindreh = 20;
int maxdreh = 160;
bool autodreh = false;
String drehrichtung = "l";
#define DHTPIN 2        // Pin, an dem der DHT11 angeschlossen ist
#define DHTTYPE DHT11   // DHT11 Sensor

DHT dht(DHTPIN, DHTTYPE);  // DHT-Objekt erstellen

IRrecv irrecv(irPin); // Objekt initialisieren für die IR Übertragung

decode_results results;

LiquidCrystal_I2C lcd (0x27, 16,2);


void setup() {                                   

  pinMode(irPin, INPUT);  // Den IR Pin als Eingang deklarieren.

  irrecv.enableIRIn(); // Den IR Pin aktivieren

  Serial.begin(9600); // Serielle Kommunikation mit 9600 Baud beginnen

  lcd.init();
  lcd.setCursor(0,0);
  lcd.backlight();
  pinMode(ventilator, OUTPUT);
  pinMode(rotator, OUTPUT);
  dht.begin();
  lcd.clear();
  lcd.print("Whoop");
  //lcd.noDisplay();

  
  drehmotor.attach(rotator);
  drehmotor.write(ausrichtung);
}

void loop() {
    String irinput;
    long time = millis();
    if(time % 10000 == 0 || time < 500) {
      float feucht = dht.readHumidity();
      float temperature = dht.readTemperature();
      temperature = temperature - 2;
      lcd.setCursor(0,1);
      lcd.print(temperature, 2);
      lcd.print("C, ");
      lcd.print(feucht, 2);
      lcd.print("%");
      lcd.setCursor(0,0);
    }
  if (irrecv.decode(&results)) { // Wenn etwas gelesen wurde dann...

    // Ausgabe des Wertes auf die serielle Schnittstelle.

    if (results.value != 0xFFFFFFFF) { // Ignoriere "FFFFFFFF"

      irinput = String(results.value, HEX);

      Serial.println(irinput); // Ausgabe als Hexadezimal
      lcd.clear();
      lcd.print(irinput);

    } else {
      irinput = "Error";
    }

    irrecv.resume(); // auf den nächsten Wert warten

    if (irinput == "FFFFE5D8") {
      if(active == 0) {
        active = 1;
        lcd.display();
        
      } else if(active == 1){
        active = 0;
        speed = 0;
        ausrichtung = 90;
        lcd.noDisplay();
        
      }
    } else if (irinput == "ff22dd") {
      if(ausrichtung > (mindreh + 9)) {
         ausrichtung = ausrichtung - 9;
       } else {
          ausrichtung = mindreh;
       }
       drehmotor.write(ausrichtung);
       autodreh = false;
    } else if (irinput == "ffc23d") {
      if(ausrichtung < (maxdreh - 9)) {
        ausrichtung = ausrichtung + 10;
      } else {
        ausrichtung = maxdreh;
      }
      drehmotor.write(ausrichtung);
      autodreh = false;
    } else if (irinput == "ff02fd") {
      if(autodreh == false) {
        autodreh = true; 
      } else {
        autodreh = false;
      }
      lcd.print(autodreh); 

    } else if (irinput == "hier schneller button") {
      if (speed <= (255 - 85)) {
        speed = speed + 85;
      }
      digitalWrite(ventilator, speed);
    } else if (irinput == "hier langsamer button") {
        if(speed >=85) {
          speed = speed - 85;
        }
        digitalWrite(ventilator, speed);
    }
    
    
    
  }
  if(autodreh == true & millis()%50 == 0) {
      if(drehrichtung == "r") {
        if(ausrichtung > mindreh) {
          ausrichtung = ausrichtung -1;
        } else {
          drehrichtung = "l";
        }
      } else if(drehrichtung == "l") {
          if(ausrichtung < maxdreh) {
            ausrichtung = ausrichtung + 1;
          } else {
            drehrichtung = "r";
          }
      }
      drehmotor.write(ausrichtung);
    }
}
