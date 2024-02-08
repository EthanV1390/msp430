/*******************************************************************************
* interp.h
*
* Copyright 2016 ITE & BMI International as an unpublished work.
* All Rights Reserved.
*
* First written by Steve Streicher
*   
********************************************************************************

*******************************************************************************/

#ifndef _INTERP_H
#define _INTERP_H

/*******************************************************************************
* INCLUDE FILES (all #include statements)
*******************************************************************************/

/*******************************************************************************
* CONSTANTS AND MACRO DEFINITIONS (#define statements)
*******************************************************************************/
#ifdef AD_VREF
    #ifdef REV_1_HW 
        #define INTERP_THERM_LENGTH (19)    /* length of thermistor table axis */
    #else
        #define INTERP_THERM_LENGTH (10)    /* length of thermistor table axis */
    #endif
#endif
    
//#define INTERP_MAINTAIN_LENGTH  (10)  /* length of maintain mode time table */
#define INTERP_MAINTAIN_LENGTH  (8)  /* length of maintain mode time table */
//#define INTERP_TIMED_LENGTH (9)     /* length of temp vs time start table */
#define INTERP_TIMED_LENGTH (6)     /* length of temp vs time start table */
#define INTERP_XTAL_LENGTH (7)      /* length of temp vs crytal error table */


/*******************************************************************************
* TYPEDEFS, STRUCTURES, AND ENUM DECLARATIONS
*******************************************************************************/

/*******************************************************************************
* DATA DECLARATIONS (storage class is extern)
*******************************************************************************/

       
extern const float fltThermTemp[];          
extern const float fltThermCts[];
extern const float fltMaintainTemp[];    
extern const float fltMaintainOnTime[];         
extern const float fltMaintainOffTime[];
extern const float fltTimedTemp[];
extern const float fltTimedTime[];    


extern const float fltXtalErrorTemp[];
extern const float fltXtalErrorTime_us[];


/*******************************************************************************
* FUNCTION DECLARATIONS (prototypes)
*******************************************************************************/

float Interp2D ( float fltXin, float *pfltX, float *pfltY, unsigned int uiTableLength );
  
#endif
