/*
  Beispielsketch für TSL45315 Breakout Board (Digitaler Licht Sensor)
 
 Basierend auf dem Originalcode von Watterott:
 https://github.com/watterott/TSL45315-Breakout
 
 Beschreibung und Datenblatt des Sensors:
 http://ams.com/eng/Products/Sensor-Driven-Lighting/SDL-Ambient-Light-Sensors/TSL45315
 
 I2C Pins auf den einzelnen Arduino Boards:
 Board           I2C/TWI Pins
 SDA, SCL
 ----------------------------
 Uno, Ethernet    A4, A5
 Mega             20, 21
 Leonardo          2,  3
 Due              20, 21
 */

// Die Wire Library zur Ansteuerung von I2C Geräten einbinden
#include <Wire.h>

// Der TSL45315 hat werkseitig die I2C ID 0x29
#define I2C_ADDR     (0x29)

// Die fünf Register des Sensors
#define REG_CONTROL  0x00
#define REG_CONFIG   0x01
#define REG_DATALOW  0x04
#define REG_DATAHIGH 0x05
#define REG_ID       0x0A

void setup()
{

  // Serielle Übertragung zum PC starten (für Ausgabe in Serial Monitor)
  Serial.begin(9600);
  // nur für Arduino Leonardo und Micro: Warten bis serielle
  // Schnittstelle bereit ist
  while(!Serial);

  // Wire-Library initialisieren und dem I2C-Bus als Master beitreten
  Wire.begin();

  // ID auslesen, der TSL45315 liefert hier ein Byte der Form 1000xxxx
  // zurück (nur die oberen 4 Bit zählen).
  Serial.print("ID: ");
  // Übertragung an I2C Device mit Adresse 0x29 starten
  Wire.beginTransmission(I2C_ADDR);
  // Ein Byte an den Sensor schicken
  // Der Sensor erwartet bei diesen Kommandos, dass das
  // MSB (Most Significant Bit = das höherwertigste Bit)
  // gesetzt ist. 
  // Das erfolgt am einfachsten mit dem bitweisen Operator | (=OR)
  // 0x80 ist die hexadezimale Darstellung von 1000 0000 (=128 im Dezimalsystem)
  // 
  // 0x80|REGID bedeutet soviel
  //      0x80 = 1000 0000
  //   OR 0x0A = 0000 1010
  //             ---------
  //      ergibt 1000 1010
  //
  // Dieses Byte wird dann mit Wire.write  in die Warteschlange gesteckt
  Wire.write(0x80|REG_ID);
  // und mit Wire.endTransmission an den Sensor geschickt
  Wire.endTransmission();

  // Mit dem obigen Befehl 0x80|REG_ID haben wir dem Sensor mitgeteilt,
  // dass wir bei der nächsten Abfrage den Inhalt des ID-Register auslesen möchten
  // Nun kündigen wir an, dass wir 1 Byte abholen möchten
  Wire.requestFrom(I2C_ADDR, 1); //request 1 byte
  // Wire.available gibt die Anzahl der Bytes an, die abgeholt werden können,
  // ist also TRUE solange noch etwas abgeholt werden kann
  while(Wire.available())
  {
    // Der Datentyp unsigned char entspricht 1 Byte. Dieses Byte abholen
    unsigned char c = Wire.read();
    // Vom zurückgelieferten Byte des ID Registers interessieren uns nur die oberen 4 Bit.
    // Die unteren 4 Bit wollen wir einfach auf Null setzen.
    // Also diesmal den binären Operator AND anwenden, d.h.
    // Inhalt von char c = 1010 xxxx (x = egal ob 0 oder 1, interessiert uns nicht)
    //                 AND 1111 0000
    //                     ---------
    //              ergibt 1010 0000 = 0xA0
    // Ergebnis der Berechnung hexadezimal ausgeben, zeigt "ID: A0"    
    Serial.print(c&0xF0, HEX);
  }
  Serial.println("");

  // Dann den Sensor einschalten
  Serial.println("Power on...");
  // Wieder Übertragung beginnen
  Wire.beginTransmission(I2C_ADDR);
  // Ankündigen, dass nächstes Byte an das Register CONTROL gesendet wird
  Wire.write(0x80|REG_CONTROL);
  // Dann Byte 0x03 senden, bedeutet laut Datenblatt "Normal Operation" (power on)
  Wire.write(0x03);
  // Warteschlange übertragen, Übertragung beenden
  Wire.endTransmission();

  // Und noch eine Konfigurationseinstellung an den Sensor schicken
  Serial.println("Config...");
  Wire.beginTransmission(I2C_ADDR);
  // Achtung: Nächstes Byte ist für das Register CONFIGURATION gedacht
  Wire.write(0x80|REG_CONFIG);
  // Der Sensor kann auf verschiedene Messdauer eingestellt werden
  Wire.write(0x00); //M=1 T=400ms
  // Wire.write(0x01); //M=2 T=200ms
  // Wire.write(0x02); //M=4 T=100ms
  // Schicken und I2C Bus freigeben
  Wire.endTransmission();

  // Im setup() ist einiges passiert, wir wissen jetzt die ID, also den Typ des Sensors
  // und haben ihn eingeschaltet und die Messdauer auf 400ms gestellt.
}


