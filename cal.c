/*******************************************************************************
* cal.c
*
* Copyright 2016 ITE & BMI as an unpublished work.
* Rubicon Dispenser
* All Rights Reserved.
*
* 7/13/16, Rev 1, Streicher / ITE
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
#include "cal.h"
#include "bmi.h"


/*******************************************************************************
* INTERNAL CONSTANTS AND MACRO DEFINITIONS
* (#define statements for definitions only used in this file)
*******************************************************************************/


/*******************************************************************************
* INTERNAL TYPEDEFS, STRUCTURES, AND ENUM DECLARATIONS
* (declarations only used in this file)
*******************************************************************************/

//static unsigned int uiNumSaves;  

static unsigned int uiCalcCheckSum = 0;        /* calculated checksum */
static unsigned int uiSavedCheckSum;           /* saved checksum value */

/*******************************************************************************
* EXTERNAL DATA DEFINITIONS (storage class is blank)
* (definitions for global variables declared as extern in .h file)
*******************************************************************************/

CalConfigData calConfigData;
//CalCalibrationData calCalibrationData;
CalStatData calStatData;

/*******************************************************************************
* PRIVATE FUNCTION DECLARATIONS & DEFINITIONS (static)
* (functions only called from within this file)
*******************************************************************************/

CalError CalSetupFlash(void)
/*******************************************************************************
* DESCRIPTION: This routine sets up for a flash erase/write. The watchdog
*               is turned off, the DCO is set to 1MHz, and the flash timing
*               generator is set. 
*
* INPUTS: None
*
* RETURNS: CalError - error status of DCO clock calibration
*
*******************************************************************************/
{   
    CalError calError = CAL_NO_ERROR;         /* assume no cal error */
    
    /* watchdog gets re-enabled in main loop when it's strobed */
    /* we don't go back to the main loop until the flash writes are done */
    WDTCTL = WDTPW + WDTHOLD;                 // Stop watchdog timer
    
    if (CALBC1_1MHZ ==0xFF || CALDCO_1MHZ == 0xFF)                                     
    {  
        calError = CAL_ERROR;                   /* clock values no good */
    }
    else
    {   
        /* no error, set the clock */
        BCSCTL1 = CALBC1_1MHZ;                    // Set DCO to 1MHz
        DCOCTL = CALDCO_1MHZ;
        FCTL2 = FWKEY + FSSEL0 + FN1;             // MCLK/3 for Flash Timing Generator  
    }
    
    return (calError);
}



/* 1/6/14 - having intermittent problem with data flash corruption, so re-write
functions to set write flag immediately after erase, and don't re-lock until all the 
data is done for that block */
void CalStartWriteFlash(unsigned int uiAddress)
/*******************************************************************************
* DESCRIPTION: This routine starts a write cycle for a data block. It erases
*               the block, then sets the write flag to start writing 
*
* INPUTS: uiAddress - starting address of the flash sector to erase
*
* RETURNS: none
*
*******************************************************************************/
{   
    unsigned char *pucByte;                 // Flash pointer
    
//    pucByte = (unsigned char *)0x1040;      // Initialize Flash pointer
    pucByte = (unsigned char *)uiAddress;   // Initialize Flash pointer
    FCTL3 = FWKEY + LOCKA;                  // Clear B,C,D Lock bit, keep Lock A
    FCTL1 = FWKEY + ERASE;                  // Set Erase bit
    *pucByte = 0;                           // Dummy write to erase Flash seg
    FCTL1 = FWKEY + WRT;                    // Set WRT bit for write operation
}

void CalSetDefaults(void)
/*******************************************************************************
* DESCRIPTION: This routine sets all defaults for configurations. 
*
* INPUTS: None
*
* RETURNS: None
*
*******************************************************************************/
{ 
    calConfigData.ucStartTimeHours = START_TIME_HOUR_DEFAULT;
    calConfigData.ucStartTimeMinutes = START_TIME_MIN_DEFAULT;
    calConfigData.ucStartTimePm = START_TIME_PM_DEFAULT;
    calConfigData.ucMaintainTimeAfterStarTime = MAINTAIN_TIME_AFTER_START_DEFAULT;
    /* write the key value */
//    calConfigData.ucConfigSaved = CAL_SAVED_FLASH_KEY;    
    /* units set separately in calsaveconfig */
//    calConfigData.tempUnits = TEMP_UNITS_DEFAULT ; 
    calConfigData.ucTimeSet = TIME_SET_DEFAULT;
}

