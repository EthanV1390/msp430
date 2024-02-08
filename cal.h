/*******************************************************************************
* cal.h
*
* Copyright 2016 ITE & BMI as an unpublished work.
* Rubicon Dispenser
* All Rights Reserved.
*
* 7/13/16, Rev 1, Streicher / ITE
*   
********************************************************************************

*******************************************************************************/

#ifndef _CAL_H
#define _CAL_H

/*******************************************************************************
* INCLUDE FILES (all #include statements)
*******************************************************************************/

#include <MSP430x21x2.h>     		
#include <stdio.h>


/*******************************************************************************
* CONSTANTS AND MACRO DEFINITIONS (#define statements)
*******************************************************************************/

#define CAL_FLASH_SECTOR_B  (0x1080)    /* address of flash sector b */
#define CAL_FLASH_SECTOR_C  (0x1040)    /* address of flash sector c */
#define CAL_FLASH_SECTOR_D  (0x1000)    /* address of flash sector d */

/* 8-bit value that is read back from flash to indicate it's been saved at least once*/
/* change this value to anything to force the code to re-zero everything next reset */
#define CAL_SAVED_FLASH_KEY (0x1A)  
#define CAL_TIME_SAVED_FLASH_KEY (0xA1)


/* configuration defaults */
#define START_TIME_HOUR_DEFAULT     (6)
#define START_TIME_MIN_DEFAULT      (00)
#define START_TIME_PM_DEFAULT       (0)
#define MAINTAIN_TIME_AFTER_START_DEFAULT   (2)
#define TEMP_UNITS_DEFAULT      (TEMP_UNITS_F)
#define TIME_SET_DEFAULT            (255)

/* calibration defaults */
#define TEMP_MEAS_AD_OFFSET_DEFAULT (0)     
#define CAL_TEMP_MEAS_AD_OFFSET_MAX         (9)   
#define CAL_TEMP_MEAS_AD_OFFSET_MIN         (-9)   
        
/*******************************************************************************
* TYPEDEFS, STRUCTURES, AND ENUM DECLARATIONS
*******************************************************************************/

/* cal error satus */
typedef enum
{
    CAL_NO_ERROR = 0,   /* no error */
    CAL_ERROR           /* error */
} CalError;

typedef struct
{
    unsigned char ucTimedConfigSaved;
    unsigned char ucMaintainConfigSaved;
    unsigned char ucStartTimeHours;
    unsigned char ucStartTimeMinutes;
    unsigned char ucStartTimePm;
    unsigned char ucMaintainTimeAfterStarTime;
    TempUnits tempUnits;
    unsigned char ucTimeSet;
} CalConfigData;

typedef struct
{
    float fltTotalNumRelayCycles;           /* total number of times relay has been energized */
    float fltTotalNumMinRunRelayOff;         /* total number of minutes unit is AC powered, but relay is not on */
    unsigned char ucStatDataZeroed;         /* flag to show stats have started */
    unsigned char ucStatFlashError;          /* flag to show flash/checksum error */
} CalStatData;
    
//typedef struct
//{
//    int uiTempMeasurementAdOffset;
//} CalCalibrationData;


#define CAL_CONFIG_NUM_BYTES    7
//#define CAL_CALIBRATION_NUM_BYTES   2  
#define CAL_STAT_NUM_BYTES  10

/*******************************************************************************
* DATA DECLARATIONS (storage class is extern)
*******************************************************************************/
extern CalConfigData calConfigData; 
//extern CalCalibrationData calCalibrationData;
extern CalStatData calStatData;
/*******************************************************************************
* FUNCTION DECLARATIONS (prototypes)
*******************************************************************************/
void CalSave(void);
void CalLoad(void);
unsigned char ucCalCheckWrittenFlash(void);

void CalSaveConfig(void);   
void CalConfigCheckLimits(void);
void CalSaveCalibration(void);
void CalSaveStats(void);

#endif
