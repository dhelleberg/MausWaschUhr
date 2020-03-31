#include <Arduino.h>
#include <HCSR04.h>
#include "DFRobotDFPlayerMini.h"
#include <FastLED.h>

HardwareSerial hwSerial(1);
DFRobotDFPlayerMini dfPlayer;
int volume = 20;

#define PIXEL_PIN 32
#define PIXEL_COUNT 12
CRGB leds[PIXEL_COUNT];

const int MIN_DISTANCE = 15;

const int MODE_INIT_ANIM = 1;
const int MODE_COUNTER = 2;
const int MODE_EFFECT = 3;
const int MODE_OFF = 0;
int mode = MODE_OFF;

int dimFactor = 0;
boolean dimDown = true;
int dimCounter = 0;

long washStarted = 0;
long overAllCounterTime = 0;
float ledFactor = 0;
int ledCounter = -1;
const long WASH_TIME = 20 * 1000; // 20sec

UltraSonicDistanceSensor distanceSensor(15, 4);

void leds_off() {
    for (int i = 0; i < PIXEL_COUNT; i++)
    { // For each pixel...
      leds[i] = CRGB::Black;
      FastLED.show();   // Send the updated pixel colors to the hardware.
    }
}

void setup()
{
  btStop();
  FastLED.addLeds<NEOPIXEL, PIXEL_PIN>(leds, PIXEL_COUNT);
  hwSerial.begin(9600, SERIAL_8N1, 17, 16); // speed, type, TX, RX
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("init player...");
  while (!dfPlayer.begin(hwSerial))
  {
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    delay(1000);
  }
  Serial.println("player online");
  dfPlayer.setTimeOut(500);
  dfPlayer.volume(volume); // Set volume value (0~30).
  dfPlayer.EQ(DFPLAYER_EQ_NORMAL);
  dfPlayer.outputDevice(DFPLAYER_DEVICE_SD);
  leds_off();
}

void initLEDs() {
    for (int i = 0; i < PIXEL_COUNT; i++)
    { // For each pixel...
      leds[i].setRGB(255, 30, 0);
      FastLED.show();    
    }
    dimFactor = 255;
    dimDown = true;
    dimCounter = 0;
}

void doFadeAnim() { 
  Serial.printf("dimFactor: %d\n",dimFactor);
  if(dimDown)
    dimFactor = dimFactor - 20;
  else
    dimFactor = dimFactor + 20;
  if(dimFactor < 20) {
    dimFactor = 20;
    dimDown = false;
  }
  else {
    if(dimFactor > 255) {
      dimCounter = dimCounter + 1;
      dimDown = true;
      dimFactor = 255;
      if(dimCounter == 2) {
        mode = MODE_COUNTER;
        leds_off();
      }          
    }
  }
  for (int i = 0; i < PIXEL_COUNT; i++)
  { // For each pixel...
    leds[i].setRGB(255, 30, 0);
    leds[i].fadeToBlackBy(dimFactor);    
  }
  FastLED.show();
  delay(30);
}

void doClockCount() {
  //calc counter
  int remainingSecs = ((washStarted + WASH_TIME) - millis()) / 100;
  Serial.printf("remaining s: %d\n",remainingSecs);  
  if(ledCounter == -1) //now calc the overall time for the clock 
  {
     overAllCounterTime = remainingSecs;
     ledFactor = (float)PIXEL_COUNT / (float)overAllCounterTime;
     Serial.printf("ledFactor: %f overAllCounterTime %d \n",ledFactor, overAllCounterTime);
  }
  //calc ledCounter
  ledCounter = (int)((overAllCounterTime - remainingSecs) * ledFactor);
  ledCounter = ledCounter + 1;
  if(ledCounter > PIXEL_COUNT)
    ledCounter = PIXEL_COUNT;
  Serial.printf("ledCounter %d\n",ledCounter);
  for (int i = 0; i < ledCounter; i++)
  { // For each pixel...
    leds[i].setRGB(255, 30, 0);    
  }
  FastLED.show();
  delay(20);


  if(remainingSecs <= 0) {
    delay(100);
    leds_off();
    mode = MODE_OFF;
    ledCounter = -1;    
  }
    
}

void loop()
{
  // put your main code here, to run repeatedly:
  switch (mode)
  {
  case MODE_OFF: {
    int distance = distanceSensor.measureDistanceCm();
    Serial.println(distance);
    if (distance < MIN_DISTANCE && distance > -1)
    {
      mode = MODE_INIT_ANIM;
      initLEDs();
      washStarted = millis();
      dfPlayer.play(1);
      break;
    }
    else
      delay(500);
    break;
  }
  case MODE_INIT_ANIM:
    doFadeAnim();    
    break;
  case MODE_COUNTER:
    doClockCount();
    break;

  }
  
  //dfPlayer.play(1);
}