/*******************************************************************************
* ad.h
*
* Copyright 2016 ITE & BMI as an unpublished work.
* Rubicon Dispenser
* All Rights Reserved.
*
* 7/11/16, Rev 1, Streicher / ITE
*   
********************************************************************************

*******************************************************************************/

#ifndef _AD_H
#define _AD_H

/*******************************************************************************
* INCLUDE FILES (all #include statements)
*******************************************************************************/

#include <MSP430x21x2.h>     		
#include <stdio.h>
#include "bmi.h"



/*******************************************************************************
* CONSTANTS AND MACRO DEFINITIONS (#define statements)
*******************************************************************************/

/* 1 channels x 8 samples each */
//#define AD_BUFFER_SIZE  8

/*******************************************************************************
* TYPEDEFS, STRUCTURES, AND ENUM DECLARATIONS
*******************************************************************************/
typedef enum
{
//    AD_PROCESS_INIT_FILTER,
    AD_PROCESS_FILTER,
    AD_PROCESS_CONVERT_TEMP,
    AD_PROCESS_CONVERT_INT_C,
    AD_PROCESS_CONVERT_INT_F,
    AD_PROCESS_CALC_TIME_FROM_START
} AdProcessSampleState;

/*******************************************************************************
* DATA DECLARATIONS (storage class is extern)
*******************************************************************************/
extern volatile unsigned int uiAd_Data[];         /* ad buffer */
extern volatile unsigned char ucAdBusy;          /* flag for ad status */
extern volatile unsigned int uiNewSampleAdCt;

extern float fltTemperature;
extern int iTemperature10xC;                 /* measured temperature x10, degC */
extern int iTemperature10xF;                 /* measured temperature x10, degF */
extern unsigned char ucAdSeedFilter ;

/*******************************************************************************
* FUNCTION DECLARATIONS (prototypes)
*******************************************************************************/
void ADStartAdSample(void);
void ADSetup(void);   
void ADTrigger(void);
void ADProcessSamples(void);

#endif
