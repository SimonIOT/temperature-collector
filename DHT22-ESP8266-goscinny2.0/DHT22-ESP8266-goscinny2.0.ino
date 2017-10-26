#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <DHT.h>


//#define DALLAS_ENABLED 
#define DHT_ENABLED 

#define DHTTYPE DHT22
#define DHTPIN  2 
ADC_MODE(ADC_VCC);
DHT dht(DHTPIN, DHTTYPE, 11); 
float h, t;  

const char* ssid = "LabZone";
const char* password = "83032105150";
const char* place = "goscinny";


String functionAddress = "http://starzaki.eu.org/~wedrowki/iot/data_collector.php";
const int port = 80;

long previousMillis = 0;
unsigned long currentMillis = 0;

long sendInterval = 60 * 1000; // SENDING INTERVAL

void setup() {
    Serial.begin(9600);

    #ifdef DHT_ENABLED
      dht.begin();
    #endif  

    #ifdef DALLAS_ENABLED
      sensors.begin();
      sensors.setResolution(Temp1, 10);
      discoverOneWireDevices();
    #endif  

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

 
}

void loop() {
  currentMillis = millis();

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
        sendDataToStarzaki(client, data);

        /////////// TEMPERATURE ////////////////////////
        String data = String("?place=") + place + "&type=temperature&value=" + String(t);
        sendDataToStarzaki(functionAddress, data);
      #endif

      

      ///////////////// NAPIECIE /////////////////////
      float vcc = ESP.getVcc() / 1000.0;
      data = String("?place=") + place + "&type=vcc&value=" + String(vcc);
      //send vcc
      sendDataToStarzaki(functionAddress, data);
       
   }  
}


void sendDataToStarzaki(String functionAddress, String data){
  
  HTTPClient http;
  String address = functionAddress + data;
  //Serial.println(String("Sending data: ") + address);
  
  http.begin(address);
  int httpCode = http.GET();

  if(httpCode > 0) {
    //Serial.printf("[HTTP] GET... code: %d\n", httpCode);
      if(httpCode == HTTP_CODE_OK) {
          String payload = http.getString();
          //Serial.println(payload);
      }
  } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
  
}


void discoverOneWireDevices(void) {
  byte i;
  byte addr[8];
  
  while(oneWire.search(addr)) {
    Serial.print("\n\rFound \'1-Wire\' device with address:\n\r");
    for( i = 0; i < 8; i++) {
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
