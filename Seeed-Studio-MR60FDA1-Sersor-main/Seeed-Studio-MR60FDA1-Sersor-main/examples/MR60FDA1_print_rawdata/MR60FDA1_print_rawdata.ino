#include "Arduino.h"
#include <60ghzfalldetection.h>
#include <stdio.h>

// 使用 UART4
FallDetection_60GHz radar = FallDetection_60GHz(&Serial4);

void setup() {
  Serial.begin(115200);
  Serial4.begin(115200);

  while(!Serial);

  Serial.println("Ready");
}

void loop() {
  radar.recvRadarBytes();  

  String radarData = radar.getDataAsString();

  if (radarData.length() > 0) {
    Serial.print(radarData);
  }

  delay(200);
}
