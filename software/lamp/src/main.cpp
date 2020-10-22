// https://randomnerdtutorials.com/esp32-pinout-reference-gpios/
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

const char* ssid = "BecauseFi";
const char* password = "wearetherealmvps";
const int ledPin = 16;  // 16 corresponds to GPIO16
const int redBtn = 33;
const int blackBtn = 32;


const int warmLight = 4;
const int coldLight = 5;
const int rightKnob = 39;
const int leftKnob = 36;

int warmLevel;
int coldLevel;
float globalPower;

int minBrightness = 40;

const int freq = 5000;
const int warmPWM = 0;
const int coldPWM = 1;
const int resolution = 12;

int rightVal = 0;
int leftVal = 0;

void setup() {  
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
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

  pinMode(redBtn,INPUT_PULLUP);
  pinMode(blackBtn,INPUT_PULLUP);
}

void loop() {
  ArduinoOTA.handle();

  if (digitalRead(blackBtn)){
    leftVal = constrain(map(analogRead(leftKnob),0,3859,0,4095),0,4095);
    rightVal = constrain(map(analogRead(rightKnob),0,3853,0,4095),0,4095);

    globalPower = leftVal;
    warmLevel = rightVal;
    coldLevel = 4095 - warmLevel;

    coldLevel = 4095 - warmLevel;

    if (warmLevel<20){
      warmLevel = 0;
    }
    else if (warmLevel>4080){
      warmLevel = 4095;
    }

    if (coldLevel<20){
      coldLevel = 0;
    }
    else if (coldLevel>4080){
      coldLevel = 4095;
    }

    if (globalPower<minBrightness){
      globalPower = minBrightness;
    }
    else if (globalPower>4080){
      globalPower = 4095;
    }

    globalPower = globalPower/4095;

    ledcWrite(warmPWM,warmLevel*globalPower);
    ledcWrite(coldPWM,coldLevel*globalPower);
  }
  else{
    ledcWrite(warmPWM,0);
    ledcWrite(coldPWM,0);
  }
  delay(20);
}

