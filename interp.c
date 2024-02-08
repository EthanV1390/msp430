/*******************************************************************************
* interp.c
*
* Copyright 2016 ITE & BMI International as an unpublished work.
* All Rights Reserved.
*
* First written by Steve Streicher
*   
********************************************************************************

*******************************************************************************/

/*******************************************************************************
* INCLUDE FILES (all #include statements)
*******************************************************************************/

//#include  <msp430x13x.h>
//#include <in430.h>
//#include  <io430x14x.h> 
//#include <msp430x14x.h>
#include "bmi.h"
//#include "uart.h"
//#include "spi.h"
#include "interp.h"


/*******************************************************************************
* INTERNAL CONSTANTS AND MACRO DEFINITIONS
* (#define statements for definitions only used in this file)
*******************************************************************************/

/* thermistor lookup table */
/* for Murata p/n NCP18XH103F03RB in a voltage divider with 33K fixed and Vs = 3.3V */
/* temperature, degC */
/* note the -50 data point is past the published data, so it's just extrapolated */
//const float fltThermTemp[] = {40, 30, 20, 10, 0, -10, -20, -30, -40, -50};
//const float fltThermTemp[] = {40, 35, 30, 25, 20, 15, 10, 5, 0, -5, -10, -15, -20, -25, -30, -35, -40, -45, -50}; 


//const float fltThermTemp[] = {40, 35, 30, 25, 20, 15, 10, 5, 0, -5, -10, -15, -20, -25, -30, -35, -40, -45, -50};    



/* voltage, a/d cts */   
/* this was for the thermistor powered from 3.3V, and 3.3V a/d reference, but in reality
a/d reference was around 3V due to diode or'ing the battery */                     
//const float fltThermCts[] = {153.7, 205.9, 274.1, 360.1, 462.4, 575.9, 689.5,
//                    792.3, 875.4, 952.7};
/* reconfigure with 100k resistor, 3.3V pull-up, and 2.5V reference */
//const float fltThermCts[] = {74.43, 103.66, 145.55, 205.27, 288.91, 402.78, 547.71,
//                        717.42, 893.62, 1068.39};
#ifdef AD_VREF
     
//    /* reconfigure with 33k resistor, 2.5V pull-up, and 2.5V reference */
//    const float fltThermCts[] = {153.7, 177.9, 205.9, 237.9, 274.1, 314.9, 360.1, 409.4, 462.4, 518.3, 575.9, 633.4,
//                                689.5, 743.0, 792.3, 836.7, 875.4, 914.1, 952.7};   
    /* Rev 2 - through hole thermistor, NXRT15WF104FA1B040 */
//     const float fltThermCts[] = {94.3, 115.0, 140.2, 170.8, 207.6, 251.5, 302.7, 361.3, 426.7, 497.1, 570.4, 643.6, 
//                                    713.6, 777.7, 833.7, 880.7, 918.7, 956.8, 994.8};   

     /* Rev 2 - panel mount probe NTCLP100E3103H */
//     const float fltThermCts[] = {98.7, 118.5, 142.2, 170.8, 204.8, 244.8, 291.4, 344.5, 403.9, 468.5, 536.6, 605.9,
//                                674.0, 738.1, 796.2, 846.9, 889.4, 931.9, 974.4};        
 
    #ifdef REV_1_HW 
     const float fltThermTemp[] = {40, 35, 30, 25, 20, 15, 10, 5, 0, -5, -10, -15, -20, -25, -30, -35, -40, -45, -50};   
     /* Rev 2 - panel mount probe NTCACAPE3C90191, 24k PU */
     const float fltThermCts[] = {102.1, 122.5, 147.0, 176.4, 211.3, 252.3, 299.8, 353.8, 414.0, 479.0, 547.3, 616.4,
                                    683.8, 747.0, 803.9, 853.3, 894.5, 927.9, 954.0};        
                                    
    #else   
    
