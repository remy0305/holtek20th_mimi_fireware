/*****************************************************************
File:             BMS26M833.cpp
Author:           BESTMODULES
Description:      IIC communication with the sensor and obtain the corresponding value  
History：         
V1.0.1   -- initial version；2023-05-22；Arduino IDE :v1.8.15
******************************************************************/
#include "BMS26M833.h"

uint8_t dataBuff[128];   //Array for storing data
int DataCnt = 0;
extern float TempMax,TempMin;

/**********************************************************
Description: Constructor
Parameters:  intPin:INT Output pin connection with Arduino 
             wire : Wire object if your board has more than one I2C interface         
Return:      none    
Others:      none
**********************************************************/
BMS26M833::BMS26M833(uint8_t intPin, TwoWire *theWire)
{
   _intpin = intPin;
   _wire = theWire;
}

/**********************************************************
Description: Module Initial
Parameters:  i2c_addr :Module IIC address              
Return:      none    
Others:      none
**********************************************************/
void BMS26M833::begin(uint8_t i2c_addr)
{
     pinMode(_intpin,INPUT);
     _i2caddr = i2c_addr;
     _wire->begin();
      /*------------REG_PCTL 0x00------------------*/
      /*NORMAL_MODE 0x00
      /*SLEEP_MODE 0x10
      /*STAND_BY_MODE_60SEC  (60sec intermittence)  0x20
      /*STAND_BY_MODE_10SEC  (10sec intermittence)  0x21  */                                 
      beginMeasure();//enter Normal mode(default) 
           
      /*--------------REG_RST 0x01-----------------*/
      /*FLAG_RESET     0x30
      /*INITIAL_RESET  0x3f                        */                   
      reset(); //Initial reset(default)
      delay(1000);
      /*-------------REG_FPSC 0x02-----------------*/
      /*FPS_1     0x01   //set to 1  FPS
      /*FPS_10    0x00   //set to 10 FPS           */  
      setFrameMode(FPS_10);//set to 10 FPS
      
      /*-------------REG_INTC 0x03-----------------*/
      /*ABS_VALUE_INT     Absolute Value Interrupt Mode     
       DIFFERENCE_INT     Difference Interrupt Mode
       DISABLE            Input false 
       ENABLE             Input  true               */
      setINT(false);//Disable Interrupt  

}
/**********************************************************
Description: write Register data
Parameters:  addr :Register to be written
             data:Value to be written
Return:      none    
Others:      none
**********************************************************/
void BMS26M833::writeReg(uint8_t addr, uint8_t data)
{
      uint8_t sendBuf[2]={addr,data};
      writeBytes(sendBuf,2);
      delay(1);
}
/**********************************************************
Description: read Register data
Parameters:  addr :Register to be written    
Return:      8-bit data of Register
Others:      user can use this function to read any register  
             including something are not mentioned.
**********************************************************/
uint8_t BMS26M833::readReg(uint8_t addr)
{
      clearBuf();
      uint8_t sendBuf[1] = {addr};
      writeBytes(sendBuf,1);
      delay(1);
      readBytes(dataBuff,1);
      delay(1);
      return dataBuff[0];     
}
/**********************************************************
Description: read Register to get Data
Parameters:  addr:Register to be written
             rBuf:Variables for storing Data to be obtained
             rLen:the byte of the data       
Return:      none    
Others:      none
**********************************************************/
void BMS26M833::readReg(uint8_t addr, uint8_t rBuf[], uint8_t rLen)
{
    clearBuf();
    uint8_t sendBuf[1] = {addr};
    writeBytes(sendBuf,1);
    delay(1);
    readBytes(rBuf,rLen);
    delay(1);
}
/**********************************************************
Description: read temperature Pixels(unit:℃)
Parameters:  tempBuff[]:Store temperature data from the sensor   
Return:      none    
Others:      none
**********************************************************/
void BMS26M833::readPixels(float tempBuff[])
{
      int cnt=0;
      uint8_t buf1[32]={0},buf2[32]={0},buf3[32]={0},buf4[32]={0};
      readReg(REG_T01L,buf1, 32);
      readReg(REG_T17L,buf2, 32);
      readReg(REG_T33L,buf3, 32);
      readReg(REG_T49L,buf4, 32);
      for(int i =0;i<32;i++)
      {
          dataBuff[cnt] = buf1[i];
          dataBuff[cnt+32] = buf2[i];
          dataBuff[cnt+64] = buf3[i];
          dataBuff[cnt+96] = buf4[i];
          cnt++; 
      }
      cnt = 0;
      for(int pixels_cnt = 0; pixels_cnt < 64; pixels_cnt++)
      {
          tempBuff[pixels_cnt] = (uint16_t)dataBuff[cnt+1] <<8  | dataBuff[cnt];
          tempBuff[pixels_cnt] *= 0.25;
          cnt += 2;
      } 
}
/**********************************************************
Description: read temperature Pixels and Maximum value(unit:℃)
Parameters:  tempBuff[]:Store temperature data from the sensor 
             maxValue:Store temperature max data
             minValue:Store temperature min data
Return:      none    
Others:      none
**********************************************************/
void BMS26M833::readPixelsAndMaximum(float tempBuff[], float &maxValue, float &minValue)
{
      int cnt=0;
      uint8_t buf1[32]={0},buf2[32]={0},buf3[32]={0},buf4[32]={0};
      maxValue = 0;
      minValue = 80;
      readReg(REG_T01L,buf1, 32);
      readReg(REG_T17L,buf2, 32);
      readReg(REG_T33L,buf3, 32);
      readReg(REG_T49L,buf4, 32);

      for(int i =0;i<32;i++)
      {
          dataBuff[cnt] = buf1[i];
          dataBuff[cnt+32] = buf2[i];
          dataBuff[cnt+64] = buf3[i];
          dataBuff[cnt+96] = buf4[i];
          cnt++; 
      }
      cnt = 0;
      for(int pixels_cnt = 0; pixels_cnt < 64; pixels_cnt++)
      {
          tempBuff[pixels_cnt] = (uint16_t)dataBuff[cnt+1] <<8  | dataBuff[cnt];
          tempBuff[pixels_cnt] *= 0.25;
          cnt += 2;
          if(tempBuff[pixels_cnt] > maxValue) maxValue = tempBuff[pixels_cnt];
          if(tempBuff[pixels_cnt] < minValue) minValue = tempBuff[pixels_cnt];
      }
}
/**********************************************************
Description: get Interrupt Table
Parameters:  buf[]: the returned data will be stored
             size: size Optional number of bytes to read. Default is 8 bytes.
Return:       
Others:      register:0x10~0x17
**********************************************************/
void BMS26M833::getINTTable(uint8_t buf[], uint8_t size)
{
      readReg(0x10,dataBuff, size);
      for(int i=0;i<size;i++)
      {
          buf[i] = dataBuff[i]; 
      }
}
/**********************************************************
Description: get Operation Mode of device
Parameters:  none
Return:      Operation Mode(1 byte)  
Others:      register:0x00
**********************************************************/
uint8_t BMS26M833::getOperationMode()
{
      uint8_t mode = 0;
      readReg(REG_PCTL);
      mode = dataBuff[0];
      return mode;
}
/**********************************************************
Description: Request device enter to sleep mode
Parameters:  none
Return:      none
Others:   
**********************************************************/
void BMS26M833::sleep()
{
      writeReg(REG_PCTL, SLEEP_MODE);
}
/**********************************************************
Description: Reset Mode of device
Parameters:  mode:Option:
              FLAG_RESET         
              INITIAL_RESET(default)          
Return:      none    
Others:      register:0x01
**********************************************************/
void BMS26M833::reset(uint8_t mode)
{
      writeReg(REG_RST, mode);
}
/**********************************************************
Description: get Frame Mode of device
Parameters:  none       
Return:      Reset Mode(1 byte)   
Others:      register:0x02
**********************************************************/
uint8_t BMS26M833::getFrameMode()
{
      uint8_t mode = 0;
      readReg(REG_FPSC);
      mode = dataBuff[0]; 
      return mode;       
}
/**********************************************************
Description: get INT PIN Status
Parameters:  
Return:      INT PIN Status(1 bit)   
Others:      1:INT status is HIGH(default)
             0:INT status is LOW(get a interrupt)
**********************************************************/
uint8_t BMS26M833::getINT()
{
      uint8_t statusValue = 0;
      statusValue = digitalRead(_intpin);
      return statusValue;
}
/**********************************************************
Description: get Status Register Value
Parameters:  none
Return:      Status Register Value(1 byte)   
Others:      register:0x04
**********************************************************/
uint8_t BMS26M833::getStatus()
{
      uint8_t statusValue = 0;
      readReg(REG_STAT);
      statusValue = dataBuff[0]; 
      return statusValue;
}
/**********************************************************
Description: get Average Output Mode
Parameters:  none
Return:      Output Mode  
Others:      register:0x07
**********************************************************/
uint8_t BMS26M833::getAverageOutputMode()
{
      uint8_t mode = 0;
      readReg(REG_AVE);
      mode = dataBuff[0];
      return mode;
}
/**********************************************************
Description: read Thermistor Temperature
Parameters:  none
Return:      Thermistor Temperature(unit:℃)    
Others:      register:0x0e 0x0f
**********************************************************/
float BMS26M833::readThermistorTemp()
{
      float tempValue = 0;
      int16_t temp = 0;
      readReg(REG_TTHL,dataBuff, 2);
      temp = ((uint16_t)dataBuff[1] <<8 | dataBuff[0]) << 4;
      temp = temp>>4;
      tempValue = temp * 0.0625;
      return tempValue;
      
}

