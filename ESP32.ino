#include <WiFi.h>
#include <PubSubClient.h>


// Update these with values suitable for your network.
const char* ssid = "PowerOfPhysics";
const char* password = "siJonasBakling";
const char* mqtt_server = "broker.mqtt-dashboard.com";
#define mqtt_port 1883
#define MQTT_USER "mqtt username"
#define MQTT_PASSWORD "mqtt password"
#define MQTT_SERIAL_PUBLISH_CH "/ic/esp32/serialdata/uno/"

// adding serial 
#define RXD2 16
#define TXD2 17

WiFiClient wifiClient;

PubSubClient client(wifiClient);

void setup_wifi() {
    delay(10);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    randomSeed(micros());
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    client.publish("outTopic", "hello world");
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(),MQTT_USER,MQTT_PASSWORD)) {
      Serial.println("connected");
      //Once connected, publish an announcement...
      client.publish("/ic/presence/esp32/", "hello world");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void setup() {
  //initializing serial 2
  // Note the format for setting a serial port is as follows: Serial2.begin(baud-rate, protocol, RX pin, TX pin);
  //Serial1.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  
  Serial.begin(9600);
  Serial.setTimeout(500);// Set time out for 
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  reconnect();
}

void publishSerialData(char *serialData){
  if (!client.connected()) {
    reconnect();
  }
  client.publish(MQTT_SERIAL_PUBLISH_CH, serialData);
}
void loop() {
   client.loop();
   //if (Serial.available() > 0) {
   //  char bfr[501];
   //  memset(bfr,0, 501);
   //  Serial.readBytesUntil( '\n',bfr,500);
   //  publishSerialData(bfr);
   //}

   while (Serial2.available()) {
    //String Reading = Serial2.readString();
    Serial.print(char(Serial2.read()));
    
   }
 }