/*4/3/17 cut back number of table entries to free code space */
//    const float fltThermTemp[] = {40, 35, 30, 25, 20, 15, 10, 5, 0, -5, -10, -15, -20, -25, -30, -35, -40, -45, -50};     
      const float fltThermTemp[] = {40, 30, 20, 10, 0, -10, -20, -30, -40, -50};                                
    /* Rev 2 Chinese Sample #2, Sanjing, Hefei Sensing, JY-MN103F3435, 49.9k PU */
//    const float fltThermCts[] = {106.9, 124.9, 146.0, 170.8, 199.7, 233.0, 271.3, 314.8, 363.3, 416.6, 473.9, 534.2, 595.8, 657.0,
//                                715.8, 770.7, 820.2, 869.7, 919.2};                      
    const float fltThermCts[] = {106.9, 146.0, 199.7, 271.3, 363.3, 473.9, 595.8, 715.8, 820.2, 919.2}; 

    /* Rev 2 Chinese Thermistor, Sample #3, Shiheng, 49.9k PU */
//    const float fltThermCts[] = {107.0, 125.0, 146.1, 170.8, 199.7, 233.1, 271.4, 314.8, 363.6, 416.4, 473.5, 533.6, 595.1, 656.4,
//                                    715.8, 771.4, 821.7, 872.1, 922.5}; 
    
    #endif                
#else     
    const float fltThermTemp[] = {40, 35, 30, 25, 20, 15, 10, 5, 0, -5, -10, -15, -20, -25, -30, -35, -40, -45, -50};    
    const float fltThermCts[] = {74.43, 87.73, 103.66, 122.76, 145.55, 172.8, 205.27, 243.7, 288.91, 341.82, 402.78, 471.51, 547.71, 630.39,
                        717.42, 806.24, 893.62, 981.0, 1068.39};   
#endif


/***************************************************************/                                       
/* PRE 2.001 VALUES - ORIGINAL VALUES FROM JEFF */                
///* maintain mode temperature breakpoints, degC */                   
//const float fltMaintainTemp[] = {-20, -18.33, -16.11, -13.89, -11.67, -9.44, -7.22,
//                -5, -2.78, 0.56, 1.67, 3.89, 6.11};      
///* maintain mode on-time, minutes */
//const float fltMaintainOnTime[] = {120.0, 120.0, 120.0, 120.0, 120.0, 120.0, 105.0, 90.0, 75.0, 60.0, 45.0, 30.0, 0};
///* maintain mode off-time, minutes */
//const float fltMaintainOffTime[] = { 0, 0, 0, 0, 0, 20, 20, 20, 20, 20, 20, 20, 1};              


/***************************************************************/   
/* REV 2.001+ - NEW VALUES FROM JEFF 01/09/17 */
/* maintain mode temperature breakpoints, degC  */    
/* NOTE: last 2 table entries for debug only. final release should have them set out of the wawy */               
//const float fltMaintainTemp[] = {-30.0, -20.6, -11.1, -5.6, -2.2, 0, 4.4, 5.6, 10, 40};
/* maintain mode on-time, seconds */
//const float fltMaintainOnTime[] = {900, 855, 720, 612, 387, 333, 72, 0, 5, 10};
/* maintain mode off-time, seconds */
//const float fltMaintainOffTime[] = {0, 45, 180, 288, 513, 567, 828, 900, 15, 25};    
/* Rev 2.003 - NEW VALUES - change duty cycle period from 15 MINUTES to 10 minutes */
/* maintain mode on-time, seconds */
//const float fltMaintainOnTime[] = {600, 570, 480, 408, 258, 222, 48, 0, 5, 10};
//const float fltMaintainOnTime[] = {600, 552, 480, 408, 258, 222, 48, 0, 0, 0};   
//const float fltMaintainOnTime[] = {600, 552, 480, 408, 258, 222, 48, 0, 0, 0};
/* maintain mode off-time, seconds */
//const float fltMaintainOffTime[] = {0, 30, 120, 192, 342, 378, 552, 600, 15, 25};    
//const float fltMaintainOffTime[] = {0, 48, 120, 192, 342, 378, 552, 600, 600, 600}; 
//const float fltMaintainOffTime[] = {0, 48, 120, 192, 342, 378, 552, 600, 600, 600};     