/**********************************************************
Description: begin Measuring of device
Parameters:   mode:Option:
                NORMAL_MODE(default)                 
                STAND_BY_MODE_60SEC 
                STAND_BY_MODE_10SEC 
Return:      none    
Others:      register:0x00
**********************************************************/
void BMS26M833::beginMeasure(uint8_t mode)
{
      writeReg(REG_PCTL, mode);
}
/**********************************************************
Description: set Frame Mode of device
Parameters:  mode:
              FPS_1         
              FPS_10  (default)         
Return:      none    
Others:      register:0x02
**********************************************************/
void BMS26M833::setFrameMode(uint8_t mode)
{
      writeReg(REG_FPSC, mode);
}
/**********************************************************
Description: set INT Mode
Parameters:  isEnable:true or false     
Return:      none    
Others:      register:0x03
**********************************************************/
void BMS26M833::setINT(uint8_t isEnable)
{
     if(isEnable == true)
     {
          writeReg(REG_INTC, 0xff);
     }
     if(isEnable == false)
     {
          writeReg(REG_INTC, 0x00);
     }
}
/**********************************************************
Description: Set the interrupt levels
Parameters:  high: the value above which an interrupt will be triggered
             low: the value below which an interrupt will be triggered
Return:        
Others:      The hysteresis value defaults to 0.95*high
**********************************************************/
void BMS26M833::setInterruptLevels(float high, float low)
{
      setInterruptLevels(high, low, high*.95);
}
/**********************************************************
Description: Set the interrupt levels
Parameters:  high: the value above which an interrupt will be triggered
             low: the value below which an interrupt will be triggered
             hysteresis: the hysteresis value for interrupt detection
Return:        
Others:      All parameters are in degrees C
**********************************************************/
void BMS26M833::setInterruptLevels(float high, float low, float hysteresis)
{
      uint16_t highTemp = convertFloatToSigned12(high * 4);
      writeReg(REG_INTHL, highTemp & 0xFF);
      writeReg(REG_INTHH, highTemp >> 8);
      uint16_t lowTemp = convertFloatToSigned12(low * 4);
      writeReg(REG_INTLL, lowTemp & 0xFF);
      writeReg(REG_INTLH, lowTemp >> 8);
      uint16_t hysteresisTemp = convertFloatToSigned12(hysteresis * 4);
      writeReg(REG_IHYSL, hysteresisTemp & 0xFF);
      writeReg(REG_IHYSH, hysteresisTemp >> 8);
}

