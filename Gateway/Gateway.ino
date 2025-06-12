#include "secure.h"
// The header file contains private info and cannot public to any one, 
// if you clone this repo and want to reuse it, you can insert your FIREBASE, BLYNK and Local network in and comment the 1st line  

// // Blynk innfo
// #define BLYNK_TEMPLATE_ID 
// #define BLYNK_TEMPLATE_NAME 
// #define BLYNK_AUTH_TOKEN 

// // WiFi Configuration (sử dụng thông tin WiFi từ chương trình đầu tiên)
// #define WIFI_SSID 
// #define WIFI_PASSWORD 

// // Firebase Configuration
// #define FIREBASE_HOST 
// #define FIREBASE_AUTH 

#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <PubSubClient.h>
#include <BlynkSimpleEsp8266.h>
#define BLYNK_PRINT Serial

// Declare Firebase objects
FirebaseConfig config;
FirebaseAuth auth;
FirebaseData firebaseData;

// MQTT Configuration
const char* mqtt_server = "192.168.47.178";  // IP của broker MQTT
const int mqttPort = 1883;
WiFiClient espClient;
PubSubClient client(espClient);

// Blynk
char ssid[] = "qqqq";
char pass[] = "33339999";
BlynkTimer timer;
bool FIRE=false;
bool auto_flag=false;

// Declare topics
const char* DHT1_humid = "DHT1_humid";  // Topic độ ẩm DHT1
const char* DHT1_temp = "DHT1_temp";    // Topic nhiệt độ DHT1
const char* air = "air";                // Topic không khí
const char* water = "water";            // Topic trạng thái nước
const char* RFID = "RFID";              // Topic RFID
const char* cleaner = "cleaner";        // Topic cleaner
const char* test = "test";        // Topic cleaner

int ledPin = 2;  // GPIO2 (D4 trên ESP8266)

// Declare variables to store the last known values for each topic
String lastHumidity = "";
String lastTemperature = "";
String lastAirQuality = "";
String lastWaterState = "";
String lastCleaner = "";

String air_state = "";
String water_state = "";
String cleaner_state = "";

bool entry_flag=false;
unsigned long entry_time;
#define ENTRY_LIMIT 1000

int est_uni,current_uni;

// WiFi Initialization
void wifi_init() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);  // Initialize Blynk after Wi-Fi is connected
}

