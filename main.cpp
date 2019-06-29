/*
  Simple wemos D1 mini  MQTT example

  This sketch demonstrates the capabilities of the pubsub library in combination
  with the ESP8266 board/library.

  It connects to the provided access point using dhcp, using ssid and pswd

  It connects to an MQTT server ( using mqtt_server ) then:
  - publishes "connected"+uniqueID to the [root topic] ( using topic ) 
  - subscribes to the topic "[root topic]/composeClientID()/in"  with a callback to handle
  - If the first character of the topic "[root topic]/composeClientID()/in" is an 1, 
    switch ON the ESP Led, else switch it off

  - after a delay of "[root topic]/composeClientID()/in" minimum, it will publish 
    a composed payload to 
  It will reconnect to the server if the connection is lost using a blocking
  reconnect function. 
  
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Update these with values suitable for your network.

const char* ssid = "metalab";
const char* pswd = 0;
const char* mqtt_server = "10.20.30.97";

WiFiClient espClient;
PubSubClient client(espClient);

int status = WL_IDLE_STATUS;     // the starting Wifi radio's status

int SWITCH_PIN = D5;
int lastSwitchState = 0;
int toggled = 0;
unsigned long lastToggle = 0;

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, pswd);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    String clientId = "antishutdown";

    // Attempt to connect
    // boolean connect (clientID, username, password, willTopic, willQoS, willRetain, willMessage)
    if (client.connect(clientId.c_str(), "xxxxxxxxxxxxxxxxxxxxxxxx", "xxxxxxxxxxxxxxxxxxxxxxx", "homeassistant/sensor/antishutdown/LWT", 0, true, "offline")) {
      Serial.println("connected");

      uint8_t mac[6];
      WiFi.macAddress(mac);

      String ipState;
      ipState += "{ \"ip\": \"";
      ipState += WiFi.localIP().toString();
      ipState += "\", ";
      ipState += "\"mac\": \"";
      ipState += macToStr(mac);
      ipState += "\" }";
      client.publish("homeassistant/sensor/antishutdown/INFO", ipState.c_str(), false);

      client.publish("homeassistant/sensor/antishutdown/BUTTON", "OFF", true);

      // Once connected and subscribed, publish an announcement...
      client.publish("homeassistant/sensor/antishutdown/LWT", "online", true);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.print(" wifi=");
      Serial.print(WiFi.status());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  pinMode(SWITCH_PIN, INPUT);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void loop() {
  // confirm still connected to mqtt server
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  int switchState = digitalRead(SWITCH_PIN);
  if(switchState != lastSwitchState) {
    if(toggled && (lastToggle + 50 > millis())) {
      // skip, too fast
      return;
    }

    toggled = 1;
    lastToggle = millis();

    Serial.print("Switch state changed to: ");
    Serial.println(switchState);
  
    if(switchState) {
      client.publish("homeassistant/sensor/antishutdown/BUTTON", "ON", true);
    } else {
      client.publish("homeassistant/sensor/antishutdown/BUTTON", "OFF", true);
    }
  }
  lastSwitchState = switchState;
}
