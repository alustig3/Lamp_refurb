#include <Arduino.h>

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

void setup() {  
  ledcSetup(warmPWM, freq, resolution);
  ledcSetup(coldPWM, freq, resolution);
  ledcAttachPin(ledPin, warmPWM);
  ledcAttachPin(warmLight, warmPWM);
  ledcAttachPin(coldLight, coldPWM);

  pinMode(red_btn_up,INPUT_PULLUP);
  pinMode(black_btn_down,INPUT_PULLUP);
}

void loop() {
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