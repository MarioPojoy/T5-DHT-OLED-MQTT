#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Adafruit_SH110X.h>
#include "credentials.h"
#include "logo.h"

#define i2c_Address   0x3c
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT   64
#define OLED_RESET      -1
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#if defined(ARDUINO_UNOR4_WIFI)
  #define DHTPIN  2
  #define HOSTNAME "uno-r4-wifi"
#elif defined(ARDUINO_D1_UNO32)
  #define DHTPIN  GPIO_NUM_26
  #define HOSTNAME "wemos-d1-r32"
#endif
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define STATUS_LED  LED_BUILTIN
WiFiClient IOTClient;
PubSubClient client(IOTClient);


unsigned long lastMsg = 0;
int read_interval = 10000;

const char* hostname = HOSTNAME;

void setup_wifi() {
  delay(100);
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


void callback(char* topic, byte* payload, unsigned int length) {

}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      String hostnameGreet = hostname;
      hostnameGreet += " connected"; 
      client.publish("home/connections", hostnameGreet.c_str());
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  pinMode(STATUS_LED, OUTPUT);
  Serial.begin(115200);
  delay(100);
  dht.begin();
  display.begin(i2c_Address, true);
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  StaticJsonDocument<32> doc;
  char output[55];

  long now = millis();
  if (now - lastMsg > read_interval) {
    digitalWrite(STATUS_LED, HIGH);
    lastMsg = now;

    float temp = dht.readTemperature();
    float humidity = dht.readHumidity();
    
    Serial.println("Read");

    doc["t"] = temp;
    doc["h"] = humidity;
    
    display.clearDisplay();
    display.drawBitmap(0, 0, logo_bmp, SCREEN_WIDTH, SCREEN_HEIGHT, SH110X_WHITE);
    display.setFont(&FreeMonoBold18pt7b);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(45, 28);
    display.print((int)temp);
    display.drawCircle(92, 8, 3, SH110X_WHITE);
    display.setCursor(100, 27);
    display.print("C");
    display.setCursor(45, 62);
    display.print((int)humidity);
    display.print("%");
    display.display(); 

    serializeJson(doc, output);
    Serial.println(output);
    client.publish("home/bedroom/sensors", output);
    Serial.println("Sent");
    digitalWrite(STATUS_LED, LOW);
  }
    
}