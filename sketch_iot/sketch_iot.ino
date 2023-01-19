// Import the necessary libraries
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <NewPing.h> 
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// Declare constants for the node ID and node name
#define NODE_ID "1"
#define NODE_NAME "MYHOME"

// define the pin numbers for the sensors
#define TRIGGER_PIN  D2  // ESP8266 NodeMCU pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     D3  // ESP8266 NodeMCU pin tied to echo pin on the ultrasonic sensor.#define MAX_DISTANCE 200 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.
#define MAX_DISTANCE 200 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.
#define LDR_PIN A0
#define DHTPIN D1    // Digital pin connected to the DHT sensor 
#define DHTTYPE    DHT11     // DHT 11

#define LED1 D5
#define LED2 D6
#define LED3 D7
#define LED4 D8

#ifndef STASSID
#define STASSID "your-ssid"
#define STAPSK "your-password"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

// Initialize ultrasonic sensor using NewPing library
NewPing ultrasonic_sensor(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

// Initialize DHT11 sensor using Adafruit_DHT library
DHT_Unified dht(DHTPIN, DHTTYPE);

//ฟังก์ชั่นที่ใช้ในการเชื่อมต่อ Wi-Fi
void connectToWiFi() {

  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
      would try to act as both a client and an access-point and could cause
      network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

}

//ฟังก์ชั่นที่ดึงข้อมูลการสถานะของ UI States มากำหนดสถานะการทำงานของ GPIO
void getStatesData() {

  // Wait for WiFi connection
  if ((WiFi.status() == WL_CONNECTED)) {
    
    // Define the API endpoint
    String url = "https://<random-string>.trycloudflare.com/states";

    // Create an instance of the WiFi client
    WiFiClient client;

    // Create an instance of the HTTP client
    HTTPClient http;
  
    Serial.print("[HTTP] begin...\n");
    // configure traged server and url
    http.begin(client, url);  // HTTPS
  
    Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode = http.GET();
    
    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] POST... code: %d\n", httpCode);
  
      // file found at server
      if (httpCode == HTTP_CODE_OK) {
        const String payload = http.getString();
        Serial.println("received payload:\n<<");
        Serial.println(payload);
        Serial.println(">>");

        StaticJsonDocument<200> jsonDoc;
        deserializeJson(jsonDoc, payload);

        // Call function writeStateValueToGPIO
        writeStateValueToGPIO(jsonDoc);
       
      }
    } else {
      Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
      // Retry sending the request
      //updateSensorData(node_id,node_name,memory_size,sensor_id,sensor_name,sensor_value);
    }
  
    // Close the connection
    http.end();

  }else{

  }
    
}

//ฟังก์ชั่นที่กำหนดสถานะการทำงานของ GPIO
void writeStateValueToGPIO(StaticJsonDocument<200> jsonDoc) {
  // Get the number of objects in the json payload
  int size = jsonDoc.size();
  
  // Loop through each object in the json payload
  for (int i = 0; i < size; i++) {
    // Get the cid and cvalue values of the current object
    String cid = jsonDoc[i]["cid"];
    int cvalue = jsonDoc[i]["cvalue"];
    // Check the cid value and write the corresponding cvalue to the appropriate GPIO pin
    if (cid == "iotsw1") {
      digitalWrite(1, cvalue);
    } else if (cid == "iotsw2") {
      digitalWrite(2, cvalue);
    } else if (cid == "iotsw3") {
      digitalWrite(3, cvalue);
    } else if (cid == "iotsw4") {
      digitalWrite(4, cvalue);
    }
  }
}