/*4/3/17 reduce size of table to free up room for xtal temp comp */
const float fltMaintainTemp[] = {-30.0, -20.6, -11.1, -5.6, -2.2, 0, 4.4, 5.6};
const float fltMaintainOnTime[] = {600, 552, 480, 408, 258, 222, 48, 0};
const float fltMaintainOffTime[] = {0, 48, 120, 192, 342, 378, 552, 600};  

/* timed mode temperature breakpoints, deg C */ 
/* NOTE: last table entry for debug only. final release should have it zeroed out */
//const float fltTimedTemp[] = {-50, -40, -30, -20, -10, 0, 10, 20};
//const float fltTimedTemp[] = {-45.6, -37.2, -23.3, -9.4, 1.7, 2.5, 5, 10, 40};
/* timed mode timer values, minutes before start time to turn on relay */
//const float fltTimedTime[] = {240, 180, 120, 60, 30, 10, 5, 1};
//const float fltTimedTime[] = {630, 480, 345, 210, 30, 0, 0, 0, 4};
//const float fltTimedTime[] = {630, 480, 345, 210, 30, 0, 0, 0, 180};
//const float fltTimedTime[] = {630, 480, 345, 210, 30, 0, 0, 0, 180};
//const float fltTimedTime[] = {630, 480, 345, 210, 30, 0, 0, 0, 0};

/*4/3/17 reduce size of table to free up room for xtal temp comp */
const float fltTimedTemp[] = {-45.6, -37.2, -23.3, -9.4, 1.7, 2.5};
const float fltTimedTime[] = {630, 480, 345, 210, 30, 0};

/***************************************************************/   
///*DEBUG VALUES */
///* maintain mode temperature breakpoints, degC  */                   
//const float fltMaintainTemp[] = {-45, -43, -40, -37, -32, -23, -9, 2, 3};
//  
/////* maintain mode on-time, minutes */
////const float fltMaintainOnTime[] = {3, 3, 3, 2, 2, 2, 2, 1, 1};
/////* maintain mode off-time, minutes */
////const float fltMaintainOffTime[] = {1, 1, 1, 1, 1, 1, 1, 1, 1};    
///* 1/10/17 change maintain times from minutes to seconds */
///* maintain mode on-time, minutes */
//const float fltMaintainOnTime[] = {180, 180, 180, 120, 120, 120, 120, 60, 10};
///* maintain mode off-time, minutes */
//const float fltMaintainOffTime[] = {60, 60, 60, 60, 60, 60, 60, 60, 10};    
//
///* timed mode temperature breakpoints, deg C */
////const float fltTimedTemp[] = {-50, -40, -30, -20, -10, 0, 10, 20};
//const float fltTimedTemp[] = {-45, -43, -40, -37, -32, -23, -9, 2, 3};
///* timed mode timer values, minutes before start time to turn on relay */
////const float fltTimedTime[] = {240, 180, 120, 60, 30, 10, 5, 1};
//const float fltTimedTime[] = {482, 481, 480, 465, 375, 300, 210, 30, 1};






///*DEBUG VALUES */
///* timed mode temperature breakpoints, deg C */
//const float fltTimedTemp[] = {-50, -40, -30, -20, -10, 0, 10, 20};
///* timed mode timer values, minutes before start time to turn on relay */
//const float fltTimedTime[] = {7, 6, 5, 4, 3, 2, 1, 0};


/*4/3/17*/
/* temperature compensation for crystal */
/* y-axis is 1 second clock error, in terms of us */
//const float fltXtalErrorTemp[] = {-50, -30, -15, 0, 15, 25, 35, 50, 65};
//const float fltXtalErrorTime_us[] = {191, 103, 54, 21, 3, 0, 3, 21, 54};
const float fltXtalErrorTemp[] = {-50, -30, -15, 15, 25, 35, 65};
const float fltXtalErrorTime_us[] = {191, 103, 54, 3, 0, 3, 54};

/* test at -50 to accrue in 5 minutes using potentiometer */
//const float fltXtalErrorTime_us[] = {3333, 103, 54, 3, 0, 3, 54};     