/**********************************************************
Description: set Status Clear
Parameters:  none
Return:      none  
Others:      none
**********************************************************/
void BMS26M833::setStatusClear()
{
      reset(FLAG_RESET);
}

/**********************************************************
Description: set Average Output Mode
Parameters:  mode:set data Average Output Mode,Option:
              TWICE_MOVE_AVE_OUTPUT
              ONE_MOVE_OUTPUT
Return:      none    
Others:      register:0x07
**********************************************************/
void BMS26M833::setAverageOutputMode(uint8_t mode)
{

      writeReg(0x1f, 0x50);
      writeReg(0x1f, 0x45);
      writeReg(0x1f, 0x57);
      writeReg(REG_AVE, mode);
      writeReg(0x1f, 0x00);
}
/**********************************************************
Description: set Operation Mode of device
Input:       mode:Option:
              NORMAL_MODE         
              SLEEP_MODE          
              STAND_BY_MODE_60SEC 
              STAND_BY_MODE_10SEC 
Output:      none  
Return:      none    
Others:      register:0x00
**********************************************************/
void BMS26M833::setOperationMode(uint8_t mode)
{
       writeReg(REG_PCTL, mode);
}
/**********************************************************The following are private functions**********************************************************/
/**********************************************************
Description: read Thermistor Temperature raw data
Parameters:  none
Return:      Thermistor Temperature raw data(2 byte)  
Others:      register:0x0e 0x0f
**********************************************************/
uint16_t BMS26M833::readRawThermistorTemp()
{
      uint16_t rawTempValue = 0;
      readReg(REG_TTHL,dataBuff, 2);
      rawTempValue = ((uint16_t)dataBuff[1] <<8 | dataBuff[0]);
      return rawTempValue;
}