// MQTT Callback
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String topicStr = String(topic);
  String message = "";

  // Chuyển payload thành chuỗi
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Received message: ");
  Serial.print("Topic: " + topicStr);
  Serial.println(" Message: " + message);

  // Kiểm tra nếu là topic DHT1_humid
  if (topicStr == DHT1_humid && message != lastHumidity) {
    if (message.equalsIgnoreCase("FIRE")){
      client.publish("FIRE","FIRE");
      FIRE=true;
    }
    // Đẩy dữ liệu vào Firebase nếu có sự thay đổi
    String path = "/env/Humidity";  // Đường dẫn Firebase
    Firebase.setString(firebaseData, path, message );
    
    // Kiểm tra nếu Firebase operation thất bại
    if (firebaseData.httpCode() != 200) {
      Serial.println("Failed to push humidity data to Firebase: " + firebaseData.errorReason());
    } else {
      Serial.println("Successfully pushed humidity data to Firebase.");
      lastHumidity = message; // Lưu lại giá trị mới
    }
  }

  // Kiểm tra nếu là topic DHT1_temp
  if (topicStr == DHT1_temp && message != lastTemperature) {
    // Đẩy dữ liệu vào Firebase nếu có sự thay đổi
    String path = "/env/Temperature";  // Đường dẫn Firebase
    Firebase.setString(firebaseData, path, message );
    
    // Kiểm tra nếu Firebase operation thất bại
    if (firebaseData.httpCode() != 200) {
      Serial.println("Failed to push temperature data to Firebase: " + firebaseData.errorReason());
    } else {
      Serial.println("Successfully pushed temperature data to Firebase.");
      lastTemperature = message; // Lưu lại giá trị mới
    }
  }

  // Kiểm tra nếu là topic air
  if (topicStr == air && message != lastAirQuality) {
    // Đẩy dữ liệu vào Firebase nếu có sự thay đổi
    air_state = message;
      Serial.println(water_state + "----------"+cleaner_state+"---"+air_state);
    String path = "dev/Air";  // Đường dẫn Firebase
    Firebase.setString(firebaseData, path, message);
    
    // Kiểm tra nếu Firebase operation thất bại
    if (firebaseData.httpCode() != 200) {
      Serial.println("Failed to push air quality data to Firebase: " + firebaseData.errorReason());
    } else {
      Serial.println("Successfully pushed air quality data to Firebase.");
      lastAirQuality = message; // Lưu lại giá trị mới
    }
  }

  // Kiểm tra nếu là topic water state
  if (topicStr == water && message != lastWaterState) {
    // Đẩy dữ liệu vào Firebase nếu có sự thay đổi
    water_state = message;
      Serial.println(water_state + "----------"+cleaner_state+"---"+air_state);
    String path = "dev/WaterHeater";  // Đường dẫn Firebase
    Firebase.setString(firebaseData, path, message);
    
    // Kiểm tra nếu Firebase operation thất bại
    if (firebaseData.httpCode() != 200) {
      Serial.println("Failed to push water state data to Firebase: " + firebaseData.errorReason());
    } else {
      Serial.println("Successfully pushed water state data to Firebase.");
      lastWaterState = message; // Lưu lại giá trị mới
    }
  }

    // Kiểm tra nếu là topic cleaner
  if (topicStr == cleaner && message != lastCleaner) {
    // Đẩy dữ liệu vào Firebase nếu có sự thay đổi
    cleaner_state = message;
    String path = "dev/Cleaner";  // Đường dẫn Firebase
    Firebase.setString(firebaseData, path, message);
    
    // Kiểm tra nếu Firebase operation thất bại
    if (firebaseData.httpCode() != 200) {
      Serial.println("Failed to push cleaner to Firebase: " + firebaseData.errorReason());
    } else {
      Serial.println("Successfully pushed cleaner data to Firebase.");
      lastCleaner = message; // Lưu lại giá trị mới
    }
  }

  //kiểm tra nếu là id yêu cầu mở cửa
  if (strcmp(topic,RFID)==0)
  {
    if(!message.equalsIgnoreCase("finish"))
    {
      getUserDataFromFirebase(message);
    }
    else
    {
      String path = "ENTRY/Name";  // Đường dẫn Firebase
      Firebase.setString(firebaseData, path, "    " );
    }
  }

  //fixed test cases
  if (strcmp(topic,test)==0)
  {
    uint8_t i=0;
    uint8_t current_h = 0;
    uint8_t current_m = 0;
    uint8_t current_s = 0;

    //shift iterator until first ':' 
    while(message[i]!=':'){
      i++;
    }

    //load hour value
    current_h=((message[i-2]-'0')*10) + ((message[i-1]-'0')); 
    i++;
    //shift iterator until second ':' 
    while(message[i]!=':'){
      i++;
    }

    //load minute value
    current_m=((message[i-2]-'0')*10) + ((message[i-1]-'0')); 

    //load second value
    current_s=((message[i+1]-'0')*10) + ((message[i+2]-'0')); 
    
    auto_flag=true;
    //handle exception
    if (current_h>24 && current_h < 0) {
      current_h = 0;
      auto_flag=false;
    }
    if (current_m>60 && current_m < 0) {
      current_m = 0;
      auto_flag=false;
    }
    if (current_s>60 && current_s < 0) {
      current_s = 0;
      auto_flag=false;
    }
      Serial.println("hour : " + String(current_h) + "min : " + String(current_m) + "second : " + String(current_s));

    //unify into minute
    current_uni = current_h * 60 + current_m;
    Serial.print("Time get from test: ");
      Serial.println(current_uni);
  }
}

