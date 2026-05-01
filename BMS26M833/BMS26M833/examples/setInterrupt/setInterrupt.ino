/*****************************************************************
File:         setInterrupt.ino
Description:  This sketch shows how to use the interrupt on the sensor.
              when received the interrupt,The information will be printed 
              out from the serial port .
******************************************************************/
#include <BMS26M833.h>

//#define INTPIN 8
//interrupt levels (in degrees C,Resolution is 0.25℃)
//any pixel above TEMP_INT_HIGH degrees C, or under TEMP_INT_LOW degrees C will trigger an interrupt
//30 degrees should trigger when you wave your hand in front of the sensor
#define TEMP_INT_HIGH 30
#define TEMP_INT_LOW 15

BMS26M833 Amg;
uint8_t interruptTable[8];
/*****
we can tell which pixels triggered the interrupt by reading the
bits in this array of bytes. Any bit that is a 1 means that pixel triggered

         bit 7  bit 6  bit 5  bit 4  bit 3  bit 2  bit 1  bit 0
byte 0 |  0      0      0      0      0      0      0      1
byte 1 |  0      0      0      0      0      0      0      0
byte 2 |  0      0      0      0      0      0      1      0
byte 3 |  0      0      0      0      0      0      0      0
byte 4 |  0      0      0      0      0      0      1      0
byte 5 |  0      0      0      0      0      0      0      0
byte 6 |  0      0      0      0      0      0      0      0
byte 7 |  0      0      1      0      0      0      0      0

*****/
void setup() 
{
    Serial.begin(9600);
    Amg.begin();
    Amg.setInterruptLevels(TEMP_INT_HIGH, TEMP_INT_LOW);
    Amg.setINT(true);
}

void loop() 
{
    if(Amg.getINT() == 0)//get a interrupt
    {
        Amg.getINTTable(interruptTable);
        Serial.println("=======Get the interrupt!=======");
        for(int i=0;i<8;i++)
        {
            for(int j=7;j>=0;j--)
            {
                bool intBit = interruptTable[i] & (1<<j);
                Serial.print(intBit);
                Serial.print(" ");
            }
            Serial.println();
        }
        Amg.setStatusClear();//clear the interrupt so we can get the next one! 
    }

}