// In der loop() wird der Sensor 1 x pro Sekunde ausgelesen 
void loop()
{
  // Variablen für HIGH und LOW Byte
  uint16_t low, high;
  // Errechneter Wert in Lux
  uint32_t lux;

  // Übertragung beginnen
  Wire.beginTransmission(I2C_ADDR);
  // Der gemessene Wert passt nicht in 1 Byte, wir brauchen 2 Byte dazu (HIGH & LOW)
  // Wir wollen den Inhalt des Registers DATALOW abfragen 
  Wire.write(0x80|REG_DATALOW);
  Wire.endTransmission();
  // Schick uns 2 Byte
  Wire.requestFrom(I2C_ADDR, 2);
  // Erstes Byte in low einlesen. 
  low = Wire.read();
  // Der Registerzeiger im Sensor springt automatisch auf das nächste Register,
  // Das ist DATAHIGH
  // Zweites Byte in high einlesen 
  high = Wire.read();
  // Sicherheitshalber noch schauen, ob noch Bytes übertragen werden wollen
  // Das sollte aber eigentlich nie der Fall sein
  while(Wire.available()){ 
    Wire.read(); 
  }

  // Jetzt den Lux-Wert aus den beiden Bytes ausrechnen. Hier wird auch klar, warum 
  // weiter oben low und high als 16-bit Integers definiert wurden, obwohl in DATALOW und
  // DATAHIGH ja nur je 1 Byte steht.
  //
  // Also: Angenommen unser Messwert wäre dezimal 2025, das ist 
  // ausgedrückt in 16 Bit: 0000 0111  1110 1001
  // oder in Dezimalzahlen:   256 * 7 +      233 = 2025
  // Damit steht im Register DATAHIGH: 0000 0111
  //                   und in DATALOW: 1110 1001
  //
  // In uint16_t high (16 bit!) steht also: 0000 0000 0000 0111
  //                            und in low: 0000 0000 1110 1001
  //
  // Die Anweisung high<<8 in der folgende Zeile schiebt nun die Bits in high um 
  // 8 Stellen nach links, also haben wir jetzt
  //                      in high : 0000 0111 0000 0000 = 1792
  //                    und in low: 0000 0000 1110 1001 =  233
  // Diese Werte werden             -------------------
  // mit | (OR) verknüpft:          0000 0111 1110 1001 = 2025
  lux  = (high<<8) | low;

  // Je nach gewählter Messdauer muss der Messwert laut Datenblatt
  // mit einem Faktor multipliziert werden   
  lux *= 1; //M=1
  // lux *= 2; //M=2
  // lux *= 4; //M=4
  // Messwert ausgeben und 500ms warten
  Serial.print("Lux: ");
  Serial.println(lux, DEC);
  delay(1000);
}