/**********************************************************
Description: convert int16 To Uint16
Parameters:  val:value(int16_t) to be converted
Return:      uint16_t data 
Others:      Avoid any ambiguity when casting int16_t to uint16_t
**********************************************************/ 
uint16_t BMS26M833::convertIntToUint16(int16_t val)
{
  union
  {
    int16_t signed16;
    uint16_t unsigned16;
  } signedUnsigned16;

  signedUnsigned16.signed16 = val;
  return signedUnsigned16.unsigned16;
}

/**********************************************************
Description: convert Float To Signed12
Parameters:  val:value(float) to be converted
Return:      Signed12 data
Others:      
**********************************************************/
uint16_t BMS26M833::convertFloatToSigned12(float val)
{
  int16_t signedVal = round(val);
  uint16_t unsignedVal = convertIntToUint16(signedVal); // Convert without ambiguity

  if (unsignedVal & (1 << 15)) // If the two's complement value is negative
    return ((unsignedVal & 0x0FFF) | (1 << 11)); // Limit to 12-bits
  else
    return(unsignedVal & 0x07FF);
}
/**********************************************************
Description: clear Buf
Parameters:  none
Return:      none    
Others:      none  
**********************************************************/
void BMS26M833::clearBuf()
{
      for(int a = 0; a < 128; a++)
      {
        dataBuff[a] = 0;
      } 
}
/**********************************************************
Description: writeBytes
Parameters:  wbuf[]:Variables for storing Data to be sent
             wlen:Length of data sent  
Return:   
Others:
**********************************************************/
void BMS26M833::writeBytes(uint8_t wbuf[], uint8_t wlen)
{
    while(_wire->available() > 0)
    {
      _wire->read();
    }
    _wire->beginTransmission(_i2caddr); //IIC start with 7bit addr
    _wire->write(wbuf, wlen);
    _wire->endTransmission();
}
/**********************************************************
Description: readBytes
Parameters:  rbuf[]:Variables for storing Data to be obtained
             rlen:Length of data to be obtained
Return:   
Others:
**********************************************************/
void BMS26M833::readBytes(uint8_t rbuf[], uint8_t rlen)
{
    _wire->requestFrom(_i2caddr, rlen);
    if(_wire->available()==rlen)
    {
      for(uint8_t i = 0; i < rlen; i++)
      {
        rbuf[i] = _wire->read();
      }
    }
}
/**********************************************************
Description: write a bit data
Parameters:  bitNum :Number of bits(bit7-bit0)
             bitValue :Value written 
Return:      none    
Others:      none
**********************************************************/
void BMS26M833::writeRegBit(uint8_t addr,uint8_t bitNum, uint8_t bitValue)
{
      uint8_t data;
      readReg(addr,dataBuff,1);
      data = dataBuff[0];
      data = (bitValue != 0)? (data|(1 <<bitNum)) : (data & ~(1 <<bitNum));
      writeReg(addr, data);
}




  