// ฟังก์ชั่นอ่านข้อมูล Sensors ต่างๆ ที่เชื่อมต่อกับบอร์ด ESP8266
void readSensorData() {

  // read the ultrasonic sensor data
  int distance = ultrasonic_sensor.ping_cm();

  // read the LDR sensor data
  int ldrValue = analogRead(LDR_PIN);

  // read the memory of the ESP8266
  int memorySize = ESP.getFreeHeap();
  
  float temperature,humidity;
  // Get temperature event and print its value.
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  }
  else {
    //Serial.print(F("Temperature: "));
    //Serial.print(event.temperature);
    //Serial.println(F("°C"));
    temperature = event.temperature;
  }
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  }
  else {
    //Serial.print(F("Humidity: "));
    //Serial.print(event.relative_humidity);
    //Serial.println(F("%"));
    humidity = event.relative_humidity;
  }

  // call the function to update the sensor data
  updateSensorData(NODE_ID, NODE_NAME, memorySize, "ultrasonic", "distance", distance);
  updateSensorData(NODE_ID, NODE_NAME, memorySize, "ldr", "light", ldrValue);
  updateSensorData(NODE_ID, NODE_NAME, memorySize, "dht11", "temperature", temperature);
  updateSensorData(NODE_ID, NODE_NAME, memorySize, "dht11", "humidity", humidity);
  
  // Delay for a while before reading sensors again
  delay(1000);

}

//ฟังก์ชั่นที่เชื่อมต่อ RESTful API เพื่อทำการ Update ข้อมูลใน Database Server
void updateSensorData(const char* node_id, const char* node_name, int memory_size, const char* sensor_id, const char* sensor_name, float sensor_value) {
  
  // Wait for WiFi connection
  if ((WiFi.status() == WL_CONNECTED)) {
  
    // Define the API endpoint
    String url = "https://<random-string>.trycloudflare.com/sensors";

    // Create an instance of the WiFi client
    WiFiClient client;

    // Create an instance of the HTTP client
    HTTPClient http;
  
    // Prepare the JSON body
    StaticJsonDocument<200> jsonDoc;
    jsonDoc["node_id"] = node_id;
    jsonDoc["node_name"] = node_name;
    jsonDoc["memory_size"] = memory_size;
    jsonDoc["sensor_id"] = sensor_id;
    jsonDoc["sensor_name"] = sensor_name;
    jsonDoc["sensor_value"] = sensor_value;
  
    Serial.print("[HTTP] begin...\n");
    // configure traged server and url
    http.begin(client, url);  // HTTP
    http.addHeader("Content-Type", "application/json");
  
    Serial.print("[HTTP] POST...\n");
    // start connection and send HTTP header and body
    int httpCode = http.POST(jsonDoc.as<String>());
    
    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] POST... code: %d\n", httpCode);
  
      // file found at server
      if (httpCode == HTTP_CODE_OK) {
        const String payload = http.getString();
        Serial.println("received payload:\n<<");
        Serial.println(payload);
        Serial.println(">>");
      }
    } else {
      Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
      // Retry sending the request
      //updateSensorData(node_id,node_name,memory_size,sensor_id,sensor_name,sensor_value);
    }
  
    // Close the connection
    http.end();
    
  }else{
    
    connectToWiFi();
    
  }

}

void setup(){

  Serial.begin(9600);
  pinMode(LED1,OUTPUT);
  pinMode(LED2,OUTPUT);
  pinMode(LED3,OUTPUT);
  pinMode(LED4,OUTPUT);
  
  pinMode(TRIGGER_PIN,INPUT);
  pinMode(ECHO_PIN,OUTPUT);

  pinMode(LDR_PIN,INPUT);
  pinMode(DHTPIN,INPUT);

  // Initialize device.
  dht.begin();
  
}

void loop(){

  /* เรียกใช้งานฟังก์ชั่นที่ดึงข้อมูลการสถานะของ UI States 
    * มากำหนดสถานะการทำงานของ GPIO ผ่านฟังก์ชั่น writeStateValueToGPIO(args)
    */
  getStatesData();

  /* เรียกใช้งานฟังก์ชั่นอ่านข้อมูล Sensors ต่างๆ ที่เชื่อมต่อกับบอร์ด ESP8266 
    * และเรียกใช้งานฟังก์ชั่น updateSensorData(args) เพื่อส่งข้อมูลผ่าน RESTful API เพื่อทำการ Update ข้อมูลใน pgSQL Database
    */
  readSensorData();
  
}