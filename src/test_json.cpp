// Includes global variables and librarys that the OLED display uses
#include "main.h"
#include <Arduino.h>
//#include <ittiot.h>
#include <FBJS_Config.h>
#include <FirebaseJson.h>

#define FIREBASEJSON_USE_PSRAM
 
#define WIFI_NAME "TalTech"
#define WIFI_PASSWORD ""
 
// Change it according to the real name of the microcontroller where DHT shield is connected
#define MQTT_TOPIC "ESP14"

// OLED reset pin is GPIO0
#define OLED_RESET 0

 
// Define variables to store humidity and temperature values
float h;
float t;

UploadState uploadState = IDLE;
 
// // Message received
// void iot_received(String topic, String msg)
// {
//   // Check if topic contains temperature data
//   if(topic == (MQTT_TOPIC"/temp"))
//   {
//     t = msg.toFloat(); // Convert string to float
//   }
 
//   // Check if topic contains humidity data
//   if(topic == (MQTT_TOPIC"/hum"))
//   {
//     h = msg.toFloat(); // Convert string to float
//   }
// }
 
// // Function started after the connection to the server is established.
// void iot_connected()
// {
//   // Send message to serial port to show that connection is established
//   Serial.println("MQTT connected callback");
//   // Subscribe to topics to get temperature and humidity messages
//   // iot.subscribe(DHT_TOPIC"/temp");
//   // iot.subscribe(DHT_TOPIC"/hum");
//   // Send message to MQTT server to show that connection is established
//   iot.log("IoT OLED example!");
// }
 
void setup()
{
  // Initialize serial port and send message
  Serial.begin(115200); // setting up serial connection parameter
  Serial.println("Booting");
  // https://registry.platformio.org/libraries/mobizt/FirebaseJson
 
  
 
  // iot.setConfig("wname", WIFI_NAME);
  // iot.setConfig("wpass", WIFI_PASSWORD);
  // iot.setConfig("msrv", "193.40.245.72");
  // iot.setConfig("mport", "1883");
  // iot.setConfig("muser", "test");
  // iot.setConfig("mpass", "test");
  // iot.printConfig(); // print IoT json config to serial
  // iot.setup(); // Initialize IoT library

  const char* json_string = "{\"sensor\":\"gps\",\"time\":1351824120,\"data\":[48.756080,2.302038]}";

  FirebaseJson json;

  json.setJsonData(json_string);

  FirebaseJsonData data;
  json.get(data, "sensor");
  String sensor = data.to<String>();
  // long time          = json["time"];
  // double latitude    = json["data"][0];
  // double longitude   = json["data"][1];

  Serial.println(sensor);
  // Serial.println(time);
  // Serial.println(latitude, 6);
  // Serial.println(longitude, 6);
}
 
void loop()
{
  // iot.handle();// IoT behind the plan work, it should be periodically called
 
 
  delay(200); // Waiting 0.2 second
}