#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>

#define DALLAS_ENABLED
//#define DHT_ENABLED

//#define DEBUG
#define DHTTYPE DHT22
#define DHTPIN  D5
#define ONE_WIRE_BUS D4

DeviceAddress Temp1 = { 0x28, 0x32, 0xC7, 0xDB, 0x06, 0x00, 0x00, 0xAA };

#ifdef DALLAS_ENABLED
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress tempDeviceAddress;
#endif

#ifdef DHT_ENABLED
DHT dht(DHTPIN, DHTTYPE);
#endif

ADC_MODE(ADC_VCC);

float h, t;

const char* ssid = "LabZone";
const char* password = "83032105150";
const char* place = "sypialnia";

String functionAddress = "http://starzaki.eu.org/~wedrowki/iot/data_collector.php";
const int port = 80;

long previousMillis = 0;
long currentMillis = 0;

long sendInterval = 300 * 1000; // SENDING INTERVAL


//Arduino IDE: Board NodeMCU 0.9 (ESP-12 Module), CPU frequency: 80MHz
//flashing:: czogori@Lenovo:/media/czogori/PQI/ARDUINO/esptool$ sudo python ./esptool.py --port /dev/ttyUSB1 write_flash 0x00000 ../nodemcu_float_0.9.6-dev_20150704.bin


void setup() {
  Serial.begin(9600);
  Serial.println("OK");
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(50);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

#ifdef DHT_ENABLED
  dht.begin();
#endif

#ifdef DALLAS_ENABLED
  sensors.begin();
  sensors.setResolution(Temp1, 10);
  discoverOneWireDevices();
#endif


#ifdef DALLAS_ENABLED
#ifdef DEBUG
  for (int i = 0; i < sensors.getDeviceCount(); i++) {
    if (sensors.getAddress(tempDeviceAddress, i)) {
      Serial.print("Found device ");
      Serial.print(i + 1, DEC);
      Serial.print(" with address: ");
      for (uint8_t i = 0; i < 8; i++) {
        if (tempDeviceAddress[i] < 16) Serial.print("0");
        Serial.print(tempDeviceAddress[i], HEX);
      }
      Serial.println();
    } else {
      Serial.print("Found ghost device at ");
      Serial.print(i, DEC);
      Serial.print(" but could not detect address. Check power and cabling");
    }
  }
#endif
#endif

}

void loop() {

  float tempAir;
  currentMillis = millis();
  String data = "";
  if (currentMillis - previousMillis > sendInterval) { // READ ONLY ONCE PER INTERVAL
    previousMillis = currentMillis;
#ifdef DALLAS_ENABLED
    sensors.requestTemperatures();
    tempAir = sensors.getTempC(Temp1);
    Serial.print("AIR: " + String(tempAir));
    Serial.println("*C\t");
    t = tempAir;
#endif

#ifdef DHT_ENABLED
    h = dht.readHumidity();
    // Read temperature as Celsius
    t = dht.readTemperature();
    Serial.println("Temp.: " + String(t));
    Serial.println("Hum.: " + String(h));

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }
    //////////// HUMIDITY //////////////////////////
    data = String("?place=") + place + "&type=humidity&value=" + String(h);
    sendDataToStarzaki(functionAddress, data);
#endif

    /////////// TEMPERATURE ////////////////////////
    data = String("?place=") + place + "&type=temperature&value=" + String(t);
    sendDataToStarzaki(functionAddress, data);

    ///////////////// NAPIECIE /////////////////////
    float vcc = ESP.getVcc() / 1000.0;
    data = String("?place=") + place + "&type=vcc&value=" + String(vcc);
    //send vcc
    sendDataToStarzaki(functionAddress, data);
  }


}

void sendDataToStarzaki(String functionAddress, String data) {

  HTTPClient http;
  String address = functionAddress + data;

  http.begin(address);
  int httpCode = http.GET();

  if (httpCode > 0) {
    //Serial.printf("[HTTP] GET... code: %d\n", httpCode);
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.print(String("Data sent: ") + address);
      Serial.println(" Response: " + payload);
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();

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