//void CalSetCalibrationDefaults(void)
///*******************************************************************************
//* DESCRIPTION: This routine sets all defaults for configurations. 
//*
//* INPUTS: None
//*
//* RETURNS: None
//*
//*******************************************************************************/
//{   
//    calCalibrationData.uiTempMeasurementAdOffset = TEMP_MEAS_AD_OFFSET_DEFAULT;
//}

/*******************************************************************************
* PUBLIC FUNCTION DEFINITIONS (storage class is blank)
* (functions called from outside this file, prototypes in .h file)
*******************************************************************************/


void CalLoad(void)
/*******************************************************************************
* DESCRIPTION: This routine loads the calibrations from flash. 
*
* INPUTS: None
*
* RETURNS: None
*
*******************************************************************************/
{     
    unsigned char *pucByte;             /* pointer for cal in RAM  */
    unsigned char *pucByteFlash;        /* pointer for cal in Flash */
    unsigned int uiCt;
   
       
    /* CONFIGURATION ****************************************************************/
    /* set address to use flash sector B for first stat data structure */    
    pucByteFlash = (unsigned char *) CAL_FLASH_SECTOR_B;
    
    pucByte = (unsigned char*) &calConfigData;       /* set pointer to beginning of structure */
    
    for (uiCt = 0; uiCt < CAL_CONFIG_NUM_BYTES; uiCt++)
    {
        *pucByte = *pucByteFlash;                       /* get the current byte from flash */
        pucByte++;                                      /* increment the data pointer */
        pucByteFlash++;                                 /* increment the flash pointer */
    }

//    /* CALIBRATION ******************************************************************/
//    /* set address to use flash sector C for cal data structure */    
//    pucByteFlash = (unsigned char *) CAL_FLASH_SECTOR_C;
//    
//    pucByte = (unsigned char*) &calCalibrationData;       /* set pointer to beginning of structure */
//    
//    for (uiCt = 0; uiCt < CAL_CALIBRATION_NUM_BYTES; uiCt++)
//    {
//        *pucByte = *pucByteFlash;                       /* get the current byte from flash */
//        pucByte++;                                      /* increment the data pointer */
//        pucByteFlash++;                                 /* increment the flash pointer */
//    }

    /* Statistics */
    /* set addres to use flash sector C for statistics */
    pucByteFlash = (unsigned char *) CAL_FLASH_SECTOR_C;
    
    pucByte = (unsigned char*) &calStatData;       /* set pointer to beginning of structure */
    
    for (uiCt = 0; uiCt < CAL_STAT_NUM_BYTES; uiCt++)
    {
        *pucByte = *pucByteFlash;                       /* get the current byte from flash */
        pucByte++;                                      /* increment the data pointer */
        pucByteFlash++;                                 /* increment the flash pointer */
    }

    
    /* check for virgin flash */
    if (calConfigData.ucMaintainConfigSaved != CAL_SAVED_FLASH_KEY)
    {
        calConfigData.tempUnits = TEMP_UNITS_DEFAULT ;
    }
    
    if (calConfigData.ucTimedConfigSaved != CAL_SAVED_FLASH_KEY)
    {
        CalSetDefaults();
//        CalSetCalibrationDefaults();
        CalSaveConfig();
//        CalSaveCalibration();
    }
    
    /* check limits of configurations. If any out of range, go to defaults for all */
    if ( ((calConfigData.ucStartTimeHours > 12) || (calConfigData.ucStartTimeHours < 1)) ||
            (calConfigData.ucStartTimeMinutes > 59) || (calConfigData.ucStartTimePm > 1) || (calConfigData.ucMaintainTimeAfterStarTime > 9) )
    
    {
        CalSetDefaults();
        CalSaveConfig();
    }
    
    /* check stats are zeroed (i.e. initialized for the very first time */
    if (calStatData.ucStatDataZeroed != CAL_SAVED_FLASH_KEY)
    {
        //calStatData.fltTotalNumRelayCycles = 123400;  
        calStatData.fltTotalNumRelayCycles = 0;   
        //calStatData.fltTotalNumMinRunRelayOff = 34068000;   
        calStatData.fltTotalNumMinRunRelayOff = 0;
        calStatData.ucStatDataZeroed = CAL_SAVED_FLASH_KEY;
        calStatData.ucStatFlashError = 0;
        CalSaveStats();       
    }
//    /* check limits of calibration */
//    if (calCalibrationData.uiTempMeasurementAdOffset > CAL_TEMP_MEAS_AD_OFFSET_MAX)
//    {
//        calCalibrationData.uiTempMeasurementAdOffset = CAL_TEMP_MEAS_AD_OFFSET_MAX;
//        CalSaveCalibration();

//    }
//    else if (calCalibrationData.uiTempMeasurementAdOffset < CAL_TEMP_MEAS_AD_OFFSET_MIN)
//    {
//       calCalibrationData.uiTempMeasurementAdOffset = CAL_TEMP_MEAS_AD_OFFSET_MIN;
//       CalSaveCalibration();        
//    }
}


