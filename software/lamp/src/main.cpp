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


// void handleNewMessages(int numNewMessages)
// {
//   Serial.print("handleNewMessages ");
//   Serial.println(numNewMessages);

//   for (int i = 0; i < numNewMessages; i++)
//   {
//     String chat_id = bot.messages[i].chat_id;
//     String text = bot.messages[i].text;

//     String from_name = bot.messages[i].from_name;
//     if (from_name == "")
//       from_name = "Guest";

//     if (text == "/ledon")
//     {
//       // digitalWrite(ledPin, LOW); // turn the LED on (HIGH is the voltage level)
//       // ledStatus = 1;
//       bot.sendMessage(chat_id, "Led is ON", "");
//     }

//     if (text == "/ledoff")
//     {
//       // ledStatus = 0;
//       // digitalWrite(ledPin, HIGH); // turn the LED off (LOW is the voltage level)
//       bot.sendMessage(chat_id, "Led is OFF", "");
//     }

//     if (text == "/status")
//     {
//       // if (ledStatus)
//       bot.sendMessage(chat_id, String(chat_id), "");
//       if (1)
//       {
//         bot.sendMessage(chat_id, "Led is ON", "");
//       }
//       else
//       {
//         bot.sendMessage(chat_id, "Led is OFF", "");
//       }
//     }

//     if (text == "/start")
//     {
//       String welcome = "Welcome to Universal Arduino Telegram Bot library, " + from_name + ".\n";
//       welcome += "This is Flash Led Bot example.\n\n";
//       welcome += "/ledon : to switch the Led ON\n";
//       welcome += "/ledoff : to switch the Led OFF\n";
//       welcome += "/status : Returns current status of LED\n";
//       bot.sendMessage(chat_id, welcome, "Markdown");
//     }
//   }
// }

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
  // if (millis() - bot_lasttime > BOT_MTBS)
  // {
  //   int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

  //   while (numNewMessages)
  //   {
  //     Serial.println("got response");
  //     handleNewMessages(numNewMessages);
  //     numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  //   }

  //   bot_lasttime = millis();
  // }

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
  }
}