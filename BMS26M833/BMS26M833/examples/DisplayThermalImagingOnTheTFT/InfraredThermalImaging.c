/*****************************************************************
File:             InfraredThermalImaging.c
Author:           Weng, BESTMODULE
Description:      This document provides Amg8833 Correlation algorithm for infrared thermal imaging of series sensors.
History：         
V1.0.1   -- initial version；2021-09-01；Arduino IDE :v1.8.15
******************************************************************/
#include "InfraredThermalImaging.h"

#define MaxGrayscale 256
extern float TempMax,TempMin;

 /**********************************************************
Description: Convert temperature matrix to thermal imaging.
Input:       *Config: Configuration structure.        
Output:      none 
Return:      none    
Others:      none
**********************************************************/
uint8_t InfraredThermalImaging(ThermalImagingConfig *Config)
{
  uint8_t Threshold;
  uint8_t TempGrayMat[Config -> InWidth * Config -> InHeight];
  float sub = 0;
  
  TransformGray(Config -> InMat,TempGrayMat,Config -> InWidth,Config -> InHeight,TempMax,TempMin);
  sub= TempMax - TempMin;
  if(sub <= Config -> TempDiff) return 0;
  Threshold = Otus(TempGrayMat,Config -> InWidth,Config -> InHeight);
  BackgroundFiltering(TempGrayMat,Config -> InWidth,Config -> InHeight,Threshold,Config -> Background);
  Bilinear(TempGrayMat,Config -> OutMat,Config -> InWidth,Config -> InHeight,Config -> OutWidth,Config -> OutHeight);
  return 0;
}
/**********************************************************
Description: Bilinear interpolation of gray image.
Input:       *InMat: Pointer to the first address of the original matrix (value range 0 ~ 255).
             *OutMat: Pointer to the first address of the output matrix (value range 0 ~ 255).      
             InWidth: Original image width.
             InHeight: Original image height.
             OutWidth: Output image width.
             Outeight: Output image height.
Output:      none 
Return:      none    
Others:      none
**********************************************************/
uint8_t Bilinear(uint8_t *InMat,uint8_t *OutMat,uint8_t InWidth,uint8_t InHeight,uint8_t OutWidth,uint8_t OutHeight)
{
  unsigned short row ,col;
  float fy,fx,RowFactor,ColFactor;
  unsigned short sy ,sx;
  unsigned short xbuf[2],ybuf[2];

  RowFactor = (float)(InHeight - 1) / OutHeight;
  ColFactor = (float)(InWidth  - 1) / OutWidth;
  for(row = 0;row < OutHeight;row++)
  {
    //Coordinate mapping
    fy = (row + (float)0.5) * RowFactor;

    sy = fy;
    fy -= sy;
    if(fy < 0) fy = 0; 
    
    //Enlarge the floating-point number to make integer operation become floating-point operation
    ybuf[0] = ((float)1.0 - fy) * 2048;
    ybuf[1] = 2048 - ybuf[0];
    
    for(col = 0;col <OutWidth;col++)
    {
      //Coordinate mapping
      fx = (col + (float)0.5) * ColFactor;
      
      sx  = fx;
      fx -= sx;
      if(fx < 0) fx = 0;
      
      //Enlarge the floating-point number to make integer operation become floating-point operation
      xbuf[0] = ((float)1.0 - fx) * 2048;
      xbuf[1] = 2048 - xbuf[0];
      OutMat[row * OutWidth + col] = (InMat[ sy      * InWidth + sx]     * ybuf[0] * xbuf[0] +
                                      InMat[(sy + 1) * InWidth + sx]     * ybuf[1] * xbuf[0] +
                                      InMat[ sy      * InWidth + (sx+1)] * ybuf[0] * xbuf[1] +
                                      InMat[(sy + 1) * InWidth + (sx+1)] * ybuf[1] * xbuf[1] ) >> 22; 
    }
  }
  return 0;
}
/**********************************************************
Description: Convert temperature matrix to gray matrix.
Input:       *InMat: Pointer to the first address of the original matrix (value range 0 ~ 255).
             *OutMat: Pointer to the first address of the output matrix (value range 0 ~ 255).      
             InWidth: Original image width.
             InHeight: Original image height.
             Max: Address where the maximum temperature is stored.
             Min: Address for storing the minimum temperature.
Output:      none 
Return:      none    
Others:      none
**********************************************************/
uint8_t TransformGray(float *InMat,uint8_t *OutMat,uint8_t InWidth,uint8_t InHeight,float Max,float Min)
{
  float SubValue;
  uint16_t Num,temp;

  //First normalize and then multiply 255 to gray value
  SubValue = Max - Min;
  if(SubValue == 0) SubValue = 1;
  temp = InWidth * InHeight;
  for(Num = 0; Num < temp; Num++)
  {
    OutMat[Num] = ((InMat[Num] - Min) * 255) / SubValue;
  }

  return 0;
}
/**********************************************************
Description: Background Filtering 
Input:       *InMat: Pointer to the first address of the original matrix (value range 0 ~ 255).  
             InWidth: Original image width.
             InHeight: Original image height.
             Threshold: Threshold for segmenting background and target (value range 0 ~ 255).
             Background: Background removal degreeThe empirical value is 20(value range 0 ~ 255).
Output:      none 
Return:      none    
Others:      none
**********************************************************/
uint8_t BackgroundFiltering(uint8_t *InMat,uint8_t InWidth,uint8_t InHeight,uint8_t Threshold,uint8_t Background)
{
  uint16_t row,col,temp;

  if(Background != 0)
  { 
    for(row = 0;row < InWidth;row++)
    {
      for(col = 0;col < InHeight;col++)
      {   
        if(InMat[row * InWidth + col] < Threshold)
        { 
          if(row != 0)
          {
            temp = (row - 1) * InWidth + col;
            if((InMat[temp] >= Threshold) && (InMat[temp] - InMat[(row) * InWidth + col] >= Background))   continue;
          }
          if(row != InHeight - 1)
          {
            temp = (row + 1) * InWidth + col;
            if((InMat[temp] >= Threshold) && (InMat[temp] - InMat[(row) * InWidth + col] >= Background))   continue;
          }
          if(col != 0)
          {
            temp = row * InWidth + col - 1 ;
            if((InMat[temp] >= Threshold) && (InMat[temp] - InMat[temp+1] >= Background))     continue;
          }
          if(col != InWidth-1)
          {
            temp = row * InWidth + col + 1;
            if((InMat[temp] >= Threshold) && (InMat[temp] - InMat[temp-1] >= Background))     continue;
          }
          InMat[row * InWidth + col] = 0;
        }
      }
    } 
  }
  return 0;
}
/**********************************************************
Description: Otus algorithm (Also known as maximum interclass variance method).Otus 
Input:       *InMat: Pointer to the first address of the original matrix (value range 0 ~ 255).   
             InWidth: Original image width.
             InHeight: Original image height.
Output:      none 
Return:      Segmentation threshold between background and target   
Others:      none
**********************************************************/
uint8_t Otus(uint8_t *InMat,uint8_t InWidth,uint8_t InHeight)
{ 
   float Value,SumValue1,SumValue2,temp;
   float GrayProbabilitySum[64]        = {0};
   float GrayMeanSum[64]               = {0}; 
   uint8_t GrayNum[MaxGrayscale] = {0};
   uint8_t OrderMat[64]          = {0};
   unsigned int TotalPixels;
   unsigned int row,col,Num,Threshold,value,ComparTh;
  
  //Statistical gray histogram
  for(row = 0;row < InHeight;row++)
  {
    for(col = 0;col < InWidth;col++)
    {
      GrayNum[InMat[row * InWidth + col]]++;
    }
  }
  
  SumValue1 = 0;
  SumValue2 = 0;
  TotalPixels = InWidth * InHeight;
  for(Num = 0,row = 0;Num < MaxGrayscale && row < 64;Num++)
  {
    if(GrayNum[Num] == 0) continue;
    
    Value = (float)GrayNum[Num] / TotalPixels;
    OrderMat[row] = Num;
    
    //Cumulative sum of probability
    GrayProbabilitySum[row] = SumValue1 + Value;
    SumValue1 = GrayProbabilitySum[row];
    
    //Mean of gray value
    GrayMeanSum[row] = SumValue2 + Num * Value;
    SumValue2 = GrayMeanSum[row];
    row++;
  }
  
  //The maximum interclass variance is calculated
  Threshold = 0;
  value = 0;
  for(Num = 0;Num < row;Num++)
  {
    temp = GrayMeanSum[row-1] * GrayProbabilitySum[Num] - GrayMeanSum[Num];
    ComparTh =  temp *temp / (GrayProbabilitySum[Num] * (1.0001 - GrayProbabilitySum[Num]));
    if(ComparTh > value) 
    {
      Threshold = OrderMat[Num];
      value = ComparTh;
    }
  }
  return (uint8_t)Threshold;
}