void CalSaveConfig(void)
/*******************************************************************************
* DESCRIPTION: This routine saves the stats structures to flash. This covers
*               flash block B & C 
*
* INPUTS: None
*
* RETURNS: CalError - error status of save
*
*******************************************************************************/
{     
    unsigned char *pucByte;
    unsigned char *pucByteFlash;
    unsigned int uiAddress;
    unsigned int uiCt;
    unsigned int uiCheckSum = 0;
//    unsigned int uiCalcCheckSum = 0;        /* calculated checksum */
//    unsigned int uiSavedCheckSum;           /* saved checksum value */
            
    CalError calError = CAL_NO_ERROR;   
    
    /* now turn off the timer interrupts (really should already be off because of the while loop above) and the 
    main system ticker interrupt. The flash controller disables these interrupts automatically while it 
    writes to flash, but it's not clear to me if that means writing each individual byte, or if it disables them
    for a time after the start of the first byte. If it allows interrupts between bytes, it's possible the duration
    of writing the entire structure could exceed the max 10ms specified for Tcpt. */
    TA0CCTL0 = 0;
    TA1CCTL0 = 0;
    
    /* set up the clock etc. */
    calError = CalSetupFlash();
        
    if (calError == CAL_NO_ERROR)
    { 
        /* set address to use flash sector B for config */    
        uiAddress = CAL_FLASH_SECTOR_B;
        
        CalStartWriteFlash(uiAddress);          /* start block write */
    
        pucByte = (unsigned char*) &calConfigData;       /* set pointer to beginning of structure */
        pucByteFlash = (unsigned char*) uiAddress;      /* set pointer to beginning of data flash */
        
        for (uiCt = 0; uiCt < CAL_CONFIG_NUM_BYTES; uiCt++)
        {
            *pucByteFlash = *pucByte;                       /* copy structure byte to flash data byte */
            uiCheckSum += *pucByte;                         /* calculate checksum */
            pucByteFlash++;                                 /* increment the flash address */
            pucByte++;                                      /* increment the data address */
        }

        /* save checksum */
        pucByteFlash++;                                 /* increment the flash address */
        pucByte = (unsigned char*) &uiCheckSum;         /* set pointer to beginning of structure */
        *pucByteFlash = *pucByte;                       /* copy structure byte to flash data byte */
        pucByteFlash++;                                 /* increment the flash address */
        pucByte++;                                      /* increment the data address */
        *pucByteFlash = *pucByte;                       /* copy structure byte to flash data byte */
        
        FCTL1 = FWKEY;                          // Clear WRT bit
        FCTL3 = FWKEY + LOCK + LOCKA;           // Set all LOCK bits        
    } 
           
    /* re-enable main system ticker interrupt */
    TA1CCTL0 |= CCIE;
    TA0CCTL0 |= CCIE;
    /* set clock back */
    SetupClock_Wake();
    
    /* now verify checksum */

    /* set address to use flash sector B for first stat data structure */    
    pucByteFlash = (unsigned char *) CAL_FLASH_SECTOR_B;
    
    uiCalcCheckSum = 0;
    
    for (uiCt = 0; uiCt < CAL_CONFIG_NUM_BYTES; uiCt++)
    {
        uiCalcCheckSum += *pucByteFlash;                /* get the current byte from flash */
        pucByteFlash++;                                 /* increment the flash pointer */
    }
   
 
    pucByteFlash++; 
    pucByte = (unsigned char*) &uiSavedCheckSum;       /* set pointer to beginning of structure */
    *pucByte = *pucByteFlash;                           /* get the current byte from flash */
    pucByte++;                                      /* increment the data pointer */
    pucByteFlash++;                                 /* increment the flash pointer */
    *pucByte = *pucByteFlash;                           /* get the current byte from flash  */
    
    if (uiCalcCheckSum != uiSavedCheckSum)
    {
        /* results don't match, so default */
        CalSetDefaults();     
    }
}


