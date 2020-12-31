// https://randomnerdtutorials.com/esp32-pinout-reference-gpios/
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <secrets.h>
const unsigned long BOT_MTBS = 1000; // mean time between scan messages

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
unsigned long bot_lasttime; // last time messages' scan has been done

// outputs
const int ledPin = 16; 
const int warmLight = 4;
const int coldLight = 5;
const int warmPWM = 0;
const int coldPWM = 1;
const int freq = 15000; // 5000 Khz produced a hum at 50% duty cycle, so brought this down
const int resolution = 12;

// inputs
const int red_btn_up = 33;
const int black_btn_down = 32;
const int rightKnob = 39;
const int leftKnob = 36;

// variables
bool btnState = false;
bool oldBtn = false;
int minBrightness = 40;
int warmLevel;
int coldLevel;
float globalPower;
int rightVal;
int leftVal;
int filter = 100;
unsigned long turned_on;
int on_count = 0;
int off_count = 0;
int repeat_thresh = 1000;
String on_string;

void blink(int count);

void setup() {  
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  ledcSetup(warmPWM, freq, resolution);
  ledcSetup(coldPWM, freq, resolution);
  ledcAttachPin(ledPin, warmPWM);
  ledcAttachPin(warmLight, warmPWM);
  ledcAttachPin(coldLight, coldPWM);

  pinMode(red_btn_up,INPUT_PULLUP);
  pinMode(black_btn_down,INPUT_PULLUP);
  bot.sendMessage(lamp_chat_id, "The lamp has restarted", "");
}

void loop() {
  if (!digitalRead(red_btn_up)){
      delay(200);
      if (!digitalRead(black_btn_down)){
        while(!digitalRead(red_btn_up)){
          ArduinoOTA.handle();
        }
      }
  }

  leftVal = constrain(map(analogRead(leftKnob),0,3859,0,4095),0,4095);
  rightVal = constrain(map(analogRead(rightKnob),0,3853,0,4095),0,4095);

  if (digitalRead(black_btn_down) == true){
    ledcWrite(warmPWM,4095);
    ledcWrite(coldPWM,4095);
    off_count = 0;
    if (on_count<repeat_thresh){
      on_count++;
    }
    else if (on_count==repeat_thresh){
      turned_on = millis();
      on_count++;
    }
  }
  else{
    globalPower = rightVal;
    warmLevel = leftVal;
    coldLevel = 4095 - warmLevel;

    if (warmLevel<filter){
      warmLevel = 0;
    }
    else if (warmLevel>(4095-filter)){
      warmLevel = 4095;
    }

    if (coldLevel<filter){
      coldLevel = 0;
    }
    else if (coldLevel>(4095-filter)){
      coldLevel = 4095;
    }

    if (globalPower<filter){
      globalPower = 0;
    }
    else if (globalPower>(4095-filter)){
      globalPower = 4095;
    }
    globalPower = globalPower/4095;
    
    ledcWrite(warmPWM,warmLevel*globalPower);
    ledcWrite(coldPWM,coldLevel*globalPower);

    if(globalPower){
      off_count = 0;
      if (on_count<repeat_thresh){
        on_count++;
      }
      else if (on_count==repeat_thresh){
        turned_on = millis();
        on_count++;
      }
    }
    else{
      on_count = 0;
      if (off_count<repeat_thresh){
        off_count++;
      }
      else if (off_count==repeat_thresh){
        unsigned long all_seconds = (millis()-turned_on)/1000;
        int hours = all_seconds/3600;
        int secs_for_minutes = all_seconds%3600;
        int minutes = secs_for_minutes/60;
        int seconds = secs_for_minutes%60;
        if(hours){
          on_string = "I gifted you with light for: " + String(hours) + "h " + String(minutes) + "m " + String(seconds) + "s";
        }
        else if (minutes){
          on_string = "I gifted you with light for: " + String(minutes) + "m " + String(seconds) + "s";
        }
        else{
          on_string = "I gifted you with light for: " + String(seconds) + "s";
        }
        if(bot.sendMessage(lamp_chat_id,on_string, "")){
          blink(2);
        }
        else{
          blink(6);
        }
        off_count++;
      }
    }
  }
}

void blink(int count){
  for (int i = 0; i < count; i++){
    ledcWrite(warmPWM,100);
    ledcWrite(coldPWM,100);
    delay(100);
    ledcWrite(warmPWM,0);
    ledcWrite(coldPWM,0);
    delay(100);
  }
}
