#include <Arduino.h>
#include <HCSR04.h>
#include "DFRobotDFPlayerMini.h"
#include <FastLED.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include "esp32-hal-cpu.h"
#include "time.h"
#include "SoftwareSerial.h"

HardwareSerial hwSerial(1);
DFRobotDFPlayerMini dfPlayer;
int volume = 20;

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
const int MODE_FINAL_ANIM = 4;
const int MODE_OFF = 0;
int mode = MODE_OFF;

const long END_ANIM_TIME = 2 * 1000;
int dimFactor = 0;
boolean dimDown = true;
int dimCounter = 0;

RTC_DATA_ATTR int bootCount = 0; 

long washStarted = 0;
long overAllCounterTime = 0;
long finalAnimStart = 0;
float ledFactor = 0;
int ledCounter = -1;
const long WASH_TIME = 18 * 1000; // 18sec

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */

void printDetail(uint8_t type, int value);


uint8_t gHue = 0; // rotating "base color" used by many of the patterns
  
// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimpleAnimList[])();
int animNumber = 0;

UltraSonicDistanceSensor distanceSensor(15, 4);
SoftwareSerial mySoftwareSerial(19,18);

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
  //setCpuFrequencyMhz(80);
  //esp_wifi_set_ps(WIFI_PS_MODEM);
  FastLED.addLeds<NEOPIXEL, PIXEL_PIN>(leds, PIXEL_COUNT);
  hwSerial.begin(9600, SERIAL_8N1, 17, 16); // speed, type, TX, RX
  // put your setup code here, to run once:
  Serial.begin(115200);
  mySoftwareSerial.begin(9600);
  delay(500);
  bootCount = bootCount+1;
  Serial.printf("bootcounter %d ",bootCount);
  
  WiFi.mode(WIFI_STA);
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
  }
  
  randomSeed(analogRead(0));
  

  //disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  
  Serial.println("init player...");
  
  while (!dfPlayer.begin(mySoftwareSerial))
  {
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));     
    delay(500);  
  }
  Serial.println("player online");
  dfPlayer.setTimeOut(500);
  delay(100);
  dfPlayer.volume(volume); // Set volume value (0~30).
  dfPlayer.EQ(DFPLAYER_EQ_NORMAL);
  delay(100);
  dfPlayer.outputDevice(DFPLAYER_DEVICE_SD);
  delay(100);
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
    powerDown(hoursToSleep * 60);    
    //powerDown(1);
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
  //Serial.printf("dimFactor: %d\n",dimFactor);
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
  //Serial.printf("remaining s: %d\n",remainingSecs);  
  if(ledCounter == -1) //now calc the overall time for the clock 
  {
     overAllCounterTime = remainingSecs;
     ledFactor = (float)PIXEL_COUNT / (float)overAllCounterTime;
     //Serial.printf("ledFactor: %f overAllCounterTime %d \n",ledFactor, overAllCounterTime);
  }
  //calc ledCounter
  ledCounter = (int)((overAllCounterTime - remainingSecs) * ledFactor);
  ledCounter = ledCounter + 1;
  if(ledCounter > PIXEL_COUNT)
    ledCounter = PIXEL_COUNT;
  //Serial.printf("ledCounter %d\n",ledCounter);
  for (int i = 0; i < ledCounter; i++)
  { // For each pixel...
    leds[i].setRGB(255, 30, 0);    
  }
  FastLED.show();
  delay(20);

  if(remainingSecs <= 0) {
    esp_random();
    delay(100);
    leds_off();
    mode = MODE_FINAL_ANIM;
    animNumber = random(0,3);
    finalAnimStart = millis();
    setCpuFrequencyMhz(80);
    ledCounter = -1;    
  }
}

void anim_confetti() 
{
  Serial.println("render confetti");
  EVERY_N_MILLISECONDS( 2 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, PIXEL_COUNT, 10);
  int pos = random16(PIXEL_COUNT);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
  FastLED.show();
  delay(20);
}

void anim_juggle() {
  Serial.println("render juggle");
  EVERY_N_MILLISECONDS( 2 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, PIXEL_COUNT, 20);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16( i+7, 0, PIXEL_COUNT-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
  FastLED.show();
  delay(20);

}

void anim_rainbow() {
  Serial.println("render rainbow");
  // do some periodic updates
  EVERY_N_MILLISECONDS( 2 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  fill_rainbow( leds, PIXEL_COUNT, gHue, 7);
  FastLED.show();
  delay(5);

}

void renderFinalAnim() {
  SimpleAnimList animations = { anim_rainbow, anim_confetti, anim_juggle };

  Serial.printf("render final Anim nr: %d\n",animNumber);
  animations[animNumber]();
  if(finalAnimStart + END_ANIM_TIME <= millis()) {
    Serial.println("DONE STOP!");
    delay(100);
    leds_off();
    mode = MODE_OFF;    
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
  case MODE_FINAL_ANIM:
    renderFinalAnim();
    break;


  }
  if (dfPlayer.available()) {
    printDetail(dfPlayer.readType(), dfPlayer.read()); //Print the detail message from DFPlayer to handle different errors and states.
  }
}



void printDetail(uint8_t type, int value){
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerUSBInserted:
      Serial.println("USB Inserted!");
      break;
    case DFPlayerUSBRemoved:
      Serial.println("USB Removed!");
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
          case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
  
}