//void CalSaveCalibration(void)
///*******************************************************************************
//* DESCRIPTION: This routine saves the calibration
//*
//* INPUTS: None
//*
//* RETURNS: CalError - error status of save
//*
//*******************************************************************************/
//{     
//    unsigned char *pucByte;
//    unsigned char *pucByteFlash;
//    unsigned int uiAddress;
//    unsigned int uiCt;
//    unsigned int uiCheckSum = 0;
////    unsigned int uiCalcCheckSum = 0;        /* calculated checksum */
////    unsigned int uiSavedCheckSum;           /* saved checksum value */
//            
//    CalError calError = CAL_NO_ERROR;   
//    
//    /* now turn off the timer interrupts (really should already be off because of the while loop above) and the 
//    main system ticker interrupt. The flash controller disables these interrupts automatically while it 
//    writes to flash, but it's not clear to me if that means writing each individual byte, or if it disables them
//    for a time after the start of the first byte. If it allows interrupts between bytes, it's possible the duration
//    of writing the entire structure could exceed the max 10ms specified for Tcpt. */
//    TA0CCTL0 = 0;
//    TA1CCTL0 = 0;
//    
//    /* set up the clock etc. */
//    calError = CalSetupFlash();
//        
//    if (calError == CAL_NO_ERROR)
//    { 
//        /* set address to use flash sector c for calibration */    
//        uiAddress = CAL_FLASH_SECTOR_C;
//        
//        CalStartWriteFlash(uiAddress);          /* start block write */
//    
//        pucByte = (unsigned char*) &calCalibrationData;       /* set pointer to beginning of structure */
//        pucByteFlash = (unsigned char*) uiAddress;      /* set pointer to beginning of data flash */
//        
//        for (uiCt = 0; uiCt < CAL_CALIBRATION_NUM_BYTES; uiCt++)
//        {
//            *pucByteFlash = *pucByte;                       /* copy structure byte to flash data byte */
//            uiCheckSum += *pucByte;                         /* calculate checksum */
//            pucByteFlash++;                                 /* increment the flash address */
//            pucByte++;                                      /* increment the data address */
//        }
//
//        /* save checksum */
//        pucByteFlash++;                                 /* increment the flash address */
//        pucByte = (unsigned char*) &uiCheckSum;         /* set pointer to beginning of structure */
//        *pucByteFlash = *pucByte;                       /* copy structure byte to flash data byte */
//        pucByteFlash++;                                 /* increment the flash address */
//        pucByte++;                                      /* increment the data address */
//        *pucByteFlash = *pucByte;                       /* copy structure byte to flash data byte */
//        
//        FCTL1 = FWKEY;                          // Clear WRT bit
//        FCTL3 = FWKEY + LOCK + LOCKA;           // Set all LOCK bits        
//    } 
//           
//    /* re-enable main system ticker interrupt */
//    TA1CCTL0 |= CCIE;
//    TA0CCTL0 |= CCIE;
//    /* set clock back */
//    SetupClock_Wake();
//    
//    /* now verify checksum */
//
//    /* set address to use flash sector C for calibration */    
//    pucByteFlash = (unsigned char *) CAL_FLASH_SECTOR_C;
//    
//    uiCalcCheckSum = 0;
//    
//    for (uiCt = 0; uiCt < CAL_CALIBRATION_NUM_BYTES; uiCt++)
//    {
//        uiCalcCheckSum += *pucByteFlash;                /* get the current byte from flash */
//        pucByteFlash++;                                 /* increment the flash pointer */
//    }
//   
// 
//    pucByteFlash++; 
//    pucByte = (unsigned char*) &uiSavedCheckSum;       /* set pointer to beginning of structure */
//    *pucByte = *pucByteFlash;                           /* get the current byte from flash */
//    pucByte++;                                      /* increment the data pointer */
//    pucByteFlash++;                                 /* increment the flash pointer */
//    *pucByte = *pucByteFlash;                           /* get the current byte from flash  */
//    
//    if (uiCalcCheckSum != uiSavedCheckSum)
//    {
//        /* results don't match, so default */
//        CalSetCalibrationDefaults();     
//    }
//}


