
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
int mindreh = 45;
int maxdreh = 135;
bool autodreh = false;
char drehrichtung = "l";

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
  lcd.clear();
  lcd.print("Whoop");
  //lcd.noDisplay();

  
  drehmotor.attach(rotator);
  drehmotor.write(ausrichtung);
}

void loop() {
    String irinput;

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
      if(ausrichtung > 54) {
         ausrichtung = ausrichtung - 9;
       } else {
          ausrichtung = 45;
       }
       drehmotor.write(ausrichtung);
    } else if (irinput == "ffc23d") {
      if(ausrichtung < 126) {
        ausrichtung = ausrichtung + 10;
      } else {
        ausrichtung = 135;
      }
      drehmotor.write(ausrichtung);
    } else if (irinput == "hier play button") {
      if(autodreh == false) {
        autodreh = true;  
      } else {
        autodreh = false;
      }
    } else if (irinput == "hier schneller button") {
      
    }
    /*switch(strtol(value, NULL, 0)) {
    case strtol("FFFFE5D8", NULL, 0): //An-Aus Toggle
      if(active == 0) {
        active = 1;
        lcd.display();
        
      } else if(active == 1){
        active = 0;
        speed = 0;
        ausrichtung = 90;
        lcd.noDisplay;
        
      }
      break;
    case strtol("22DD", NULL, 0): //drehe nach links
      if(ausrichtung > 54) {
         ausrichtung = ausrichtung - 9;
       } else {
          ausrichtung = 45;
       }
      
  */
    
    if(autodreh == true) {
      if(drehrichtung == "l") {
        if(ausrichtung > mindreh) {
          ausrichtung = ausrichtung -1;
          drehmotor.write(ausrichtung);
        } else {
          drehrichtung = "r";
        }
      } else if(drehrichtung == "r") {
        if(drehrichtung == "r") {
          if(ausrichtung < maxdreh) {
            ausrichtung = ausrichtung + 1;
          } else {
            drehrichtung = "l";
          }
        }
      }
      
    }
    
  }
       // delay(500); // 200 ms Pause einfügen

}


/*#include <IRremote.h>
#include <LiquidCrystal_I2C.h>

const int ventilator = 5;
const int rotator = 3;int ir = 7;
IRrecv irrecv(ir);
decode_results results;

LiquidCrystal_I2C lcd (0x27, 16,2);
void setup() {
  // put your setup code here, to run once:
  pinMode(ir, INPUT);
  Serial.begin(9600);
  lcd.init();
  lcd.setCursor(0,0);
  lcd.backlight();
  irrecv.begin(ir);
  pinMode(ventilator, OUTPUT);
  pinMode(rotator, OUTPUT);
  lcd.println("Whoop");
  irrecv.enableIRIn();
}

void loop() {
  // put your main code here, to run repeatedly:
  if(irrecv.decode()) {
    //Serial.print(irrecv.decode);
    int key_value = results.value;
    int reval = irrecv.decode();
    Serial.println(key_value);
    lcd.println(key_value, HEX);
        irrecv.resume();

    //results.value = ""; 
  }
     // irrecv.resume();
  */
//}
