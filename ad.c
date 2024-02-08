/*******************************************************************************
* ad.c
*
* Copyright 2016 ITE & BMI as an unpublished work.
* Rubicon Dispenser
* All Rights Reserved.
*
* 7/11/16, Rev 1, Streicher / ITE
*
********************************************************************************

*******************************************************************************/

/*******************************************************************************
* INCLUDE FILES (all #include statements)
*******************************************************************************/
#include <stdlib.h>
#include <stdbool.h>
#include "msp430x21x2.h"
#include "ad.h"
#include "bmi.h"
#include "uart.h"
#include "cal.h"    
#include "interp.h" 

/*******************************************************************************
* INTERNAL CONSTANTS AND MACRO DEFINITIONS
* (#define statements for definitions only used in this file)
*******************************************************************************/


/*******************************************************************************
* INTERNAL TYPEDEFS, STRUCTURES, AND ENUM DECLARATIONS
* (declarations only used in this file)
*******************************************************************************/
static float fltAd = 0;
static AdProcessSampleState adProcessSampleState = AD_PROCESS_FILTER;

/*******************************************************************************
* EXTERNAL DATA DEFINITIONS (storage class is blank)
* (definitions for global variables declared as extern in .h file)
*******************************************************************************/
//unsigned int uiAd_Data[AD_BUFFER_SIZE+1];         /* ad buffer */
//volatile unsigned int uiAd_Data[AD_BUFFER_SIZE];         /* ad buffer */
volatile unsigned char ucAdBusy = 0;
volatile unsigned char ucAdPending = 0;
volatile unsigned int uiNewSampleAdCt;
   
    
float fltTemperature = 21.9;
int iTemperature10xC = 219;                 /* measured temperature x10, in degC */
int iTemperature10xF = 714;                 /* measured temperature x10, in degF */

/* flag to force the next sample to seed the filtered A/D */
/* gets rid of the excessive settling time on power-up and out of sleep mode */
unsigned char ucAdSeedFilter = 1;               

/*******************************************************************************
* PRIVATE FUNCTION DECLARATIONS & DEFINITIONS (static)
* (functions only called from within this file)
*******************************************************************************/


/*******************************************************************************
* PUBLIC FUNCTION DEFINITIONS (storage class is blank)
* (functions called from outside this file, prototypes in .h file)
*******************************************************************************/

void ADSetup(void)
/*******************************************************************************
* DESCRIPTION: This function sets up pins and general ad stuff. 
*
* INPUTS: None
*
* RETURNS: None
*
*******************************************************************************/
{
#ifndef DEBUG_ACLK   
#ifdef AD_VREF
    /* set to inputs */
    P2DIR &= ~(VTEMP);  
    /* set to outputs */
//    P2DIR |= AD_VREF_OUTPUT;
    /* turn off the periph */
    P2SEL &= ~(VTEMP); 
#else
    /* set to inputs */
    P2DIR &= ~(VTEMP);  
    /* turn off the periph */
    P2SEL &= ~(VTEMP); 
#endif
#endif  
}

void ADStartAdSample(void)
/*******************************************************************************
* DESCRIPTION: This function sets up and starts the AD.
*
* INPUTS: None
*
* RETURNS: None
*
*******************************************************************************/
{
// 11/12/13
//    WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
    
#ifndef DEBUG_ACLK  



    ADC10CTL0 &= ~ENC;                          /* disable first */
    
    /* set input channel to channel 0  */
    /* set sample and hold clock to ADC10SC bit */
    /* use ADC10OSC for ad clock, no divide by, so ~5MHz */
    /* single conversion */
//    ADC10CTL1 = INCH_0 + SHS_0 + CONSEQ_0;  
ADC10CTL1 = INCH_0 + SHS_0 + ADC10DIV_3+ CONSEQ_0;       
    
    /* use 3.3V Vcc as reference */
    /* ADC sample and hold time of 16 clocks */
    /* reference burst on such that reference only turned on during sampling */
    /* reference = 3.3V vcc */
    /* turn on the ad and enable the interrupt */
//    ADC10CTL0 = SREF_0 + ADC10SHT_0 + MSC + ADC10ON + ADC10IE;  
#ifdef AD_VREF
    ADC10CTL0 = SREF_1 + ADC10SHT_3 + REFOUT + REFON + REF2_5V + ADC10ON + ADC10IE;  
#else
    ADC10CTL0 = SREF_1 + ADC10SHT_3 + REFON + REF2_5V + ADC10ON + ADC10IE;  
#endif

        
//    ADC10DTC1 = AD_BUFFER_SIZE;                 // number of conversions 
//    //ADC10DTC1 = 1;
//    ADC10SA = (unsigned short) &uiAd_Data[0];                    // Data buffer start
    
#ifdef AD_VREF    
    //enable channel 1 (0) and Vref output on P2.4
    ADC10AE0 |= 0x11;
#else
    /* enable channel 1 */
    ADC10AE0 |= 0x01; 
#endif
//ADC10AE0 |= 0x10;                         // P2.4 ADC option select    

    
    /* enable conversion and trigger */
    ADC10CTL0 |= (ENC|ADC10SC);
  
    ucAdPending = 1;  
    ucAdBusy = 1;       /* set flag to show sampling is in progress */
    
    
#endif
}