void CalSaveStats(void)
/*******************************************************************************
* DESCRIPTION: This routine saves the statistics
*
* INPUTS: None
*
* RETURNS: CalError - error status of save
*
*******************************************************************************/
{     
    unsigned char *pucByte;
    unsigned char *pucByteFlash;
    unsigned int uiAddress;
    unsigned int uiCt;
    unsigned int uiCheckSum = 0;
//    unsigned int uiCalcCheckSum = 0;        /* calculated checksum */
//    unsigned int uiSavedCheckSum;           /* saved checksum value */
            
    CalError calError = CAL_NO_ERROR;   
    
    /* now turn off the timer interrupts (really should already be off because of the while loop above) and the 
    main system ticker interrupt. The flash controller disables these interrupts automatically while it 
    writes to flash, but it's not clear to me if that means writing each individual byte, or if it disables them
    for a time after the start of the first byte. If it allows interrupts between bytes, it's possible the duration
    of writing the entire structure could exceed the max 10ms specified for Tcpt. */
    TA0CCTL0 = 0;
    TA1CCTL0 = 0;
    
    /* set up the clock etc. */
    calError = CalSetupFlash();
        
    if (calError == CAL_NO_ERROR)
    { 
        /* set address to use flash sector c for calibration */    
        uiAddress = CAL_FLASH_SECTOR_C;
        
        CalStartWriteFlash(uiAddress);          /* start block write */
    
        pucByte = (unsigned char*) &calStatData;       /* set pointer to beginning of structure */
        pucByteFlash = (unsigned char*) uiAddress;      /* set pointer to beginning of data flash */
        
        for (uiCt = 0; uiCt < CAL_STAT_NUM_BYTES; uiCt++)
        {
            *pucByteFlash = *pucByte;                       /* copy structure byte to flash data byte */
            uiCheckSum += *pucByte;                         /* calculate checksum */
            pucByteFlash++;                                 /* increment the flash address */
            pucByte++;                                      /* increment the data address */
        }

        /* save checksum */
        pucByteFlash++;                                 /* increment the flash address */
        pucByte = (unsigned char*) &uiCheckSum;         /* set pointer to beginning of structure */
        *pucByteFlash = *pucByte;                       /* copy structure byte to flash data byte */
        pucByteFlash++;                                 /* increment the flash address */
        pucByte++;                                      /* increment the data address */
        *pucByteFlash = *pucByte;                       /* copy structure byte to flash data byte */
        
        FCTL1 = FWKEY;                          // Clear WRT bit
        FCTL3 = FWKEY + LOCK + LOCKA;           // Set all LOCK bits        
    } 
           
    /* re-enable main system ticker interrupt */
    TA1CCTL0 |= CCIE;
    TA0CCTL0 |= CCIE;
    /* set clock back */
    SetupClock_Wake();
    
    /* now verify checksum */

    /* set address to use flash sector C for calibration */    
    pucByteFlash = (unsigned char *) CAL_FLASH_SECTOR_C;
    
    uiCalcCheckSum = 0;
    
    for (uiCt = 0; uiCt <  CAL_STAT_NUM_BYTES; uiCt++)
    {
        uiCalcCheckSum += *pucByteFlash;                /* get the current byte from flash */
        pucByteFlash++;                                 /* increment the flash pointer */
    }
   
 
    pucByteFlash++; 
    pucByte = (unsigned char*) &uiSavedCheckSum;       /* set pointer to beginning of structure */
    *pucByte = *pucByteFlash;                           /* get the current byte from flash */
    pucByte++;                                      /* increment the data pointer */
    pucByteFlash++;                                 /* increment the flash pointer */
    *pucByte = *pucByteFlash;                           /* get the current byte from flash  */
    
    if (uiCalcCheckSum != uiSavedCheckSum)
    {
        /* results don't match, set error flag to indicate problem */    
        calStatData.ucStatFlashError = 1;
    }   
}


