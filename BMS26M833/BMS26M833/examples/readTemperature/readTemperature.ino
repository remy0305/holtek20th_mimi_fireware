/*****************************************************************
File:         readTemperaturePixels.ino
Description:  repeatedly obtain the Thermistor Temperature(unit: ℃),
              and 8*8 temperature pixel matrix(unit: ℃) through IIC. 
              Then display the value in the serial port. 
******************************************************************/
#include <BMS26M833.h>
BMS26M833 Amg;
float temp;
float TempMat[64]; 
void setup() 
{
    Amg.begin();
    Serial.begin(9600);
    Serial.println("======== BMS26M833 8*8 pixels Text ========");
}

void loop() 
{
    Amg.readPixels(TempMat);
    temp = Amg.readThermistorTemp(); 
    Serial.println("\t\t====== Unit:℃ ======");
    Serial.print("Thermistor Temp= ");
    Serial.println(temp);
    for(int x = 1; x <= 64 ; x++)   //Output 8*8 temperature pixel matrix
    {
        Serial.print(TempMat[x-1], 1);
        Serial.print(",  ");
        if(x%8 == 0)  Serial.println();     
    }
  
    
    Serial.println();

    delay(500);
}
