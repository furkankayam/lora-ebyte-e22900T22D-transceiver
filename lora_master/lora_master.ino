/*
    Author: Mehmet Furkan KAYA
    Version: 1.1
*/

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <LoRa_E22.h>
#include <HardwareSerial.h>


const char* WiFi_SSID = "<WIFI_NAME>";
const char* WiFi_PASS = "<WIFI_PASSWORD>";
const char* MQTT_SERVER = "<MQTT_SERVER_IP>";
const char* MQTT_CLIENT_ID = "master";
const char* MQTT_SUB_TOPIC = "master_subscribe";


WiFiClient mqttInstance;
PubSubClient mqtt(mqttInstance);


#define M0 32
#define M1 33
#define RX 27
#define TX 35
#define address 1
#define channel 36
#define sendToAddress 2


typedef struct {
  bool test;
} MASTER_DATA;

MASTER_DATA masterData;


HardwareSerial mySerial(1);
LoRa_E22 e22(TX, RX, &mySerial, UART_BPS_RATE_9600);


String removeSpaces(String input);
void wifiConnect(void);
void mqttConnect(void);
void wifiMqttCheck(void);
void callback(char* topic, byte* message, unsigned int length);
void publishMessage(const char* topic, const char* message);
void loraE22Settings(void);
void loraSend(void);


void setup() {
  pinMode(M0, OUTPUT);
  pinMode(M1, OUTPUT);
 
  Serial.begin(9600);
  e22.begin();
 
  loraE22Settings();

  digitalWrite(M0, LOW);
  digitalWrite(M1, LOW);

  delay(500);

  wifiConnect();
  mqtt.setServer(MQTT_SERVER, 1883);
  mqtt.setCallback(callback);
  mqttConnect();
}

void loop() {
  wifiMqttCheck();
}

// WiFi
void wifiConnect(void) {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(WiFi_SSID, WiFi_PASS);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected to WiFi.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// MQTT
void mqttConnect(){
   while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqtt.connect(MQTT_CLIENT_ID)) {
      Serial.println("connected");
      mqtt.subscribe(MQTT_SUB_TOPIC);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 2 seconds");
      delay(2000);
    }
  }
}

// WiFi and MQTT Connect
void wifiMqttCheck(void) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost, reconnecting...");
    wifiConnect();
  }

  if (!mqtt.connected()) {
    Serial.println("MQTT connection lost, reconnecting...");
    mqttConnect();
  }

  mqtt.loop();
}

// MQTT Publish
void publishMessage(const char* topic, const char* message) {
  if (!mqtt.publish(topic, message)) {
    Serial.println("Message could not be published!");
  } else {
    Serial.print("Message successfully published: ");
    Serial.println(message);
  }
}

// MQTT Subscribe
void callback(char* topic, byte* message, unsigned int length) {
  String messageContent;
  for (unsigned int i = 0; i < length; i++) {
    messageContent += (char)message[i];
  }

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, messageContent);

  if (error) {
    Serial.print("JSON parsing error: ");
    Serial.println(error.c_str());
    return;
  }

  bool testValue = doc["test"];

  masterData.test = testValue;
  loraSend();
}

// LoRa E22 Settings
void loraE22Settings() {
 
  digitalWrite(M0, LOW);
  digitalWrite(M1, HIGH);
 
  ResponseStructContainer c;
  c = e22.getConfiguration();
  Configuration configuration = *(Configuration*)c.data;
 
  //SETTINGS THAT CAN CHANGE
  configuration.ADDL = lowByte(address);
  configuration.ADDH = highByte(address);
  configuration.CHAN = channel;
  configuration.NETID = 0x00;
 
  //OPTIONAL SETTINGS
  configuration.SPED.airDataRate = AIR_DATA_RATE_010_24;
  configuration.OPTION.subPacketSetting = SPS_240_00;
  configuration.OPTION.transmissionPower = POWER_22;
 
  //ADVANCED SETTINGS
  configuration.SPED.uartBaudRate = UART_BPS_9600;
  configuration.SPED.uartParity = MODE_00_8N1;
  configuration.TRANSMISSION_MODE.WORPeriod = WOR_2000_011;
  configuration.OPTION.RSSIAmbientNoise = RSSI_AMBIENT_NOISE_DISABLED;
  configuration.TRANSMISSION_MODE.WORTransceiverControl = WOR_RECEIVER;
  //configuration.TRANSMISSION_MODE.WORTransceiverControl = WOR_TRANSMITTER;
  configuration.TRANSMISSION_MODE.enableRSSI = RSSI_DISABLED;
  configuration.TRANSMISSION_MODE.fixedTransmission = FT_FIXED_TRANSMISSION;
  configuration.TRANSMISSION_MODE.enableRepeater = REPEATER_DISABLED;
  configuration.TRANSMISSION_MODE.enableLBT = LBT_DISABLED;
 
  //Save and Store SETTINGS
  ResponseStatus rs = e22.setConfiguration(configuration, WRITE_CFG_PWR_DWN_SAVE);
  c.close();

}

// LoRa Send
void loraSend(){
  ResponseStatus rs = e22.sendFixedMessage(highByte(sendToAddress), lowByte(sendToAddress), channel, &masterData, sizeof(MASTER_DATA));
  Serial.println(rs.getResponseDescription());
}
