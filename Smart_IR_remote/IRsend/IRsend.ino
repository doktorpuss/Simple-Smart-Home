
#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#if !defined(ARDUINO_ESP32C3_DEV) // This is due to a bug in RISC-V compiler, which requires unused function sections :-(.
#define DISABLE_CODE_FOR_RECEIVER // Disables static receiver code like receive timer ISR handler and static IRReceiver and irparams data. Saves 450 bytes program memory and 269 bytes RAM if receiving functions are not required.
#endif
//#define SEND_PWM_BY_TIMER         // Disable carrier PWM generation in software and use (restricted) hardware PWM.
//#define USE_NO_SEND_PWM           // Use no carrier PWM, just simulate an active low receiver signal. Overrides SEND_PWM_BY_TIMER definition

#include "PinDefinitionsAndMore.h"
#include <IRremote.hpp> // include the library

#define AIR_ON 0xC3
#define AIR_OFF 0xFF
#define CLEAN 0x3C
#define RELAY D0

uint8_t sCommand = 0xAB;
uint8_t sRepeats = 0;

// const char* ssid = "Ngoi Nha Vui Ve 456";
// const char* password = "0967936828";

// const char* ssid = "The Anh";
// const char* password = "theanh18923";

// const char* ssid = "Lymart t2";
// const char* password = "0938925258";

const char* ssid = "qqqq";
const char* password = "33339999";

const char* mqtt_server = "192.168.47.178";

const char* subcribeTopic = "req";
const char* publishTopic = "Scammer so 1 vn";  // Topic để gửi message đi
const int mqttPort = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      client.subscribe(subcribeTopic);
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void IR_send(uint8_t command){
    Serial.println();
    Serial.print(F("Send now: address=0x00, command=0x"));
    Serial.print(command, HEX);
    Serial.println();

    Serial.println(F("Send standard NEC with 8 bit address"));
    Serial.flush();

    // Receiver output for the first loop must be: Protocol=NEC Address=0x102 Command=0x34 Raw-Data=0xCB340102 (32 bits)
    IrSender.sendNEC(0x00, command, sRepeats);

}

void process(char chr[]){
  if (strcmp( chr , "woff")==0){
    digitalWrite(RELAY,LOW);
  }
  else if (strcmp( chr , "won")==0){
    digitalWrite(RELAY,HIGH);
  }
  else if (strcmp( chr , "off")==0){
    IR_send(AIR_OFF);
  }
  else if ('9'>= chr[0] && '0'<= chr[0] && '0'<= chr[1] && '9'>= chr[1]){
    IR_send((chr[0]-'0')*10+(chr[1]-'0'));
  }
  else if (strcmp( chr , "clean")==0){
    IR_send(CLEAN);
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
  Serial.println();

  temp[length]='\0';

  Serial.print("payload size: ");
  Serial.print(length);
  Serial.print(";   Payload content: ");
  Serial.println(temp);

  process(temp);
}

void wifi_init(){
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void mqtt_init(){
  client.setServer(mqtt_server, mqttPort);
  client.setCallback(callback);

  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP8266Client2")) {
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

void IR_send_init(){
  // Just to know which program is running on my Arduino
  Serial.println(F("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_IRREMOTE));
  Serial.print(F("Send IR signals at pin "));
  Serial.println(IR_SEND_PIN);

  /*
    * The IR library setup. That's all!
    */
  IrSender.begin(); // Start with IR_SEND_PIN -which is defined in PinDefinitionsAndMore.h- as send pin and enable feedback LED at default feedback LED pin
  disableLEDFeedback(); // Disable feedback LED at default feedback LED pin
}

void command_listen(){
  if(Serial.available()){
    
    String str = Serial.readStringUntil('\n');
    char chr[str.length()+1];
    str.toCharArray(chr, str.length()+1);
    Serial.print("get command:");
    Serial.println(chr);

    if (strcmp( chr , "woff")==0)
    {
      digitalWrite(RELAY,LOW);
    }
    else if (strcmp( chr , "won")==0)
    {
      digitalWrite(RELAY,HIGH);
    }
    else if (strcmp( chr , "off")==0)
    {
      IR_send(AIR_OFF);
    }
    else if ('9'>= chr[0] && '0'<= chr[0] && '0'<= chr[1] && '9'>= chr[1])
    {
      IR_send((chr[0]-'0')*10+(chr[1]-'0'));
    }
    else if (strcmp( chr , "clean")==0)
    {
      IR_send(CLEAN);
    }
  }
}

void setup() {
    pinMode(RELAY, OUTPUT);

    Serial.begin(115200);
    while (!Serial); // Wait for Serial to become available. Is optimized away for some cores.

    IR_send_init();
    wifi_init();
    mqtt_init();
    
    client.publish("transmitter","READY");
}

void loop() {
    client.loop();
    command_listen();
    delay(3000);  // delay must be greater than 5 ms (RECORD_GAP_MICROS), otherwise the receiver sees it as one long signal
}
