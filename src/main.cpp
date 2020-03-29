#include <Arduino.h>
#include <HCSR04.h>
#include "DFRobotDFPlayerMini.h"


HardwareSerial hwSerial(1);
DFRobotDFPlayerMini dfPlayer;
int volume = 20;

UltraSonicDistanceSensor distanceSensor(15, 4);

void setup() {
  btStop();
  hwSerial.begin(9600, SERIAL_8N1, 17, 16);  // speed, type, TX, RX
  // put your setup code here, to run once:
  Serial.begin(115200); 
  Serial.println("init player...");
  while(!dfPlayer.begin(hwSerial)) {
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    delay(1000);
  }
  Serial.println("player online");
  dfPlayer.setTimeOut(500);
  dfPlayer.volume(volume);  // Set volume value (0~30).
  dfPlayer.EQ(DFPLAYER_EQ_NORMAL);
  dfPlayer.outputDevice(DFPLAYER_DEVICE_SD);
  
  
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println(distanceSensor.measureDistanceCm());
  delay(500);

  //dfPlayer.play(1);
}