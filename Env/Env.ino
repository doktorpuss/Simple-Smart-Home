#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define DHTPIN D5         // Chân kết nối DHT11 (GPIO14)
#define DHTTYPE DHT11
#define PERIOD 2000
DHT dht(DHTPIN, DHTTYPE);

#define SMOKE_SENSOR_PIN A0  // Chân kết nối cảm biến khói (GPIO12)
#define BUZZER_PIN D4        // Chân kết nối Buzzer (GPIO13)
int smokeThreshold = 800;    // Ngưỡng phát hiện khói

// const char* ssid = "The Anh";
// const char* password = "theanh18923";

const char* ssid = "qqqq";
const char* password = "33339999";

// const char* ssid = "Lymart t2";
// const char* password = "0938925258";
const char* mqtt_server = "192.168.47.178";

const char* subcribeTopic = "DHT";
const char* temperTopic = "DHT1_temp";  // Topic để gửi message đi
const char* humidTopic = "DHT1_humid";  // Topic để gửi message đi
const int mqttPort = 1883;

unsigned long masked_t;

WiFiClient espClient;
PubSubClient client(espClient);

void DHTrun(){
    String str;
  // Đọc dữ liệu từ DHT11
    char humidity[8];
    dtostrf(dht.readHumidity()+40.0,5,2,humidity);
    char temperature[8];
    dtostrf(dht.readTemperature()+23.0,5,2,temperature);

    // if (isnan(humidity) || isnan(temperature)) {
    //     Serial.println(F("Lỗi đọc DHT11!"));
    //     return;
    // }

    // Hiển thị nhiệt độ và độ ẩm trên OLED
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("Nhiet do: ");
    display.print(temperature);
    display.print(" C");
    display.setCursor(0, 20);
    display.print("Do am: ");
    display.print(humidity);
    display.print(" %");
    display.display();

  client.publish(humidTopic, humidity);
  client.publish(temperTopic,temperature);
}

void wifi_init(){
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
    Serial.print((char)payload[i]);
    temp[i]=(char)payload[i];
  }
  temp[length]='\0';

  if(strcmp(temp,"run")){
    DHTrun();
  }

  Serial.println();
}

void mqtt_init(){
  client.setServer(mqtt_server, mqttPort);
  client.setCallback(callback);

  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("Connected to MQTT");

      // Subscribe to both topics
      client.subscribe(subcribeTopic);
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

unsigned long DHT_timer;
#define DHT_CYCLE 5000
void DHT_update(){
  if(millis() - DHT_timer > DHT_CYCLE){
    DHTrun();
    DHT_timer=millis();
  }
}

void setup() {
    Serial.begin(115200);
    pinMode(SMOKE_SENSOR_PIN, INPUT); 
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, HIGH);

    dht.begin();

    // Khởi tạo OLED
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("Không tìm thấy OLED!"));
        for (;;);
    }
    display.clearDisplay();

  wifi_init();
  mqtt_init();
  DHTrun();
  masked_t=millis();
  client.publish(subcribeTopic,"ok");

  DHT_timer=millis();
}

void loop() {
    
    client.loop();
    // Đọc mức khói
    int smokeLevel = analogRead(SMOKE_SENSOR_PIN);

    // Kiểm tra mức khói và cảnh báo
    if (smokeLevel > smokeThreshold) {
        digitalWrite(BUZZER_PIN, LOW);  // Buzzer kêu
              
        client.publish(humidTopic,"FIRE");
        client.publish(temperTopic,"FIRE");
        delay(1000);
    } else {
        digitalWrite(BUZZER_PIN, HIGH);   // Tắt buzzer
    }

    // unsigned long current_t=millis();
    // if ((current_t - masked_t)>=PERIOD){
    //   DHTrun();
    //   masked_t=currr;
    //   Serial.print("Mức khói: ");
    //   Serial.println(smokeLevel);
    // }

    DHT_update();
    Serial.print("Mức khói: ");
    Serial.println(smokeLevel);
    delay(200);
}
