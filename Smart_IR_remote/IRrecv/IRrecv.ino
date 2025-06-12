#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "PinDefinitionsAndMore.h"
#include <IRremote.hpp>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define WATER D8

#define PROC_TIM 300
#define SLEEP_TIM 5000

bool water;

bool air_status=false;
uint8_t air_temp;

bool vacuum_sleep=true;
bool vacuum_done=false;
unsigned long vacuum_start;
uint8_t vacuum_work;

// const char* ssid = "Ngoi Nha Vui Ve 456";
// const char* password = "0967936828";

// const char* ssid = "The Anh";
// const char* password = "theanh18923";

const char* ssid = "qqqq";
const char* password = "33339999";

// const char* ssid = "Lymart t2";
// const char* password = "0938925258";

const char* mqtt_server = "192.168.47.178";

const char* subcribeTopic = "req";
const int mqttPort = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

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

  // if(strcmp(temp,"state")==0){
  //   client.publish("cleaner_state",);
  //   client.publish("air_state",);
  //   client.publish("water_state",);
  // }
  // Serial.println();
}

  

void IR_recv_init(){
  // Just to know which program is running on my Arduino
  Serial.println(F("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_IRREMOTE));

  // Start the receiver and if not 3. parameter specified, take LED_BUILTIN pin from the internal boards definition as default feedback LED
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);

  Serial.print(F("Ready to receive IR signals of protocols: "));
  printActiveIRProtocols(&Serial);
  Serial.println(F("at pin " STR(IR_RECEIVE_PIN)));
}

void OLED_init(){
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  
  // display.drawPixel(10, 10, SSD1306_WHITE);
  display.display();
  delay(2000); // Pause for 2 seconds
  display.clearDisplay();

  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text

  display.setCursor(0,0);             // Start at top-left corner
  display.println(F("Air conditioner:"));
  display.setCursor(100,0);
  display.println(F("OFF"));
  
  display.setCursor(0,10);
  display.println(F("Water heater:"));
  display.setCursor(100,10);
  display.println(F("OFF"));

  display.setCursor(0,20);
  display.println(F("Cleaner: "));
  display.setCursor(70,20);
  display.println(F("SLEEPING"));

  display.display();
}

void IR_recv_process(){
  // Check if received data is available and if yes, try to decode it
  if (IrReceiver.decode()) {
    if(IrReceiver.decodedIRData.command!=0x00){
      // In ra dữ liệu raw đầy đủ nhận được từ cảm biến IR
      Serial.print(F("Raw HEX: "));
      Serial.print(IrReceiver.decodedIRData.decodedRawData, HEX);
      Serial.print(F("====>Command HEX: "));
      Serial.println(IrReceiver.decodedIRData.command,HEX);

      //Execute command
      executeIrCommand(IrReceiver.decodedIRData.command);
      IrReceiver.decodedIRData.command=0x00;
    }
    // Tiếp tục nhận tín hiệu mới
    IrReceiver.resume();
  }
}

void cleaner_work_update(){
  if(!vacuum_sleep) 
  {
    vacuum_work=(uint8_t)((millis()-vacuum_start)/PROC_TIM);
  }
  if(vacuum_work>=100) {
    vacuum_work=0;

    vacuum_sleep=true;
    vacuum_done=true;
    vacuum_start=millis();
    Serial.println("Cleaner done");
    client.publish("cleaner","done"); //upadate
  }
  if(vacuum_done && ((millis()-vacuum_start)>SLEEP_TIM)){
    vacuum_done=false;
    Serial.println("Cleaner ready");
    client.publish("cleaner","ready"); //upadate
  }
}

void water_update(){
  //update 13/12
  if(water==digitalRead(WATER)){
    water=!digitalRead(WATER);
    if(!water)
    {
      client.publish("water","OFF");
    }
    else
    {
      client.publish("water","ON");
    }
  }
  //end update

  display.setCursor(0,10);
  display.println(F("Water heater:"));

  display.setCursor(100,10);
  if(!water){
    display.println(F("OFF"));
  }
  else{
    display.println(F("ON "));
  }
}

void air_update(){
  display.setCursor(0,0);             
  display.println(F("Air conditioner:"));
    
  display.setCursor(100,0);
  if (air_status){
    display.println(air_temp);
  }
  else{
    display.println(F("OFF"));
  }
}

void cleaner_update(){
  display.setCursor(0,20);
  display.println(F("Cleaner: "));

  if(vacuum_sleep){
    if(vacuum_done){
      display.setCursor(65,20);
      display.println(F("COMPLETED"));
    }
    else{
      display.setCursor(70,20);
      display.println(F("SLEEPING"));
    }
  } 
  else{
    display.setCursor(100,20);
    display.print(vacuum_work);
    display.println("%");
  }
}

void OLED_update(){
  display.clearDisplay();
  water_update();
  cleaner_update();
  air_update();
  display.display();
}

void executeIrCommand(uint8_t command){
  switch (command) {
    case 0xC3:
      air_status = true;
      Serial.println("Air conditioner: ON");
      break;
    case 0xFF:
      air_status = false;
      Serial.println("Air conditioner: OFF");
      client.publish("air","OFF"); //update
      break;
    case 0x3C:
      vacuum_sleep = false;
      vacuum_start = millis();
      client.publish("cleaner","running"); //update
      Serial.println("Cleaner run");
      break;
    default:
      break;
  }

  if((command < 31) && (command > 15)){
      air_temp = command;
      const char chr[3]= {command/10+'0',command%10+'0','\0'};
      air_status = true;

      Serial.print("Air conditioner: ");
      Serial.print(command);
      Serial.println("°C");
      client.publish("air",chr); //update
  }
}

void command_listen(){
  if(Serial.available()){
    String str = Serial.readStringUntil('\n');
    char chr[str.length()+1];
    str.toCharArray(chr, str.length()+1);
    Serial.print("get command:");
    Serial.println(chr);

    client.publish("req",chr);

    // if (strcmp( chr , "toggle")==0){
    //   IR_send(TOGGLE);
    //   pinToggle(RELAY);
    // }
    // else if (strcmp( chr , "on")==0){
    //   IR_send(AIR_ON);
    // }
    // else if (strcmp( chr , "off")==0){
    //   IR_send(AIR_OFF);
    // }
    // else if ('9'>= chr[0] && '0'<= chr[0] && '0'<= chr[1] && '9'>= chr[1]){
    //   IR_send((chr[0]-'0')*10+(chr[1]-'0'));
    // }
    // else if (strcmp( chr , "clean")==0){
    //   IR_send(CLEAN);
    // }
  }
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

void setup() {
    Serial.begin(115200);
    while (!Serial); // Wait for Serial to become available. Is optimized away for some cores.

    IR_recv_init();
    OLED_init();
    wifi_init();
    mqtt_init();

    client.publish("receiver","READY");
}

void loop() {
    client.loop();
    IR_recv_process();
    cleaner_work_update();
    OLED_update();
    command_listen();
}
