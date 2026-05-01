#include "Arduino.h"
#include <60ghzfalldetection.h>

//#include <SoftwareSerial.h>
// Choose any two pins that can be used with SoftwareSerial to RX & TX
//#define RX_Pin A2
//#define TX_Pin A3

//SoftwareSerial mySerial = SoftwareSerial(RX_Pin, TX_Pin);

// we'll be using software serial
//FallDetection_60GHz radar = FallDetection_60GHz(&mySerial);

// can also try hardware serial with
FallDetection_60GHz radar = FallDetection_60GHz(&Serial1);

// 記錄上次的狀態
unsigned int lastReport = 0x00;
unsigned long lastHeartbeat = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial1.begin(115200);

  //  mySerial.begin(115200);

  while(!Serial);   //When the serial port is opened, the program starts to execute.

  Serial.println("Ready");
  Serial.println("60GHz Fall Detection - Monitoring Started");
  Serial.println("==========================================");
}

void loop()
{
  // 持續接收雷達數據
  radar.recvRadarBytes();
  radar.Fall_Detection();
  
  // 只在狀態改變時顯示
  if(radar.sensor_report != 0x00 && radar.sensor_report != lastReport){
    
    Serial.println("");
    Serial.print("[");
    Serial.print(millis()/1000);
    Serial.print("s] >> ");
    
    switch(radar.sensor_report){
      case NOONE:
        Serial.println("No one detected");
        break;
        
      case SOMEONE:
        Serial.println("Someone detected");
        break;
        
      case NONEPSE:
        Serial.println("No presence");
        break;
        
      case STATION:
        Serial.println("Person is stationary");
        break;
        
      case MOVE:
        Serial.println("Person is moving");
        break;
        
      case BODYVAL:
        Serial.print("Body movement: ");
        Serial.println(radar.bodysign_val);
        break;
        
      case MOVEDIS:
        Serial.println("Movement distance detected");
        break;
        
      case NOFALL:
        Serial.println("Safe - No fall");
        break;
        
      case FALL:
        Serial.println("*** FALL DETECTED! ***");
        break;
        
      case NORESIDENT:
        Serial.println("No stationary resident");
        break;
        
      case RESIDENCY:
        Serial.println("Stationary resident detected");
        break;
        
      default:
        Serial.print("Unknown: 0x");
        Serial.println(radar.sensor_report, HEX);
        break;
    }
    
    lastReport = radar.sensor_report;
    lastHeartbeat = millis();
  }
  
  // 每30秒顯示一個心跳訊號，確認系統運作中
  if(millis() - lastHeartbeat >= 30000) {
    Serial.print(".");
    lastHeartbeat = millis();
  }
  
  delay(50);
}