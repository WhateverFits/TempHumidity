#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#include <PubSubClient.h>
#include <Time.h>
#include "Adafruit_Sensor.h"
#include "Adafruit_AM2320.h"

#define FREQUENCY            10 

Adafruit_AM2320 am2320 = Adafruit_AM2320();

#define DNSNAME "office"
#define MQTT_SERVER "pi4"
#define MQTT_PORT 1883
#define MQTT_CHANNEL_TEMP "home/" DNSNAME "/temp"
#define MQTT_CHANNEL_HUMID "home/" DNSNAME "/humidity"
#define MQTT_USER "user"
#define MQTT_PASSWORD "pass"
#define UPDATE_URL "http://pi4/cgi-bin/test.rb"

String mqttClientId;
const char* server    = "api.thingspeak.com";
const float convert = 9.0/5.0;

WiFiClient client;
WiFiClient mqttWiFiClient;

void mqttCallback(char* topic, byte* payload, unsigned int length);

void ConnectAP(void);
void GoToSleep();

PubSubClient mqttClient(MQTT_SERVER, MQTT_PORT, mqttCallback, mqttWiFiClient);

String generateMqttClientId() {
  char buffer[4];
  uint8_t macAddr[6];
  WiFi.macAddress(macAddr);
  sprintf(buffer, "%02x%02x", macAddr[4], macAddr[5]);
  return DNSNAME + String(buffer);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("Inside mqtt callback: %s\n", topic);
  Serial.println(length);

  String topicString = (char*)topic;
  topicString = topicString.substring(topicString.lastIndexOf('/')+1);
  Serial.print("Topic: ");
  Serial.print(topicString);

  String action = (char*)payload;
  action = action.substring(0, length);
  Serial.println(action);

  if (action == "Update") {
	  WiFiClient updateWiFiClient;
	  t_httpUpdate_return ret = ESPhttpUpdate.update(updateWiFiClient, UPDATE_URL);
	  switch (ret) {
		  case HTTP_UPDATE_FAILED:
			  Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
			  break;

		  case HTTP_UPDATE_NO_UPDATES:
			  Serial.println("HTTP_UPDATE_NO_UPDATES");
			  break;

		  case HTTP_UPDATE_OK:
			  Serial.println("HTTP_UPDATE_OK");
			  break;
	  }
  }
}

void mqttPublishTemp(float temperature) {
  char buf[256];
	if (mqttClient.connected()) {
    String(temperature).toCharArray(buf, 256);
		mqttClient.publish(MQTT_CHANNEL_TEMP, buf, true);
	}
}


void mqttPublishHumidity(float humidity) {
  char buf[256];
	if (mqttClient.connected()) {
    String(humidity).toCharArray(buf, 256);
		mqttClient.publish(MQTT_CHANNEL_HUMID, buf, true);
	}
}

boolean mqttReconnect() {
  char buf[100];
  mqttClientId.toCharArray(buf, 100);
  if (mqttClient.connect(buf, MQTT_USER, MQTT_PASSWORD)) {
  }

  return mqttClient.connected();
}


void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10); // hang out until serial port opens
  }

  long milliseconds = millis();

  mqttClientId = generateMqttClientId();

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

  // Separate mqtt publishing because system seemed to miss reading them when too close together
  if (mqttReconnect()) {
    mqttClient.loop();
    mqttPublishTemp(temp);
  }

  Serial.print("Connecting...");
  if (client.connect(server,80)) {  
    Serial.println("done.");
    String postStr = apiKey;
           postStr +="&field1=";
           postStr += String(temp * convert + 32.0);
           postStr +="&field2=";
           postStr += String(humid);
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

  // Separate mqtt publishing because system seemed to miss reading them when too close together
  if (mqttReconnect()) {
    mqttClient.loop();
    mqttPublishHumidity(humid);
  }

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