void ADTrigger(void)
/*******************************************************************************
* DESCRIPTION: This function determines when and triggers the a/d for a buffer
*               of samples. 
*
* INPUTS: None
*
* RETURNS: None
*
*******************************************************************************/
{
    static unsigned int uiLastTick = 0;     /* ticker tracker */
    
    if ( ( (uiTimerTick_250us - uiLastTick) >= AD_TRIGGER_CT) && (ucAdBusy == 0) )
    {
        uiLastTick = uiTimerTick_250us;
        ADStartAdSample();
    }
}

void ADProcessSamples(void)
/*******************************************************************************
* DESCRIPTION: This function processes a new ad sample
*               Measured temperature is low-pass filtered:
*               Filtered value = filtered value + (new sample - filtered value)*K
*               K = sampling rate (ms) / (LPF tau (ms) + sampling rate (ms) 

*               This routine takes ~440us to complete with conversions to temperature
*
* INPUTS: None
*
* RETURNS: None
*
*******************************************************************************/
{
    static unsigned int uiNumSamples = 0;
    
//    if ( (ucAdPending == 1) && (ucAdBusy == 0) )
//    {
///*DEBUG*/
////P2OUT |= 0x02;      
//   
//        //Filtered value = filtered value + (new sample - filtered value)*K
//        fltAd = fltAd + ( (uiNewSampleAdCt - fltAd)*AD_TEMP_LPF_K );
//        ucAdPending = 0;
//        
//        /* convert to temperature */
//        fltTemperature = Interp2D (fltAd, (float*) &fltThermCts[0], (float*) &fltThermTemp[0], INTERP_THERM_LENGTH);
//                    
//        /* convet to a 10x integer version, degC */
//        iTemperature10xC = (fltTemperature * 10);
//        
//        /* convet to a 10x integer version, degC */
//        iTemperature10xF = ( ( ((fltTemperature*9)/5) + 32) * 10);
//        
//#ifdef DEBUG_UART      
//        UARTSendData();
//#endif
        
/*DEBUG*/
//P2OUT &= ~0x02;         
//    }
    
/* DEBUG TO REDUCE CODE SIZE */
#ifdef REMOVE_SAMPLE_PROCESSING
    fltTemperature = 22.0;
    UpdateTempTimeFromStart();
#else   
    if ( (ucAdPending == 1) && (ucAdBusy == 0) )
    {
     
        /* process these things in separate states so we're not in this function too
        long. Doing this to provide enough display servicing */
        switch (adProcessSampleState)
        {
            case AD_PROCESS_FILTER:
            default:
            {
                if (ucAdSeedFilter)
                {
                    fltAd = uiNewSampleAdCt;
                    uiNumSamples++;
                    if (uiNumSamples >= NUM_SAMPLES_SEED_FILTER)
                    {
                        /* done seeding filter */
                        ucAdSeedFilter = 0;
                        uiNumSamples = 0;
                    }
                }
                else
                {
                    //Filtered value = filtered value + (new sample - filtered value)*K
                    fltAd = fltAd + ( (uiNewSampleAdCt - fltAd)*AD_TEMP_LPF_K );
                    
                }
                adProcessSampleState++; 
            }
            break;
            
            case AD_PROCESS_CONVERT_TEMP:
            {
                float fltAdjustedAd;
                
//                fltAdjustedAd = fltAd + calCalibrationData.uiTempMeasurementAdOffset;   
                fltAdjustedAd = fltAd;   
                 /* convert to temperature */
                fltTemperature = Interp2D (fltAdjustedAd, (float*) &fltThermCts[0], (float*) &fltThermTemp[0], INTERP_THERM_LENGTH);
                adProcessSampleState++; 
            }
            break;
            
            case AD_PROCESS_CONVERT_INT_C:
            {
                /* convet to a 10x integer version, degC */
                iTemperature10xC = (int) (fltTemperature * 10);   
                adProcessSampleState++;              
            }
            break;
            
            case AD_PROCESS_CONVERT_INT_F:
            {
                /* convet to a 10x integer version, degC */
                iTemperature10xF = (int) ( ( ((fltTemperature*9)/5) + 32) * 10);
                ucAdPending = 0;
                adProcessSampleState = AD_PROCESS_CALC_TIME_FROM_START; 
                
#ifdef DEBUG_UART      
        UARTSendData();
#endif                
            }
            break;
            
            case AD_PROCESS_CALC_TIME_FROM_START:
            {
                /* calculate how far ahead we should turn on relay */
                UpdateTempTimeFromStart();
                adProcessSampleState = AD_PROCESS_FILTER; 
            }
            break;
        }
        
    }    
#endif    

    
}

//void ADConvertToEngUnits(void)
///*******************************************************************************
//* DESCRIPTION: This function converts the filted a/d ct to enginering units,
//*               degC
//*
//* INPUTS: None
//*
//* RETURNS: None
//*
//*******************************************************************************/
//{
//    adEngUnits.fltTherm1 = Interp2D (adVolts.fltTherm1, (float*) &fltThermV[0], (float*) &fltThermTemp[0], INTERP_THERM_LENGTH);
//}

/******************************************************************************/
//                 Interrupts
/******************************************************************************/

// **** ADC10 ISR ******************************************
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_Handler(void)
{
    /* sample done, reset flag, and turn off ad */
    ucAdBusy = 0; 
    ADC10CTL0 = 0;
    ADC10CTL1 = 0;
    
    /* get sample */
    uiNewSampleAdCt = ADC10MEM;
}

