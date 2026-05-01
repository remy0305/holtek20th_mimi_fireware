
/*****************************************************************
File:             DisplayThermalImagingInTheTFT.ino
Author:           BESTMODULE
Description:      The temperature value of the sensor is obtained through IIC, 
                  and the 8 * 8 temperature pixel matrix is converted into a more 
                  appropriate pixel value in combination with the algorithm to facilitate 
                  the observation of imaging phenomena
History：         
V1.0.2   -- initial version；2021-10-09；Arduino IDE :v1.8.15
******************************************************************/
#include <SPI.h>            // SPI library
#include <BMD58T280.h>
#include "lcdfont.h"
#include <BMS26M833.h>
extern "C" {
  #include "InfraredThermalImaging.h" 
}

// Choose communication interface
#if !defined(ARDUINO_AVR_UNO) // For BMduino
BMD58T280 TFTscreen;          // Create LCD objects. Please comment out this line of code if you don't use EBI mode
//BMD58T280 TFTscreen(&SPI);    // Create LCD objects. Please comment out this line of code if you don't use SPI mode
#else // For UNO
BMD58T280 TFTscreen(&SPI);    // Create LCD objects
#endif

/*Macro definition, convenient debugging------------------------------------------------------------------*/
#define OutMatWidth    55                   //Output image width
#define OutMatHeight   55                   //Output image height
#define Magnification   3                   //Output image Magnification 
#define TempDiffConfig  4                   //When the temperature difference is less than this value, 
                                            //it is considered as invalid thermal imaging and will not be displayed,(unit:℃)
#define BackgroundConfig 20                 //Background removal degree,The larger the value, the smaller the transition layer
                                            // between the target and the background. The empirical value is 20 and the range is 0-255
#define ColorBarConfig  Rainbow2_65K        //Display style,  Optional:
                                            //PseudoColor1_65K/PseudoColor2_65K/MetalColor1_65K/MetalColor2_65K/Rainbow1_65K/Rainbow2_65K
/* Global variables ---------------------------------------------------------------------------------------*/
ThermalImagingConfig ThermImaConfig;
BMS26M833 amg(22,&Wire1);//22:STATUS1

float TempMat[8 * 8];                         //Store temperature data from the sensor
uint8_t  DataBuf[OutMatWidth * OutMatHeight]; //Store temperature data After algorithm processing
uint16_t timecnt=0;
float TempMax,TempMin;                        //temperature maximum data from the sensor
char AxisPrintout[20];   // char array to print to the screen
uint16_t maging_xStart,maging_yStart;
uint32_t Systime = 0;

void setup() {
  Serial.begin(9600);// initialize the serial port
  TFTscreen.begin();// initialize the display
  TFTscreen.setRotation(1);

  amg.begin();          //sensor initialization
  LCDShowColorbar();    //display Initial picture
  maging_xStart = 100-Magnification*25;
  maging_yStart = 130-Magnification*25;
  ThermImaConfig.InWidth    = 8;
  ThermImaConfig.InHeight   = 8;  
  ThermImaConfig.InMat      = TempMat;
  ThermImaConfig.OutMat     = DataBuf;
  ThermImaConfig.OutWidth   = OutMatWidth;
  ThermImaConfig.OutHeight  = OutMatHeight;
  ThermImaConfig.Background = BackgroundConfig;
  ThermImaConfig.TempDiff   = TempDiffConfig;
}

void loop() {
  amg.readPixelsAndMaximum(TempMat, TempMax, TempMin);//Obtain temperature and maximum value

  if(TempMin>0 && TempMax<80)
  {
    InfraredThermalImaging(&ThermImaConfig);         //algorithm processing
    LCDShow(DataBuf,TempMax,TempMin,OutMatWidth,OutMatHeight,TempDiffConfig,Magnification,maging_xStart,maging_yStart); //display
  }
  else
  {
    LCDShow(DataBuf,0,0,OutMatWidth,OutMatHeight,TempDiffConfig,Magnification,maging_xStart,maging_yStart);    
    delay(30);
  }
}