/*******************************************************************************
* INTERNAL TYPEDEFS, STRUCTURES, AND ENUM DECLARATIONS
* (declarations only used in this file)
*******************************************************************************/

/*******************************************************************************
* EXTERNAL DATA DEFINITIONS (storage class is blank)
* (definitions for global variables declared as extern in .h file)
*******************************************************************************/

/*******************************************************************************
* INTERNAL DATA DEFINITIONS (storage class is static)
* (definitions of variables that are global to this file)
*******************************************************************************/

/*******************************************************************************
* PRIVATE FUNCTION DECLARATIONS & DEFINITIONS (static)
* (functions only called from within this file)
*******************************************************************************/

int InterpGetXPosition(float fltXin, float *pfltX, unsigned int uiTableLength)
/*******************************************************************************
* DESCRIPTIION: The routine performs a binary search on the input X axis to 
*               determine the input values position. The position will correspond
*               to the table value just above the input value. The lower value
*               will then be 1 position below that. This routine assumes the 
*               input axis is monotonically increasing
*
* INPUTS:   fltXin - the input value to be used to determine position
*           pfltX - pointer to the lookup table's X-axis
*           uiTableLength - length of the table's axi         
*
* RETURNS:  (-1)  - value is outside table range on low side
*           (uiTableLength) - value is outside table range on the high side
*           (uiLow) - position of X axis, higher value 
*
*******************************************************************************/
{
    int uiLow, uiMid, uiHigh;
    
    uiLow = 0;
    uiHigh = (uiTableLength - 1);    
  
    if (fltXin <= *pfltX)
        /* value is off the table on the low side, or exactly equal to lowest, return -1 */
        return(-1);
    else if (fltXin >= *(pfltX + (uiTableLength - 1) ) )
        /* value is off the table on the high side, or exactly equal to hights, return hightest slot */
        return(uiTableLength);
    else
    {   
        /* value is in table, so binary search */   
        while (uiLow < uiHigh)
        {
            uiMid = (uiLow + uiHigh)/2;
            if ( *(pfltX + uiMid) < fltXin )
                uiLow = uiMid + 1;
            else
                uiHigh = uiMid;
        }
        
        /* uiLow ends up on the slot above the input value  */
        return(uiLow);
    }        
}

/*******************************************************************************
* PUBLIC FUNCTION DEFINITIONS (storage class is blank)
* (functions called from outside this file, prototypes in .h file)
*******************************************************************************/

float Interp2D ( float fltXin, float *pfltX, float *pfltY, unsigned int uiTableLength )
/*******************************************************************************
* DESCRIPTIION: The routine performs linear interpolation to return an output
*               value off the tables Y axis given an input value on the X
*               axis. The binary search requires that the X axis must always
*               be monotonically increasing.
*
* INPUTS:   fltXin - the input value to be used to determine the output
*           pfltX - pointer to the lookup tables X-axis
*           pfltY - pointer to the lookup tables Y-axis
*           uiTableLenght - length of the table's axis         
*
* RETURNS: fltYout  - the output value 
*
*******************************************************************************/
{
    int iXPosHi, iXPosLo;
    float fltYout;
    
    /* gets the array's position of the X input. The position is the one below
    the input value */
    iXPosHi = InterpGetXPosition(fltXin, pfltX, uiTableLength);
    iXPosLo = iXPosHi - 1;
        
    if (iXPosHi == -1)
        /* off table low, so return lowest value */
        return ( *pfltY );
    else if (iXPosHi == uiTableLength )
        /* off the table high, so return highest value */
        return ( *(pfltY + uiTableLength - 1) );
    else
    {
        fltYout = ( fltXin - *(pfltX + iXPosLo) )
                    /( *(pfltX + iXPosHi) - *(pfltX + iXPosLo) );
        fltYout *= ( *(pfltY + iXPosHi) - *(pfltY + iXPosLo) );
        fltYout += *(pfltY + iXPosLo);
        return (fltYout);
    }        
}

/*******************************************************************************
* Interrupt Functions
*******************************************************************************/

