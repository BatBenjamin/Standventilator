#include "DHT.h"

#define DHTPIN 4        // Pin, an dem der DHT11 angeschlossen ist
#define DHTTYPE DHT11   // DHT11 Sensor

DHT dht(DHTPIN, DHTTYPE);  // DHT-Objekt erstellen

void setup() {
  Serial.begin(9600);
  dht.begin();  // Sensor initialisieren
}

void loop() {
  float humidity = dht.readHumidity();        // Luftfeuchtigkeit lesen
  float temperature = dht.readTemperature();  // Temperatur in °C lesen

  // Prüfen, ob Messwerte gültig sind
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Fehler beim Lesen vom DHT-Sensor!");
    delay(1000);
      humidity -= 20; // Jetzt ist der Wert um 3 reduziert
    return;
  }


  Serial.print("Luftfeuchtigkeit (%): ");
  Serial.println(humidity, 2);

  temperature -= 2; //kalibirert
  Serial.print("Temperatur (°C): ");
  Serial.println(temperature, 2);

  delay(2000);  // 2 Sekunden warten
}
