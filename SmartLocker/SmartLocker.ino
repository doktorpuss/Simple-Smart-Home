#include <SPI.h>
#include <MFRC522.h>
#include <string.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

#define RST_PIN D0
#define SS_PIN D8
#define LOCK_LED D4

LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 mfrc522(SS_PIN, RST_PIN);

// const char* ssid = "The Anh";
// const char* password = "theanh18923";

const char* ssid = "qqqq";
const char* password = "33339999";

const char* mqtt_server = "192.168.47.178";

const char* nameTopic = "Name";
const char* passTopic = "Key";
const char* uidTopic = "UID";  // Topic để gửi đi
const char* publishTopic = "RFID";  // Topic để gửi message đi

bool loading_flag = false;  // Cờ kiểm tra trạng thái chờ nhận thông tin từ MQTT

const int mqttPort = 1883;
WiFiClient espClient;
PubSubClient client(espClient);

typedef struct user {
  String name = "";
  String uid = "";
  String key = "";

  bool isReady() {
    return !name.equals("") && !key.equals("") && !uid.equals("");
  }
} user;

user *user_list = NULL;

void wifi_init() {
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      
      // Subscribe to both topics
      client.subscribe(passTopic);
      client.subscribe(nameTopic);
      client.subscribe(uidTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  char temp[length];
  Serial.print("Message received on topic: ");
  Serial.println(topic);

  Serial.print("Message: ");
  for (int i = 0; i < length; i++) {
    temp[i] = (char)payload[i];
  }
  temp[length] = '\0';
  Serial.println(temp);

  // Update user data based on received topic
  if (strcmp(topic, uidTopic) == 0) {
    user_list->uid = String(temp);
  }
  else if (strcmp(topic, nameTopic) == 0) {
    user_list->name = String(temp);
  }
  else if (strcmp(topic, passTopic) == 0) {
    user_list->key = String(temp);
  }

  // Set loading_flag to false once all information is received
  if (user_list->isReady()) {
    loading_flag = false;
  }
}

void mqtt_init() {
  client.setServer(mqtt_server, mqttPort);
  client.setCallback(callback);

  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("Connected to MQTT");

      // Subscribe to both topics
      client.subscribe(passTopic);
      client.subscribe(nameTopic);
      client.subscribe(uidTopic);
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println(F("Read personal data on a MIFARE PICC:"));
  lcd.init();
  lcd.backlight();
  pinMode(LOCK_LED, OUTPUT);

  wifi_init();
  mqtt_init();

  client.publish(publishTopic, "READY");
  client.publish(nameTopic, "READY");
  client.publish(passTopic, "READY");

  user_list = new user(); // Allocate memory for user_list
}

void loop() {
  client.loop();
  digitalWrite(LOCK_LED, LOW);

  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  Serial.println(F("**Card Detected:**"));

  String uidString = getUIDString();

  // Publish UID to MQTT
  client.publish(uidTopic, uidString.c_str());

  // Wait until we receive complete information (name and key)
  loading_flag = true;
  while (loading_flag) {
    client.loop(); // Process MQTT messages while waiting
    delay(100);
  }

  // Now check credentials once all data is received
  String name = user_list->name;
  String pass = user_list->key;

  if (checkCredentials(uidString, name, pass)) {
    displayWelcomeMessage(name);
  } else {
    displayWrongMessage();
  }

  Serial.println(F("\n**End Reading**\n"));

  delay(1000);
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

String getUIDString() {
  String uidString = "";

  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uidString += String(mfrc522.uid.uidByte[i], HEX);
    if (i < mfrc522.uid.size - 1) {
      uidString += " ";
    }
  }
  
  Serial.println();
  Serial.print(uidString);
  Serial.println();

  return uidString;
}

bool checkCredentials(String uidString, String name, String pass) {
  String UID1 = "33 CF ED 2F";
  String saved_name = "An";
  String saved_pass = "12345678";

  return uidString.equalsIgnoreCase(UID1) && name.equalsIgnoreCase(saved_name) && pass.equalsIgnoreCase(saved_pass);
}

void displayWelcomeMessage(String name) {
  lcd.setCursor(1, 0);
  lcd.print("WELCOME");
  lcd.setCursor(1, 1);
  lcd.print(name);
  digitalWrite(LOCK_LED, HIGH);
  delay(3000);
  lcd.clear();
}

void displayWrongMessage() {
  lcd.setCursor(1, 0);
  lcd.print("WRONG");
  delay(3000);
  lcd.clear();
}
