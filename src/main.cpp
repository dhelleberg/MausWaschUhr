#include <Arduino.h>
#include <HCSR04.h>
#include "DFRobotDFPlayerMini.h"
#include <FastLED.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include "esp32-hal-cpu.h"
#include "time.h"

HardwareSerial hwSerial(1);
DFRobotDFPlayerMini dfPlayer;
int volume = 17;

/*Put your SSID & Password*/
const char* ssid = WSSID;  // Enter SSID here
const char* password = WPWD;  //Enter Password here
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int daylightOffset_sec = 3600;


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

RTC_DATA_ATTR int bootCount = 0; 

long washStarted = 0;
long overAllCounterTime = 0;
float ledFactor = 0;
int ledCounter = -1;
const long WASH_TIME = 20 * 1000; // 20sec

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */


UltraSonicDistanceSensor distanceSensor(15, 4);

void leds_off() {
    for (int i = 0; i < PIXEL_COUNT; i++)
    { // For each pixel...
      leds[i] = CRGB::Black;
      FastLED.show();   // Send the updated pixel colors to the hardware.
    }
}
bool printLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return false;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  return true;
}


void setup()
{
  btStop();
  
  //esp_wifi_set_ps(WIFI_PS_MODEM);
  FastLED.addLeds<NEOPIXEL, PIXEL_PIN>(leds, PIXEL_COUNT);
  hwSerial.begin(9600, SERIAL_8N1, 17, 16); // speed, type, TX, RX
  // put your setup code here, to run once:
  Serial.begin(115200);

  delay(500);
  bootCount = bootCount+1;
  Serial.printf("bootcounter %d ",bootCount);
  
  /*WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  

  //check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected..!");
  Serial.print("Got IP: ");  Serial.println(WiFi.localIP());
  bool timeOK = false;
  while(!timeOK) {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    timeOK = printLocalTime();
    delay(100);
  }  */
  
  
  

  //disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  
  Serial.println("init player...");
  
  while (!dfPlayer.begin(hwSerial))
  {
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    dfPlayer.reset();
    hwSerial.end();
    delay(1000);
    
  }
  Serial.println("player online");
  dfPlayer.setTimeOut(500);
  dfPlayer.volume(volume); // Set volume value (0~30).
  dfPlayer.EQ(DFPLAYER_EQ_NORMAL);
  dfPlayer.outputDevice(DFPLAYER_DEVICE_SD);
  
  leds_off();
  
}
void powerDown(uint64_t sleepInMin) {
  
  delay(500);
  Serial.printf("power down... for deep sleep... %d", sleepInMin);
  const uint64_t sleepTime = sleepInMin * 60 * uS_TO_S_FACTOR;
  esp_sleep_enable_timer_wakeup(sleepTime);
  Serial.flush();
  delay(100);
  dfPlayer.reset();
  hwSerial.end();
  delay(100);
  esp_deep_sleep_start(); //Gute Nacht!*/
}

void checkSleep() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  int hour = timeinfo.tm_hour +1;
  if(hour >= 20 || hour <= 6) {
    //calc hours to sleep.
    uint64_t hoursToSleep = 24 - hour + 6;
    Serial.printf("going to sleep for %d hours...\n",hoursToSleep);
    //powerDown(hoursToSleep * 60);
    
    powerDown(1);
  }

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
    dfPlayer.sleep();
    setCpuFrequencyMhz(80);
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
    //checkSleep();
    Serial.println(distance);
    if (distance < MIN_DISTANCE && distance > -1)
    {
      setCpuFrequencyMhz(240);
      dfPlayer.outputDevice(DFPLAYER_DEVICE_SD);
      mode = MODE_INIT_ANIM;
      initLEDs();
      washStarted = millis();
      dfPlayer.play(1);
      break;
    }
    else
      delay(700);
    break;
  }
  case MODE_INIT_ANIM:
    doFadeAnim();    
    break;
  case MODE_COUNTER:
    doClockCount();
    break;

  }
}