//void CalConfigCheckLimits(void)
///*******************************************************************************
//* DESCRIPTION: This routine checks and limits the configuration values
//*
//* INPUTS: None
//*
//* RETURNS: CalError - error status of save
//*
//*******************************************************************************/
//{    
//    if (calConfigData.ucHighTemp > MAX_TEMP_C)    
//    {
//       calConfigData.ucHighTemp =  MAX_TEMP_C;
//    }
//    else if (calConfigData.ucHighTemp < MIN_TEMP_C)
//    {
//       calConfigData.ucHighTemp =  MIN_TEMP_C;
//    }
//
//    if (calConfigData.ucMedTemp > MAX_TEMP_C)    
//    {
//       calConfigData.ucMedTemp =  MAX_TEMP_C;
//    }
//    else if (calConfigData.ucMedTemp < MIN_TEMP_C)
//    {
//       calConfigData.ucMedTemp =  MIN_TEMP_C;
//    }
//    
//    if (calConfigData.ucLowTemp > MAX_TEMP_C)    
//    {
//       calConfigData.ucLowTemp =  MAX_TEMP_C;
//    }
//    else if (calConfigData.ucLowTemp < MIN_TEMP_C)
//    {
//       calConfigData.ucLowTemp =  MIN_TEMP_C;
//    }
//    
//
//    if (calConfigData.ucSineCycleTime > MODE_CYCLE_TIME_MAX_M)
//    {
//        calConfigData.ucSineCycleTime = MODE_CYCLE_TIME_MAX_M;
//    }
//    else if (calConfigData.ucSineCycleTime < MODE_CYCLE_TIME_MIN_M)
//    {
//        calConfigData.ucSineCycleTime = MODE_CYCLE_TIME_MIN_M;
//    }
//    
//    if (calConfigData.ucSineTempBoost > MAX_BOOST_TEMP)
//    {
//        calConfigData.ucSineTempBoost = MAX_BOOST_TEMP;
//    }
//       
//    /* minimum temperature must be less than the lowest max */
//    if (calConfigData.ucSineTempMin > calConfigData.ucHighTemp)
//    {
//        calConfigData.ucSineTempMin = (calConfigData.ucHighTemp - 1);
//    }
//    if (calConfigData.ucSineTempMin > calConfigData.ucMedTemp)
//    {
//        calConfigData.ucSineTempMin = (calConfigData.ucMedTemp - 1);
//    }
//    if (calConfigData.ucSineTempMin > calConfigData.ucLowTemp)
//    {
//        calConfigData.ucSineTempMin = (calConfigData.ucLowTemp - 1);
//    }
//    /* finnally, check it's not too low */
//    if (calConfigData.ucSineTempMin < MIN_TEMP_C )
//    {
//        calConfigData.ucSineTempMin = MIN_TEMP_C ;
//    }
//    
//    if (calConfigData.ucBurstOnTime > MODE_CYCLE_TIME_MAX_M)
//    {
//        calConfigData.ucBurstOnTime = MODE_CYCLE_TIME_MAX_M;
//    }
//    else if (calConfigData.ucBurstOnTime < MODE_CYCLE_TIME_MIN_M)
//    {
//        calConfigData.ucBurstOnTime = MODE_CYCLE_TIME_MIN_M;
//    }
//    
//    if (calConfigData.ucBurstOffTime > MODE_CYCLE_TIME_MAX_M)
//    {
//        calConfigData.ucBurstOffTime = MODE_CYCLE_TIME_MAX_M;
//    }
//    else if (calConfigData.ucBurstOffTime < MODE_CYCLE_TIME_MIN_M)
//    {
//        calConfigData.ucBurstOffTime = MODE_CYCLE_TIME_MIN_M;
//    }   
//
//    if (calConfigData.ucBurstTempBoost > MAX_BOOST_TEMP)
//    {
//        calConfigData.ucBurstTempBoost = MAX_BOOST_TEMP;
//    }
//
//    /* minimum temperature must be less than the lowest max */
//    if (calConfigData.ucBurstTempMin > calConfigData.ucHighTemp)
//    {
//        calConfigData.ucBurstTempMin = (calConfigData.ucHighTemp - 1);
//    }
//    if (calConfigData.ucBurstTempMin > calConfigData.ucMedTemp)
//    {
//        calConfigData.ucBurstTempMin = (calConfigData.ucMedTemp - 1);
//    }
//    if (calConfigData.ucBurstTempMin > calConfigData.ucLowTemp)
//    {
//        calConfigData.ucBurstTempMin = (calConfigData.ucLowTemp - 1);
//    }
//    /* finnally, check it's not too low */
//    if (calConfigData.ucBurstTempMin < MIN_TEMP_C )
//    {
//        calConfigData.ucBurstTempMin = MIN_TEMP_C ;
//    }
//    
//    if (calConfigData.ucRampDownTemp > 1)
//    {
//       calConfigData.ucRampDownTemp = 0;        /* default off */ 
//    } 
//    
//    if (calConfigData.powerMode > POWER_MODE_BURST)
//    {
//        calConfigData.powerMode = POWER_MODE_SETPOINT;
//    }    
//    
//    CalSaveConfig();
//    CalLoad();
//}

