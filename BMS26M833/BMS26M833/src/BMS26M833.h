/*****************************************************************
File:             BMS26M833.h
Author:           BESTMODULES
Description:      Define classes and required variables
History：         
V1.0.1   -- initial version；2023-05-22；Arduino IDE :v1.8.15
******************************************************************/

#ifndef _BMS26M833_H_
#define _BMS26M833_H_

#include <Wire.h>
#include <Arduino.h>

//BMS26M833 IIC Address
 
//const uint8_t BMS26M833_IICADDR = 0x69;//AD_SELECT to high
const uint8_t BMS26M833_IICADDR = 0x68;//AD_SELECT to low
/*REG_PCTL 0x00*/
#define   NORMAL_MODE         0x00
#define   SLEEP_MODE          0x10
#define   STAND_BY_MODE_60SEC 0x20
#define   STAND_BY_MODE_10SEC 0x21
/*REG_RST 0x01*/
#define   FLAG_RESET          0x30
#define   INITIAL_RESET       0x3f
/*REG_FPSC 0x02*/
#define   FPS_1               0x01
#define   FPS_10              0x00
/*REG_SCLR 0x05*/
#define   OVT_CLR             0x08
#define   OVS_CLR             0x04
#define   INTCLR              0x02
/*REG_AVE 0x07*/
#define   TWICE_MOVE_AVE_OUTPUT 0x20
#define   ONE_MOVE_OUTPUT       0x00
#define ENABLE                1   
#define DISABLE               0


//BMS26M833 Registers
#define    REG_PCTL      0x00 
#define    REG_RST       0x01 
#define    REG_FPSC      0x02 
#define    REG_INTC      0x03 
#define    REG_STAT      0x04 
#define    REG_SCLR      0x05 
//0x06 reserved
#define    REG_AVE       0x07 
#define    REG_INTHL     0x08 
#define    REG_INTHH     0x09 
#define    REG_INTLL     0x0A 
#define    REG_INTLH     0x0B 
#define    REG_IHYSL     0x0C 
#define    REG_IHYSH     0x0D 
#define    REG_TTHL      0x0E 
#define    REG_TTHH      0x0F 
//0x80~0xff 为64组数据
#define    REG_T01L      0x80
#define    REG_T17L      0xA0
#define    REG_T33L      0xC0
#define    REG_T49L      0xE0

class BMS26M833
{
   public:
        float pixels[64];
        
        BMS26M833(uint8_t intPin = 8, TwoWire *theWire = &Wire);
        void begin(uint8_t i2c_addr=BMS26M833_IICADDR);
        void writeReg(uint8_t addr, uint8_t data);
        uint8_t readReg(uint8_t addr);
        void readReg(uint8_t addr, uint8_t rBuf[], uint8_t rLen);       
        void readPixels(float tempBuff[]);
        void readPixelsAndMaximum(float tempBuff[], float &maxVlaue, float &minVlaue);
        //INT0~INT7     0x10~0x17
        void getINTTable(uint8_t buf[], uint8_t size = 8);        
        uint8_t getOperationMode();
        void sleep();
        void reset(uint8_t mode = INITIAL_RESET); 
        uint8_t getFrameMode();
        uint8_t getINT();
        //REG_STAT      0x04 
        uint8_t getStatus();
        //REG_AVE       0x07 
        uint8_t getAverageOutputMode();
        //REG_Thermistor 0x0E 0x0F
        float readThermistorTemp(); 
        
        void beginMeasure(uint8_t mode = NORMAL_MODE);        
        void setFrameMode(uint8_t mode = FPS_10);   
        void setINT(uint8_t isEnable = false);
        // this will automatically set hysteresis to 95% of the high value
        void setInterruptLevels(float high, float low);
        // this will manually set hysteresis
        void setInterruptLevels(float high, float low, float hysteresis);
        void setStatusClear();
        void setAverageOutputMode(uint8_t mode = ONE_MOVE_OUTPUT);
        void setOperationMode(uint8_t mode);

 
        
    private:
        void clearBuf();
        void writeBytes(uint8_t wbuf[], uint8_t wlen);
        void writeRegBit(uint8_t addr,uint8_t bitNum, uint8_t bitValue);
        void readBytes(uint8_t rbuf[], uint8_t rlen);
        uint16_t readRawThermistorTemp();
        uint16_t convertIntToUint16(int16_t val);
        uint16_t convertFloatToSigned12(float val);
        TwoWire *_wire;
        uint8_t _i2caddr;
        uint8_t _intpin;
        
};

#endif
