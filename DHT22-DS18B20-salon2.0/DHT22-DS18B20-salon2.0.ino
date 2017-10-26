#include <DHT.h>
#include <Ethernet.h>
#include <SPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define DALLAS_ENABLED 
#define DHT_ENABLED 

#define DEBUG 
#define DHTTYPE DHT22
#define DHTPIN  5
#define ONE_WIRE_BUS 6
const char* place = "salon";
const char* place_floor = "salon_podloga";

DeviceAddress Temp1 = { 0x10, 0x8B, 0x1F, 0x0B, 0x03, 0x08, 0x00, 0xFC }; 

#ifdef DALLAS_ENABLED
  OneWire oneWire(ONE_WIRE_BUS);
  DallasTemperature sensors(&oneWire);
  DeviceAddress tempDeviceAddress;
#endif

#ifdef DHT_ENABLED
  DHT dht(DHTPIN, DHTTYPE);
#endif


float h, t;  


String functionAddress = "http://starzaki.eu.org/~wedrowki/iot/data_collector.php";
const int port = 80;

long previousMillis = 0;
long currentMillis = 0;

long sendInterval = 300000; // SENDING INTERVAL


byte mac[] = { 0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x01 }; // RESERVED MAC ADDRESS
EthernetClient client;


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

  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
  }

}


void loop() {
  float tempFloor;
  currentMillis = millis();
  String data = "";

   if (currentMillis - previousMillis > sendInterval) { // READ ONLY ONCE PER INTERVAL

      previousMillis = currentMillis;
      #ifdef DALLAS_ENABLED
          sensors.requestTemperatures();  
          tempFloor = sensors.getTempC(Temp1);
          Serial.print("FLOOR: " + String(tempFloor));
          Serial.println("*C\t");
          ///////////////// PODLOGA /////////////////////
          data = String("?place=") + place_floor + "&type=temperature&value=" + String(tempFloor);
          sendDataToStarzaki(functionAddress, data);
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
        ///////////////// TEMP /////////////////////
        data = String("?place=") + place + "&type=temperature&value=" + String(t);
        sendDataToStarzaki(functionAddress, data);     
      #endif      
   }
}
String getValue(String data, char separator, int index){
 int found = 0;
  int strIndex[] = {0, -1  };
  int maxIndex = data.length()-1;
  for(int i=0; i<=maxIndex && found<=index; i++){
  if(data.charAt(i)==separator || i==maxIndex){
  found++;
  strIndex[0] = strIndex[1]+1;
  strIndex[1] = (i == maxIndex) ? i+1 : i;
  }
 }
  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}




void sendDataToStarzaki(String functionAddress, String data){
  Serial.println(data);
  if (client.connect("starzaki.eu.org", 80)) { // REPLACE WITH YOUR SERVER ADDRESS
        client.println(String("GET /~wedrowki/iot/data_collector.php") + data + " HTTP/1.1");
        client.println("Host: starzaki.eu.org"); // SERVER ADDRESS HERE TOO
        client.println("Content-Type: application/x-www-form-urlencoded");
        client.print("Content-Length: ");
        client.println(data.length());
        client.println();
        client.print(data);
      }
/*
   while(client.connected() && !client.available()) delay(1); //waits for data
   while (client.connected() || client.available()) { //connected or data available
    char c = client.read(); //gets byte from ethernet buffer
    Serial.print(c); //prints byte to serial monitor 
  }*/
    
  if (client.connected()) {
      client.stop();  // DISCONNECT FROM THE SERVER
  }
}
#ifdef DALLAS_ENABLED
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
#endif