// MQTT Initialization
void mqtt_init() {
  client.setServer(mqtt_server, mqttPort);
  client.setCallback(mqttCallback);  // Set the callback function for handling messages

  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("Connected to MQTT");
      client.subscribe(DHT1_humid);   // Subscribe to DHT1_humid
      client.subscribe(DHT1_temp);    // Subscribe to DHT1_temp
      client.subscribe(air);          // Subscribe to air
      client.subscribe(water);       // Subscribe to water state
      client.subscribe(RFID);        // Subscribe to RFID
      client.subscribe(cleaner);     // Subscribe to cleaner
      client.subscribe(test);         // Subscribe to test
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

// MQTT Reconnect
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");

      client.subscribe(DHT1_humid);   // Subscribe to DHT1_humid
      client.subscribe(DHT1_temp);    // Subscribe to DHT1_temp
      client.subscribe(air);          // Subscribe to air
      client.subscribe(water);       // Subscribe to water state
      client.subscribe(RFID);        // Subscribe to RFID
      client.subscribe(cleaner);     // Subscribe to cleaner
      client.subscribe(test);         // Subscribe to test
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// Blynk button callback for controlling Water Heater
BLYNK_WRITE(V0) {
  int buttonState = param.asInt();
  if (buttonState == 1) {
    client.publish("req", "won");  // Send MQTT message
  } else {
    client.publish("req", "woff");  // Send MQTT message
  }
}

// Blynk button callback for controlling Cleaner
BLYNK_WRITE(V1) {
  int buttonState = param.asInt();
  if (buttonState == 1) 
  {
    client.publish("req", "clean");  // Send MQTT message
  }
}

// Blynk button callback for controlling Cleaner
BLYNK_WRITE(V2) {
  int val = param.asInt();
  if (val > 15 && val <= 30) 
  {
    char buf[3]={val/10 + '0',val%10 + '0', '\0'};
    client.publish("req", buf);  // Send MQTT message
  }
  else
  {
    client.publish("req", "off");  // Send MQTT message
  }
}

// Firebase data push and MQTT publishing (same as before, with minor adjustments)
void getUserDataFromFirebase(String chr) {
  String path = "/usersmode/" + chr;

  if (Firebase.getJSON(firebaseData, path)) {
    Serial.println("Successfully fetched user data from Firebase:");
    FirebaseJson json;
    json.setJsonData(firebaseData.jsonString());
    FirebaseJsonData jsonData;

    String id, name, pass;

    if (json.get(jsonData, "id")) {
      id = jsonData.stringValue;
      Serial.println("User ID: " + id);
    }

    if (json.get(jsonData, "name")) {
      name = jsonData.stringValue;
      Serial.println("User Name: " + name);
    }

    if (json.get(jsonData, "pass")) {
      pass = jsonData.stringValue;
      Serial.println("User Pass: " + pass);
    }

    // Publish data to MQTT after fetching
    publishToMQTT(id, name, pass);

  } else {
    Serial.println("Failed to fetch data from Firebase");
    Serial.println("Reason: " + firebaseData.errorReason());
  }
}

// Function to push to MQTT
void publishToMQTT(String id, String name, String pass) {
  String idPayload = id;
  if (client.publish("UID", idPayload.c_str())) {
    Serial.println("Published to ID topic: " + idPayload);
  }

  String namePayload = name;
  if (client.publish("Name", namePayload.c_str())) {
    Serial.println("Published to Name topic: " + namePayload);
  }

  String path = "ENTRY/Name";  // Path to Firebase
  Firebase.setString(firebaseData, path, "Welcome " + name + " [" + id + "] home.");

  entry_flag = true;
  entry_time = millis();

  String passPayload = pass;
  if (client.publish("Key", passPayload.c_str())) {
    Serial.println("Published to Pass topic: " + passPayload);
  }
}

void checkFire(){
  if (FIRE)
  {
    Blynk.logEvent("FIRE","Fire Detected");
    Serial.println("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
    FIRE=false;
  }
}

#define EST_CYCLE 5000
unsigned long est_last;
void est_update(){
  if((millis() - est_last) > EST_CYCLE){
    est_arrive();
    est_last=millis();
  }
}

void est_arrive(){
  uint8_t est_h,est_m,est_s;
  String path = "/TimeComeBack";

   if (Firebase.getJSON(firebaseData, path)) 
   {
    Serial.println("Successfully fetched TIME data from Firebase:");
    FirebaseJson json;
    json.setJsonData(firebaseData.jsonString());
    FirebaseJsonData jsonData;

    String buf;

    if (json.get(jsonData, "Time")) {
      buf = jsonData.stringValue;
      Serial.println("Arrive time estimated: " + buf);
      
      uint8_t i=0;

      //shift iterator until first ':' 
      while(buf[i]!=':'){
        i++;
      }

      //load hour value
      est_h=((buf[i-2]-'0')*10) + ((buf[i-1]-'0')); 
      i++;
      //shift iterator until second ':' 
      while(buf[i]!=':'){
        i++;
      }

      //load minute value
      est_m=((buf[i-2]-'0')*10) + ((buf[i-1]-'0')); 

      //load second value
      est_s=((buf[i+1]-'0')*10) + ((buf[i+2]-'0')); 
    
      //handle exception
      if (est_h>24 && est_h < 0) est_h = 0;
      if (est_m>60 && est_m < 0) est_m = 0;
      if (est_s>60 && est_s < 0) est_s = 0;
      Serial.println("hour : " + String(est_h) + "min : " + String(est_m) + "second : " + String(est_s));

      //unify into minute
      est_uni = est_h * 60 + est_m;

      Serial.print("Time get: ");
      Serial.println(est_uni);
    }
   }else {
    Serial.println("Failed to fetch data from Firebase");
    Serial.println("Reason: " + firebaseData.errorReason());
    return ;
  }
}

void do_home_automation(){
    Serial.println("Home Smart Automated!!!");
    client.publish("req","won");
    delay(5000);
    client.publish("req","clean");
    delay(5000);
    client.publish("req","25");
    delay(5000);
    //   client.loop();
    // while(water_state.equalsIgnoreCase("OFF")||cleaner_state.equalsIgnoreCase("")){
    //   Serial.println(water_state + "---"+cleaner_state+"----------"+air_state);
    //   client.publish("req","won");
    //   client.loop();
    //   delay(5000);
    // }
    // while(cleaner_state.equalsIgnoreCase("ready")||cleaner_state.equalsIgnoreCase("")){
    //   Serial.println(water_state + "------"+cleaner_state+"----------"+air_state);
    //   client.publish("req","clean");
    //   delay(5000);
    // }
    // while(air_state.equalsIgnoreCase("OFF")||air_state.equalsIgnoreCase("")){
    //   Serial.println(water_state + "--------------"+cleaner_state+"----------"+air_state);
    //   client.publish("req","25");
    //   delay(5000);
    // }
}

void time_check(){
  if (((est_uni - current_uni) < 10) && ((est_uni - current_uni) > 1) && auto_flag){
    // auto_mark_time = millis();
    Serial.println("Home automation processing!!");
    do_home_automation();
    auto_flag = false;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  wifi_init();  // Initialize Wi-Fi and Blynk
  config.host = FIREBASE_HOST; // Configure Firebase
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);  // Begin Firebase connection
  Firebase.reconnectWiFi(true);
  Serial.println("Firebase setup completed.");
  mqtt_init();  // Initialize MQTT

  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  if (!client.connected()) {
    reconnect();  // Ensure MQTT is connected
  }
  client.loop();  // Keep the MQTT client running
  Blynk.run();    // Keep the Blynk client running
  timer.run();
  checkFire();
  est_update();
  time_check();
}