/**********************************************************
Description: The thermal imaging grayscale image is toned and displayed on the screen.
Input:     *Buff: Pointer to the first address of the output matrix (value range 0 ~ 255).
           Max: Maximum value of original temperature matrix.
           Min: Minimum value of original temperature matrix.
           Width: Thermal imaging width.
           Height: Thermal image height.
           TempDiff: Displays the lowest order of the palette when the maximum and minimum values of the temperature matrix are less than or equal to this value.
           Mul: Display magnification.
           xStart: Displays the x value in the upper left corner of the image.
           yStart: Displays the y value in the upper left corner of the image.
Output:    none
Return:    none    
Others:    none
**********************************************************/
void LCDShow(uint8_t *Buff,float Max,float Min,uint8_t Width,uint8_t Height,uint8_t TempDiff,uint8_t Mul,uint16_t xStart,uint16_t yStart)
{ 
  uint16_t temp; 
  uint16_t color;
  uint16_t i,j,k,l,xLeng,yLeng;
  xLeng = Mul * Width;
  yLeng = Mul * Height;

  LCD_ShowFloatNum1(210,110,Max,3,BM_ILI9341::WHITE,BM_ILI9341::BLACK,3);
  LCD_ShowFloatNum1(210,190,Min,3,BM_ILI9341::WHITE,BM_ILI9341::BLACK,3);
 // LCD_ShowFloatNum1(208,115,Max,3,BM_ILI9341::WHITE,BM_ILI9341::BLUE,3);
  
  if((Max-Min) <= TempDiff)
  {
    color=ColorBarConfig[0];
    TFTscreen.fill(color);
    TFTscreen.noStroke();
    TFTscreen.rect(xStart,yStart,xLeng,yLeng);
    return;
  } 
  TFTscreen.setAddrWindow(xStart,yStart,xLeng,yLeng);
  for(i = 0;i < Height;i++)
  {
    for(k = 0;k < Mul;k++)
    {
      for(j = 0;j < Width;j++)
      {   
        temp = i * Width + j;
        color=ColorBarConfig[Buff[temp]]; 
        TFTscreen.writeColor(color,Mul);
      }
    }
  } 
}
/******************************************************************************
Description: Displays a two-digit decimal variable
Input:      x,y： displays coordinates
            num ：  num to display decimal variables
            len :   The number of bits to display
            fc: The color of the fc word
            bc: The background color of the bc word
            sizey: The sizey font size 
Output:      none 
Return:      none    
Others:      none
******************************************************************************/
void LCD_ShowFloatNum1(uint16_t x,uint16_t y,float num,uint8_t len,u32 fc,u32 bc,uint8_t sizey)
{
  String strResult = String(num);
  strResult.toCharArray(AxisPrintout, 5);
  TFTscreen.fill(bc);
  TFTscreen.noStroke();
  TFTscreen.rect(x-1, y-2, 70, 26);//background color
  TFTscreen.stroke(fc);//word color
  TFTscreen.setTextSize(sizey);
  TFTscreen.text(AxisPrintout, x, y);//display variable
}

/**********************************************************
Description: display Initial picture
Input:     none
Output:    none
Return:    none    
Others:    none
**********************************************************/
void LCDShowColorbar()
{
  TFTscreen.background(BM_ILI9341::BLACK);
  
 /* TFTscreen.fill(BM_ILI9341::RED);
  TFTscreen.noStroke();
  TFTscreen.rect(0, 210, 320, 30);*/

  TFTscreen.stroke(BM_ILI9341::WHITE);
  TFTscreen.setTextSize(3);
  TFTscreen.text("MAX:", 205, 70);
  TFTscreen.text("MIN:", 205, 150);
//TFTscreen.text("Core:", 210, 80);
  
/*  TFTscreen.fill(BM_ILI9341::BLUE);
  TFTscreen.noStroke();
  TFTscreen.rect(207, 113, 71, 26);//core value background*/
  
  TFTscreen.stroke(BM_ILI9341::WHITE);
  TFTscreen.setTextSize(3);
  TFTscreen.text("BMS26M833", 25, 15);
  
  TFTscreen.drawImage(288,105,gImage_tempLogo,32,32,0XFFFF);//Display ℃
  TFTscreen.drawImage(288,185,gImage_tempLogo,32,32,0XFFFF);//Display ℃
  TFTscreen.drawImage(200,0,gImage_BMLogo,104,44,0XFFFF);//Display BMLOGO
  uint8_t a=0;
  uint8_t i;
  a=0;
  for(i = 245;i > 0;i--)
  {
    TFTscreen.stroke(ColorBarConfig[i]);
    TFTscreen.line(0,a,18,a);
    a++;
  }
  TFTscreen.noStroke();
}
