/*****************************************************************
File:             InfraredThermalImaging.h
Author:           Weng, BESTMODULE
Description:      This document provides Amg8833 Correlation algorithm for infrared thermal imaging of series sensors.
History：         
V1.0.1   -- initial version；2021-09-01；Arduino IDE :v1.8.15
******************************************************************/
#ifndef _INFRAREDTHERMALIMAGING_H_
#define _INFRAREDTHERMALIMAGING_H_
#include <Arduino.h>

typedef struct 
{
	float *InMat;
	uint8_t *OutMat;
	uint8_t InWidth;
	uint8_t InHeight;
	uint8_t OutWidth;
	uint8_t OutHeight;
	uint8_t  Background;	
	uint8_t TempDiff;
}ThermalImagingConfig;



/* Exported functions --------------------------------------------------------------------------------------*/
uint8_t InfraredThermalImaging(ThermalImagingConfig *Config);
uint8_t Bilinear(uint8_t *InMat,uint8_t *OutMat,uint8_t InWidth,uint8_t InHeight,uint8_t OutWidth,uint8_t OutHeight);
uint8_t TransformGray(float *InMat,uint8_t *OutMat,uint8_t InWidth,uint8_t InHeight,float Max,float Min);
uint8_t  Otus(uint8_t *InMat,uint8_t InWidth,uint8_t InHeight);
uint8_t BackgroundFiltering(uint8_t *InMat,uint8_t InWidth,uint8_t InHeight,uint8_t Threshold,uint8_t Background);

#endif 
