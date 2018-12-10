#include <ESP8266WiFi.h>
#include "Adafruit_Sensor.h"
#include "Adafruit_AM2320.h"

#define FREQUENCY            10 

Adafruit_AM2320 am2320 = Adafruit_AM2320();

String apiKey         = "";   /* Replace with your thingspeak API key */
const char* ssid      = "";          /* Your Router's SSID */
const char* password  = "";      /* Your Router's WiFi Password */
const char* server    = "api.thingspeak.com";
const float convert = 9.0/5.0;

WiFiClient client;
void ConnectAP(void);
void GoToSleep();

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    delay(10); // hang out until serial port opens
  }

  Serial.println("Temperature and humidity monitoring");
  am2320.begin();
  ConnectAP();

  float temp = am2320.readTemperature();
  float humid = am2320.readHumidity();
  Serial.print("Temp: "); Serial.println(temp);
  Serial.print("Hum: "); Serial.println(humid);

  if (isnan(humid) || isnan(temp)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Connecting...");
  if (client.connect(server,80)) {  
    Serial.println("done.");
    String postStr = apiKey;
           postStr +="&field1=";
           postStr += String(temp * convert + 32.0);
           postStr +="&field2=";
           postStr += String(humid);
//           postStr +="&field3=";
//           postStr += String(temp * convert + 32.0);
           postStr += "\r\n\r\n";
     client.print("POST /update HTTP/1.1\n"); 
     client.print("Host: api.thingspeak.com\n"); 
     client.print("Connection: close\n"); 
     client.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n"); 
     client.print("Content-Type: application/x-www-form-urlencoded\n"); 
     client.print("Content-Length: "); 
     client.print(postStr.length()); 
     client.print("\n\n"); 
     client.print(postStr);
  }
  delay(1000);
  client.stop();
  delay(1000);
  GoToSleep();
}

void ConnectAP(void) {
  WiFi.forceSleepWake();
  delay( 1 );
  WiFi.persistent( false );
  WiFi.mode(WIFI_STA);    /* Set WiFi to station mode */
  WiFi.disconnect();     /* disconnect from an AP if it was Previously  connected*/
  delay(100);
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);
 
  WiFi.begin(ssid, password);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    if (tries++ > 120) {
      Serial.println("WiFi failed");
      GoToSleep();
    }
    delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);
}
void GoToSleep() {
   ESP.deepSleep(FREQUENCY * 60 * 1000000, WAKE_RF_DEFAULT);
}
void loop() {
}
