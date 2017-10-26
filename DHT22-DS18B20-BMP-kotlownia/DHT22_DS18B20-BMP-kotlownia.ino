#include <DHT.h>
#include <Ethernet.h>
#include <SPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_BMP085.h>

#define DALLAS_ENABLED
#define DHT_ENABLED
#define DHT_ENABLED1
//#define DHT_ENABLED2
//#define BMP_ENABLED
#define SEND

//#define DEBUG
#define DHTTYPE DHT22
#define DHTPIN1  5
#define DHTPIN2  6
#define ONE_WIRE_BUS 7


const char* place_kotlownia     = "kotlownia";
const char* place_garaz         = "garaz";
const char* place_co_in         = "co_in";
const char* place_co_out        = "co_out";
const char* place_co_floor_in   = "co_floor_in";
const char* place_co_floor_out  = "co_floor_out";
const char* place_outside       = "outside";

DeviceAddress TempCoIn      = { 0x28, 0xCF, 0xC3, 0xCB, 0x06, 0x00, 0x00, 0xB5 };
DeviceAddress TempCoOut     = { 0x28, 0x21, 0xB9, 0xDB, 0x06, 0x00, 0x00, 0x67 };
DeviceAddress TempFloorIn  = { 0x28, 0xFF, 0x07, 0xD4, 0x33, 0x16, 0x03, 0x88 };
DeviceAddress TempFloorOut   = { 0x28, 0xFF, 0x6D, 0x11, 0x34, 0x16, 0x03, 0xE6 };
DeviceAddress TempGaraz     = { 0x28, 0x9D, 0x7F, 0xC3, 0x06, 0x00, 0x00, 0x87 };

#ifdef DALLAS_ENABLED
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress tempDeviceAddress;
#endif

#ifdef DHT_ENABLED1
DHT dht1(DHTPIN1, DHTTYPE);
#endif
#ifdef DHT_ENABLED2
DHT dht2(DHTPIN2, DHTTYPE);
#endif
#ifdef BMP_ENABLED
Adafruit_BMP085 bmp;
#endif

String functionAddress = "http://starzaki.eu.org/~wedrowki/iot/data_collector.php";

long previousMillis = 0;
long currentMillis = 0;

long sendInterval = 300000; // SENDING INTERVAL
//0C-38-59-E8-7D-AE
byte mac[] = { 0x0C, 0x38, 0x59, 0xE8, 0x7D, 0xAE }; // RESERVED MAC ADDRESS


#ifdef SEND
EthernetClient client;
#endif



int MODYFIKATOR = 2;




void setup() {
  Serial.begin(9600);

#ifdef BMP_ENABLED
  bmp.begin();
#endif
#ifdef DHT_ENABLED1
  dht1.begin();
#endif
#ifdef DHT_ENABLED2
  dht2.begin();
#endif

#ifdef DALLAS_ENABLED
  sensors.begin();
  sensors.setResolution(TempCoIn, 10);
  sensors.setResolution(TempCoOut, 10);
  sensors.setResolution(TempFloorIn, 10);
  sensors.setResolution(TempFloorOut, 10);
  sensors.setResolution(TempGaraz, 10);
  discoverOneWireDevices();
#endif

#ifdef SEND
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
  }
#endif
}


void loop() {
  currentMillis = millis();

  float temp[5];
  float natezenie;


  if (currentMillis - previousMillis > sendInterval) { // SEND ONLY ONCE PER INTERVAL
    previousMillis = currentMillis;
    Serial.println("---------------------------------------------------");

#ifdef DALLAS_ENABLED
    sensors.requestTemperatures();
    readDallasTempAndSend(TempCoIn, place_co_in, MODYFIKATOR);
    delay(3000);
    readDallasTempAndSend(TempCoOut, place_co_out, MODYFIKATOR);
    delay(3000);
    readDallasTempAndSend(TempFloorIn, place_co_floor_in, MODYFIKATOR);
    delay(3000);
    readDallasTempAndSend(TempFloorOut, place_co_floor_out, MODYFIKATOR);
    delay(3000);
    readDallasTempAndSend(TempGaraz, place_garaz, 0);
#endif

#ifdef DHT_ENABLED1
    delay(3000);
    readTempAndHumAndSend(&dht1, place_kotlownia);
#endif
#ifdef DHT_ENABLED2
    delay(3000);
    readTempAndHumAndSend(&dht2, place_outside);
#endif
#ifdef BMP_ENABLED
    float tBMP = bmp.readTemperature();
    int cisnienie = bmp.readPressure() / 100;
#ifdef DEBUG
    Serial.println("-------- WARUNKI NA ZEWNATRZ ------------");
    Serial.print("T. BMP = ");
    Serial.print(tBMP);
    Serial.print(" *C\t");
    Serial.print("P: ");
    Serial.print(cisnienie);
    Serial.print("hPa\t");
#endif
    delay(3000);
    sendDataToStarzaki(functionAddress, place_outside, "pressure", String(cisnienie));
#endif
  }
}

#ifdef DALLAS_ENABLED
void readDallasTempAndSend(DeviceAddress address, String place, int modyfikator) {
  float temp = sensors.getTempC(address);
  temp += modyfikator;
#ifdef DEBUG
  Serial.print(place + ": " + String(temp));
  Serial.println("*C\t");
#endif
  sendDataToStarzaki(functionAddress, place, "temperature", String(temp));
}
#endif

#ifdef DHT_ENABLED
void readTempAndHumAndSend(DHT* dht, String place) {
  String data = "";
  float h = dht->readHumidity();
  float t = dht->readTemperature();
#ifdef DEBUG
  Serial.println("Temp. " + place + ": " + String(t));
  Serial.println("Hum. " + place + ": " + String(h));
#endif
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  sendDataToStarzaki(functionAddress, place, "humidity", String(h));
  sendDataToStarzaki(functionAddress, place, "temperature", String(t));
}
#endif
void sendDataToStarzaki(String functionAddress, String place, String type, String value) {
  String data = String("?place=") + place + "&type=" + type + "&value=" + value;
  Serial.print(data);
#ifdef SEND

  if (client.connect("starzaki.eu.org", 80)) { // REPLACE WITH YOUR SERVER ADDRESS
    Serial.println(" >>>> WYSYLAM >>>>");

    client.println(String("GET /~wedrowki/iot/data_collector.php") + data + " HTTP/1.1");
    client.println("Host: starzaki.eu.org"); // SERVER ADDRESS HERE TOO
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.print("Content-Length: ");
    client.println(data.length());
    client.println();
    client.print(data);
  } else {
    Serial.println("Problem z wyslaniem!!!");
  }
  if (client.connected()) {
    client.stop();  // DISCONNECT FROM THE SERVER
  }
#endif
}


#ifdef DALLAS_ENABLED
void discoverOneWireDevices(void) {
  byte i;
  byte addr[8];

  while (oneWire.search(addr)) {
    Serial.print("\n\rFound \'1-Wire\' device with address:\n\r");
    for ( i = 0; i < 8; i++) {
      Serial.print("0x");
      if (addr[i] < 16) {
        Serial.print('0');
      }
      Serial.print(addr[i], HEX);
      if (i < 7) {
        Serial.print(", ");
      }
    }
    if ( OneWire::crc8( addr, 7) != addr[7]) {
      Serial.print("CRC is not valid!\n");
      return;
    }
  }
  Serial.println();
  oneWire.reset_search();
  return;
}
#endif
