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

UltraSonicDistanceSensor distanceSensor(15, 4);

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
}

void loop()
{
  // put your main code here, to run repeatedly:
  Serial.println(distanceSensor.measureDistanceCm());
  delay(500);
  for (int i = 0; i < PIXEL_COUNT; i++)
  { // For each pixel...
    leds[i].setRGB(255,30,0);
    FastLED.show();
    delay(100);
  }
  
  //dfPlayer.play(1);
}