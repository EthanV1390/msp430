/*******************************************************************************
* bmi.c
*
* Copyright 2014 ITE & BMI as an unpublished work.
* BMI Engine Block Heater Timer
* All Rights Reserved.
*
* 6/23/16, Rev 1, Streicher / ITE
*
********************************************************************************

*******************************************************************************/

/*******************************************************************************
* INCLUDE FILES (all #include statements)
*******************************************************************************/
#include <stdlib.h>
#include <stdbool.h>
#include "msp430x21x2.h"
#include "bmi.h"
#include "ad.h"
#include "cal.h"
#include "uart.h"
#include "interp.h"


/*******************************************************************************
* INTERNAL CONSTANTS AND MACRO DEFINITIONS
* (#define statements for definitions only used in this file)
*******************************************************************************/

/* array position represents the desired digit 0 - 9*/
const unsigned char ucDigiToHex[] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f};
/* special characters: "-", "C", "F", "H", "r", "O", "n", "E", "d", "y" */
//const unsigned char ucCharToHex[] = {0x40, 0x39, 0x71, 0x76, 0x50, 0x3f, 0x54, 0x79, 0x5e, 0x6e};
/* 072717 test differnt lower case r */
const unsigned char ucCharToHex[] = {0x40, 0x39, 0x71, 0x76, 0x31, 0x3f, 0x54, 0x79, 0x5e, 0x6e};

    
/*******************************************************************************
* INTERNAL TYPEDEFS, STRUCTURES, AND ENUM DECLARATIONS
* (declarations only used in this file)
*******************************************************************************/

static SwitchInputs switchInputs = {0};
//static SwitchPressed switchPressed = {0};
static unsigned char ucSw1Cnt = 0;
static unsigned char ucSw2Cnt = 0;
static unsigned char ucSw3Cnt = 0;
static unsigned char ucSw4Cnt = 0;

volatile static DisplayState displayState = DISP_STATE_INIT;
//static RunDisplayMode runDisplayMode = RUN_DISP_MODE_TIME;
//tatic ProgDisplayMode progDisplayMode = PROG_DISP_MODE_TIME;     
static DisplayMode displayMode = DISP_MODE_INIT;  

/* this array holds the raw display driver values */
volatile static unsigned char ucDisplayHexValue[] = {0xff, 0xff, 0xff, 0xff, 0xff};
static DisplayFlash displayFlash = DISP_NO_FLASH;
//static unsigned char ucInhibitFlashMomentarily = 0;     /* inhibits flashing when pressing button */
static DisplayInhibitFlash displayInhibitFlash = DISP_INHIBIT_FLASH_NO;
static unsigned char ucDisplayReady = 0; 
static unsigned int uiLastTickDisplay = 0;
static unsigned int uiDisplayAutoCycle = 0;
static unsigned char ucDigitOffTimeCt = 1;          /* off-time and brightness adjustment */
#ifdef AUTO_DIM 
    static unsigned char ucDispEnableAutoDim = 0;   /* flag to enable auto dim - don't dim if configurations must be set */
#endif

/* 010917 */
/* move from function so it can be initialed on wake up */
static unsigned int uiLastTickFlash = 0;

/*3/30/17 move timer out of RelayControl() so it can be incremented even on battery power */
static unsigned int uiLastTickMaintain = 0;
static unsigned char ucCancelMaintain = 0;


//static unsigned char ucUpdateDisplayBrightnessLastTick = 0;
//static unsigned char ucBrightnessUp = 1;

//static unsigned char ucStartTimeHours = 6;
//static unsigned char ucStartTimeMinutes = 30;
//static unsigned char ucStartTimePm = 0;

//static TempUnits tempUnits = TEMP_UNITS_C;

//static DiscreteLed discreteLed = {0x78};
static DiscreteLed discreteLed = {0x00};     

static RelayState relayState = RELAY_OFF;
static unsigned int uiRelayLastTick = 0;  

//static unsigned int uiLastSecTick_us = 0;
//static unsigned int uiLastMinTick_s = 0;

static float fltRelayTimeMin = RELAY_TIME_DEFAULT;          /* interpolated on or off time for relay, minutes */
static float fltRelayTimeSec = RELAY_TIME_SEC_DEFAULT;          /* interpolated on or off time for relay, seconds */

static float fltClockTimeAwayFromStart;         /* shows how many hours are left before start time */
static float fltTemperatureTimeAwayFromStart;   /* shows how many hours we need to start ahead of start time */
static unsigned char ucCancelTimedStart = 0;    /* flag to cancel timed start */

static unsigned char ucOSstate = 0;         /* simple OS counter to break up the slower routines to allow timely display service */

//static unsigned char ucForceSetTime = 1;            /* if time has never been set, force to go into set mode */
//static unsigned char ucForceSetMaintainConfig = 1;  /* if maintain configuration has never been set, force into mode */        
//static unsigned char ucForceSetTimedConfig = 1;     /* if timed start configuration has never been set, force into mode */          
static ForceSettings forceSettings;

#ifdef AUTO_DIM 
    static unsigned int uiLastSwitchPressTick = 0;
#endif

static StatDispMode statDispMode;

/*3/31/17*/
/* create new variable to hold start time. this is to handle the special case Jeff wants - if the user tries a timded start when
there actually isn't enough time. In this case, push out the start time to have enough */
static TimedStartConfig timedStartConfig;

/*4/3/17*/
/* RTC temperature compensation */
static float fltRTCAccruedError_us = 0;

/* 4/4/17 intermediate calculation for timed start */
static unsigned int uiTimeCalculation = 0;  

static unsigned char ucTimeSet = 0;
/*******************************************************************************
* EXTERNAL DATA DEFINITIONS (storage class is blank)
* (definitions for global variables declared as extern in .h file)
*******************************************************************************/

/* main timer tickers */
volatile unsigned int uiTimerTick_250us = 0;       
volatile unsigned int uiTimerTick_RTC_1s = 0;  
volatile unsigned int uiTimerTick_1s = 0;
volatile unsigned int uiTimerTick_1min = 0;
volatile unsigned int uiTimerTick_125us = 0;
volatile unsigned int uiMaintainTimeTick_1min = 0;


volatile unsigned char ucTimeHours = 12;
volatile unsigned char ucTimeMinutes = 0;
volatile unsigned char ucTimePm = 0;

volatile OperatingMode operatingMode = MODE_AWAKE_RUN;
OperatingMode operatingModeLast = MODE_AWAKE_RUN;
RunMode runMode = RUN_MODE_OFF;
RunMode runModeRelay = RUN_MODE_OFF;    /* separate run mode for relay because it runs both timed and maintain in timed setting */
ProgMode progMode = PROG_MODE_OFF;
OperatingState operatingState = OP_STATE_IDLE;
TimerModeRelayState timerModeRelayState = TIMER_MODE_RELAY_OFF;
MaintainModeRelayState maintainModeRelayState = MAINTAIN_MODE_RELAY_DISABLED;



/*******************************************************************************
* PRIVATE FUNCTION DECLARATIONS & DEFINITIONS (static)
* (functions only called from within this file)
*******************************************************************************/

void UpdateTimers(void)
/*******************************************************************************
* DESCRIPTION: This updates the long timer tickers
*
* INPUTS: None
*
* RETURNS: None
*
*******************************************************************************/
{
//    /* 1,000,000 sec/us / 250 us */
//    if ( (uiTimerTick_250us - uiLastSecTick_us) >= 4000)
//    {
//        uiLastSecTick_us = uiTimerTick_250us;
//        uiTimerTick_1s++;       /* increment seconds ticker */
//    }


/*srs*/
/* optimize code */   
/* can't set = RTC, because RTC gets zeroed after 60 sec. So increment this ticker
in the interrupt */ 
//    uiTimerTick_1s = uiTimerTick_RTC_1s;
    
    
/* move to interrupt */
//    if ( ( uiTimerTick_1s - uiLastMinTick_s) >= 60)
//    {
//        uiTimerTick_1min++;
//        uiLastMinTick_s = uiTimerTick_1s;   
//    }
    
    
////    /* 60 sec/ min */
////    if ( (uiTimerTick_1s - uiLastMinTick_s) >= 60)
////    {
////       uiLastMinTick_s =  uiTimerTick_1s;
////       uiTimerTick_1m++;        /* increment minutes ticker */
////    }
//
//    /* increment the slow 130ms sleep timer to compensate for periods when we're awake */
//    /* 130ms / 250us */
//    if ( (uiTimerTick_250us -  uiLast130msTick_us) >= 520)
//    {
//        uiLast130msTick_us = uiTimerTick_250us;
//        uiTimerTick_Sleep_130ms++;
//    }
}

void ClearTickers(void)
/*******************************************************************************
* DESCRIPTION: This clears the tickers
*
* INPUTS: None
*
* RETURNS: None
*
*******************************************************************************/
{
    /* clear timer tickers */
    uiTimerTick_250us = 0;
    uiTimerTick_1s = 0;
//    uiTimerTick_1m = 0;

    /* clear long timer thresholds */
//    uiLastSecTick_us = 0;
//    uiLastMinTick_s = 0;
    
}


void SetupPort(void)
/*******************************************************************************
* DESCRIPTION: This function sets up the port pins.
*
* INPUTS: None
*
* RETURNS: None
*
*******************************************************************************/
{
    /* Port 1 */
    P1DIR = (A1_CTRL | A2_CTRL | A3_CTRL | A4_CTRL | A5_CTRL | RELAY);
    P1SEL = 0;                                  /* turn all peripherals off for now, pwm/timer pins set when run */
    P1IE = 0; 
    P1IES = 0;
    P1IFG = 0;
    P1OUT = 0;        

/*DEBUG*/
P1DIR |= DEBUG_P15;
P1OUT &= ~DEBUG_P15;

    /* Port 2 */
    P2DIR = 0;
/*DEBUG*/
//P2DIR = (BIT1);
//    P2SEL = 0;
    /* set P2.6 and P2.7 for xtal */
    P2SEL |= 0xC0;
    P2IE = 0;
    P2OUT = 0;
    
/*DEBUG*/
/* look for aclk - same channel as thermistor 
if no op-amp, make sure R10 = 1k or capacitor loading prevents clock on pin */
#ifdef DEBUG_ACLK
P2DIR |= 0x01;     
P2SEL |= 0x01;
#endif

#ifdef DEBUG_UART   
    P3DIR = (A_CTRL | B_CTRL | C_CTRL | D_CTRL | G_CTRL | DP_CTRL);
    P3SEL &= ~(A_CTRL | B_CTRL | C_CTRL | D_CTRL | G_CTRL | DP_CTRL);
    P3OUT &= ~(A_CTRL | B_CTRL | C_CTRL | D_CTRL | G_CTRL | DP_CTRL);
    
#else    
    /* Port 3 */
    P3DIR = (A_CTRL | B_CTRL | C_CTRL | D_CTRL | E_CTRL | F_CTRL | G_CTRL | DP_CTRL);
    P3SEL = 0;
    P3OUT = 0;
    
    /* debug only */
    P3OUT |= 0x01;
#endif
}

void SetupTimerA0(void)
/*******************************************************************************
* DESCRIPTION: This function sets up timer A0. Timer A0 is used for the RTC.
*
* INPUTS: None
*
* RETURNS: None
*
*******************************************************************************/
{
    TA0CTL = TACLR;             /* stop the timer and clear it first */
    TA0CCTL0 = CCIE;                          // CCR0 interrupt enabled
    TA0CCR0 = 32767;                 // 1 sec interrupt with 32768 crystal on ACLK
    
    TA0CTL = TASSEL_1 + MC_1;                  // ACLK, count-up mode
//    TA0CTL = TASSEL_2 + MC_1;                  // MCLK, count-up mode
}
     
void SetupTimerA1_Wake(void)
/*******************************************************************************
* DESCRIPTION: This function sets up timer A1. Timer A1 is used for the main
*               system ticker while we're awake.
*
* INPUTS: None
*
* RETURNS: None
*
*******************************************************************************/
{
  TA1CTL = TACLR;             /* stop the timer and clear it first */

  TA1CCTL0 = CCIE;                          // CCR0 interrupt enabled
  TA1CCR0 = MAIN_TIMER_CTS;                 // 250us @ 8MHz (main timer tick)          
  
  /* need to increase speed of display processing, but didn't want to monkey with the
  250us timer, which is used all over. So create a new ticker 2 times as fast,
  and just add CCR1  increment it */
  TA1CCR1 = MAIN_TIMER_2X_CTS;                 // 250us @ 8MHz (main timer tick) 
  TA1CCTL1 = CCIE;                          // CCR0 interrupt enabled
  
  
  TA1CTL = TASSEL_2 + MC_1;                  // SMCLK, count-up mode
    
  
  /* 7/7/16 - which TAR is this really setting? According to .h file it's TA0 */
  //TAR = 0;
}

//void SetupTimerA1_Sleep(void)
///*******************************************************************************
//* DESCRIPTION: This function sets up timer A1 for sleep mode. This wakes every
//*               ~130ms to strobe the hand sensor's IR led.
//*
//* INPUTS: None
//*
//* RETURNS: None
//*
//*******************************************************************************/
//{
//    TA1CTL = TACLR;             /* stop the timer and clear it first */
//
//  TA1CCTL0 = CCIE;                          // CCR0 interrupt enabled
//  TA1CCR0 = SLEEP_TIMER_CTS;                 // 130ms @ 12kHz
//
//
//    TA1CCTL1 = OUTMOD_7;                       // reset/set mode on pin
////    TA1CTL = TASSEL_2 + MC_1;                  // ACLK, count-up mode
//  TA1CTL = TASSEL_1 + MC_1;                  // ACLK, count-up mode
//  
//  /* 7/7/16 - which TAR is this really setting? According to .h file it's TA0 */ 
////  TAR = 0;
//}

void SetupClock_Wake(void)
/*******************************************************************************
* DESCRIPTION: This function sets up the clock for operation when awake.
*
* INPUTS: None
*
* RETURNS: None
*
*******************************************************************************/
{
      DCOCTL = 0;                               // Select lowest DCOx and MODx settings
  BCSCTL1 = CALBC1_8MHZ;                    // Set DCO to 8MHz
  DCOCTL = CALDCO_8MHZ;
  
  
//    DCOCTL = CALDCO_8MHZ;                     // Set DCO to run at 8 MHz
//    BCSCTL1 = CALBC1_8MHZ;

                      // Set for 12.5pF xtal load capacitance
    BCSCTL2 = (SELM_0 | DIVM_0 | DIVS_0);    /* DCO for SMCLK & MCLK, no dividers */




    BCSCTL3 = (LFXT1S_0 | XCAP_3);             /* select 32.768kHz clock crystal for ACLK */
//    BCSCTL3 = (LFXT1S_0 | XCAP_3);             /* select 32.768kHz clock crystal for ACLK */





//    BCSCTL3 = (LFXT1S_2);                      /* select internal vlo */
    
    IFG1 &= ~OFIFG;                           // Clear osc fault bit
//    IE1 |= OFIE;                              // Enable oscillator fault int (NMI)
}

void SetupClock_Sleep(void)
/*******************************************************************************
* DESCRIPTION: This function sets up the clock for operation in sleep mode
*
* INPUTS: None
*
* RETURNS: None
*
*******************************************************************************/
{
//    BCSCTL3 |= LFXT1S_2;             /* select internal low power oscillator, ~12kHz */
//    IFG1 &= ~OFIFG;                           // Clear OSCFault flag
}



void DebounceSwitches ( void )
/*******************************************************************************
* DESCRIPTIION:
*
* INPUTS: None
*
* RETURNS: None
*
*******************************************************************************/
{
    static unsigned int uiLastTick = 0;
    
    if ( (uiTimerTick_250us - uiLastTick) >= SW_DEBOUNCE_UPDATE_RATE_CT)
    {
        uiLastTick = uiTimerTick_250us;
        
        /* switch 1, Up --------------------------------- */
        if (switchInputs.bits.Up)
        {
            /* switch already actuated (i.e, switch pressed, input high), so look for deactivation */
            if ((P2IN & UP) == 0)
            {
                /* port low, so switch is deactivated */
                if (ucSw1Cnt < SW_DEBOUNCE_CTS)
                {
                    ucSw1Cnt++;       /* bump counter */
                }
                else
                {
                   switchInputs.bits.Up = 0;
                }
            }
            else
            {
                ucSw1Cnt = 0;
            }
        }
        else
        {
           /* switch currently deactivated (i.e., switch not pressed, input low), so look for activation */
            if ((P2IN & UP) == UP)
            {
                /* port high, so switch activated */
                if (ucSw1Cnt < SW_DEBOUNCE_CTS)
                {
                    ucSw1Cnt++;       /* bump counter */
                }
                else
                {
                   switchInputs.bits.Up = 1;
                }
            }
            else
            {
                ucSw1Cnt = 0;
            }
        }
        
        /* switch 2, Down --------------------------------------- */
        if (switchInputs.bits.Down)
        {
            /* switch already actuated (i.e, switch pressed, input high), so look for deactivation */
            if ((P2IN & DOWN) == 0)
            {
                /* port low, so switch is deactivated */
                if (ucSw2Cnt < SW_DEBOUNCE_CTS)
                {
                    ucSw2Cnt++;       /* bump counter */
                }
                else
                {
                    switchInputs.bits.Down = 0;     /* door open */
                }
            }
            else
            {
                ucSw2Cnt = 0;
            }
        }
        else
        {
           /* switch currently deactivated (i.e., switch not pressed, input low), so look for activation */
            if ((P2IN & DOWN) == DOWN)
            {
                /* port high, so switch activated */
                if (ucSw2Cnt < SW_DEBOUNCE_CTS)
                {
                    ucSw2Cnt++;       /* bump counter */
                }
                else
                {
                    switchInputs.bits.Down = 1;         /* door closed */
                }
            }
            else
            {
                ucSw2Cnt = 0;
            }
        }
        
        
        /* switch 3, Program Mode -------------------------------------------------------------*/
        if (switchInputs.bits.ProgMode)
        {
            /* switch already actuated (i.e, switch pressed, input high), so look for deactivation */
            if ((P2IN & PROG_MODE) == 0)
            {
                /* port low, so switch is deactivated */
                if (ucSw3Cnt < SW_DEBOUNCE_CTS)
                {
                    ucSw3Cnt++;       /* bump counter */
                }
                else
                {
                   switchInputs.bits.ProgMode = 0;
                }
            }
            else
            {
                ucSw3Cnt = 0;
            }
        }
        else
        {
           /* switch currently deactivated (i.e., switch not pressed, input low), so look for activation */
            if ((P2IN & PROG_MODE) == PROG_MODE)
            {
                /* port high, so switch is activated */
                if (ucSw3Cnt < SW_DEBOUNCE_CTS)
                {
                    ucSw3Cnt++;       /* bump counter */
                }
                else
                {
                   switchInputs.bits.ProgMode = 1;
                }
            }
            else
            {
                ucSw3Cnt = 0;
            }
        }
        
            /* switch 4, Run Mode -------------------------------------------------------------*/
        if (switchInputs.bits.RunMode)
        {
            /* switch already actuated (i.e, switch pressed, input high), so look for deactivation */
            if ((P2IN & RUN_MODE) == 0)
            {
                /* port low, so switch is deactivated */
                if (ucSw4Cnt < SW_DEBOUNCE_CTS)
                {
                    ucSw4Cnt++;       /* bump counter */
                }
                else
                {
                   switchInputs.bits.RunMode = 0;
                }
            }
            else
            {
                ucSw4Cnt = 0;
            }
        }
        else
        {
           /* switch currently deactivated (i.e., switch not pressed, input low), so look for activation */
            if ((P2IN & RUN_MODE) == RUN_MODE)
            {
                /* port high, so switch is activated */
                if (ucSw4Cnt < SW_DEBOUNCE_CTS)
                {
                    ucSw4Cnt++;       /* bump counter */
                }
                else
                {
                   switchInputs.bits.RunMode = 1;
                }
            }
            else
            {
                ucSw4Cnt = 0;
            }
        }
    }
}


void SetTime (volatile unsigned char *ucHours, volatile unsigned char *ucMinutes, volatile unsigned char *ucStartTimePm)
/*******************************************************************************
* DESCRIPTIION: Handles the setting of the current time or the start time
*               using the up and down buttons
*
* INPUTS: Pointer to hour and minute variables
*
* RETURNS: None
*
*******************************************************************************/
{
    static SwitchInputs switchInputsLast = {0};
//    static unsigned int uiLastTick_Up = 0;
//    static unsigned int uiLastTick_Down = 0;
    static SwitchInputs switchInputsPressed = {0};
    static unsigned int uiLastTickIncremental = 0;
    static unsigned int uiAutoIncrementValue = 0;
    
    
    
    /************************ up button ****************************************/ 
    if ( (switchInputs.bits.Up == 1) && (switchInputsLast.bits.Up == 0) )
    {        
        /* any switch pressed, assume time is set */
//        calConfigData.ucTimeSet = CAL_TIME_SAVED_FLASH_KEY;
        
        /* firt, initial press */
        /* increase minute by 1 */
        if ((*ucMinutes) < 59)
        {
            (*ucMinutes)++;
        }
        else
        {
            (*ucMinutes) = 0;
            /* bump hours */
            if ((*ucHours) < 12)
            {
                (*ucHours)++;
                if ((*ucHours) == 12)
                {
                    /* flip AM/PM */
                    (*ucStartTimePm) ^= 1;
                }                
            }
            else
            {
                (*ucHours) = 1;
            }
        }
        /* set pressed flag to show we're holding, and record the start time */
        switchInputsPressed.bits.Up= 1;     
//        uiLastTick_Up = uiTimerTick_250us;   
        uiLastTickIncremental = uiTimerTick_250us;
//        uiAutoIncrementValue = 4000;        /* start slow, once per second */
        uiAutoIncrementValue = 1000;     
        
        displayInhibitFlash = DISP_INHIBIT_FLASH_SET;   
    }
    
    /* if we're holding button, auto-increment */
    if (switchInputsPressed.bits.Up == 1)   
    {
        if (switchInputs.bits.Up == 0)
        {
            /* released */
            switchInputsPressed.bits.Up = 0;
        }
        else
        {
            if ( (uiTimerTick_250us - uiLastTickIncremental) >  uiAutoIncrementValue) 
            {
                if ((*ucMinutes) < 59)
                {
                    (*ucMinutes)++;
                }
                else
                {
                    (*ucMinutes) = 0;
                    /* bump hours */
                    if ((*ucHours) < 12)
                    {
                        (*ucHours)++;
                        if ((*ucHours) == 12)
                        {
                            /* flip AM/PM */
                            (*ucStartTimePm) ^= 1;
                        }
                    }
                    else
                    {                        
                        (*ucHours) = 1;
                    }
                } 
                
                uiLastTickIncremental = uiTimerTick_250us;
                /* cut increment in half */
//                if (uiAutoIncrementValue > 40)
                if (uiAutoIncrementValue > 100)
                {
//                    uiAutoIncrementValue = uiAutoIncrementValue/2;
                    uiAutoIncrementValue = uiAutoIncrementValue - 50;
                }          
            }
        }  
        
        displayInhibitFlash = DISP_INHIBIT_FLASH_SET;
    }
    
    
    /************************ down button ****************************************/ 
    if ( (switchInputs.bits.Down == 1) && (switchInputsLast.bits.Down == 0) )
    {   
        /* any switch pressed, assume time is set */
//        calConfigData.ucTimeSet = CAL_TIME_SAVED_FLASH_KEY;
        
        /* firt, initial press */
        /* increase minute by 1 */
        if ((*ucMinutes) >0)
        {
            (*ucMinutes)--;
        }
        else
        {
            (*ucMinutes) = 59;
            /* bump hours */
            if ( (*ucHours) > 1)
            {
                (*ucHours)--;
            }
            else
            {
                (*ucHours) = 12;
            }
        }
        
        if ( ( (*ucHours) == 11)  &&   ( (*ucMinutes) == 59) )
        {
            /* flip AM/PM */
            (*ucStartTimePm) ^= 1;            
        }
        
        /* set pressed flag to show we're holding, and record the start time */
        switchInputsPressed.bits.Down= 1;     
//        uiLastTick_Down = uiTimerTick_250us;   
        uiLastTickIncremental = uiTimerTick_250us;
//        uiAutoIncrementValue = 4000;        /* start slow, once per second */
        uiAutoIncrementValue = 1000;     
        
        displayInhibitFlash = DISP_INHIBIT_FLASH_SET;   
    }
    
    /* if we're holding button, auto-increment */
    if (switchInputsPressed.bits.Down == 1)   
    {
        if (switchInputs.bits.Down == 0)
        {
            /* released */
            switchInputsPressed.bits.Down = 0;
        }
        else
        {
            if ( (uiTimerTick_250us - uiLastTickIncremental) >  uiAutoIncrementValue) 
            {
                if ( (*ucMinutes) > 0)
                {
                    (*ucMinutes)--;
                }
                else
                {
                    (*ucMinutes) = 59;
                    /* bump hours */
                    if ( (*ucHours) > 1)
                    {
                        (*ucHours)--;
                    }
                    else
                    {                        
                        (*ucHours) = 12;
                    }
                } 
                
                if ( ( (*ucHours) == 11)  &&  ( (*ucMinutes) == 59) )
                {
                    /* flip AM/PM */
                    (*ucStartTimePm) ^= 1;            
                }
                
                
                uiLastTickIncremental = uiTimerTick_250us;
                /* cut increment in half */
//                if (uiAutoIncrementValue > 40)
                if (uiAutoIncrementValue > 100)
                {
//                    uiAutoIncrementValue = uiAutoIncrementValue/2;
                    uiAutoIncrementValue = uiAutoIncrementValue - 50;
                }          
            }
        }  
        
        displayInhibitFlash = DISP_INHIBIT_FLASH_SET;
    }  
    
    /* refresh last value */
    switchInputsLast.byte = switchInputs.byte;
}

void ProcessSwitches ( void )
/*******************************************************************************
* DESCRIPTIION: Hanldes swith presses cauing changes in run mode and program 
*               modes
*
* INPUTS: None
*
* RETURNS: None
*
*******************************************************************************/
{
    static SwitchInputs switchInputsLast = {0};
//    /*062617 - initialize last press to all 1's to fix factory reset jumping to Off mode if buttons are held */
//    static SwitchInputs switchInputsLast = {0x0F};
    static SwitchInputs switchInputsPressed = {0};
    static unsigned int uiLastTick = 0;
    
    
    if ( (switchInputs.bits.RunMode == 1) && (switchInputsLast.bits.RunMode == 0) )
    {
        switchInputsPressed.bits.RunMode = 1;           /* switch pressed flag */
#ifdef AUTO_DIM         
        ucDigitOffTimeCt = DISP_DOT_FULL_BRIGHT;        /* display full brightness */
        uiLastSwitchPressTick = uiTimerTick_1s;
#endif
        /*6/20/17*/
        /* fix bug where stats are immediatelyd displayed if you press mode + up + down in "On" mode because this timer isn't initialized */
        uiLastTick = uiTimerTick_1s;
    }   
    else
    {
        switchInputsPressed.bits.RunMode = 0;
    }
    
    if ( (switchInputs.bits.ProgMode == 1) && (switchInputsLast.bits.ProgMode == 0) )
    {
        switchInputsPressed.bits.ProgMode = 1;          /* switch pressed flag */
#ifdef AUTO_DIM 
        ucDigitOffTimeCt = DISP_DOT_FULL_BRIGHT;        /* display full brightness */
        uiLastSwitchPressTick = uiTimerTick_1s;
#endif
    }
    else
    {
        switchInputsPressed.bits.ProgMode = 0;
    }
   
#ifdef AUTO_DIM
    /* look at up/down switches for auto-dim only */
    if ( ( (switchInputs.bits.Up == 1) && (switchInputsLast.bits.Up == 0) ) || ( (switchInputs.bits.Down == 1) && (switchInputsLast.bits.Down == 0) ) )
    {
        ucDigitOffTimeCt = DISP_DOT_FULL_BRIGHT;        /* display full brightness */
        uiLastSwitchPressTick = uiTimerTick_1s;        
    }
    
#endif   
   
#ifdef AUTO_DIM 
    if (ucDigitOffTimeCt == DISP_DOT_FULL_BRIGHT)
    {
        /* check for timeout only if configurations and time have been set */
        if ( (ucDispEnableAutoDim == 1) &&( (uiTimerTick_1s - uiLastSwitchPressTick) >= DISP_AUTO_DIM_TIME_CT) )
        {
            /* timed out, go to dim */
            ucDigitOffTimeCt = DISP_DOT_FULL_DIM;
        }
    }
#endif

   
    
    switch (operatingState)
    {
        case OP_STATE_IDLE:
        default:
        {       
            displayMode = DISP_MODE_IDLE;
            ucDispEnableAutoDim = 1;                    /* enable auto dimming */
   
#ifndef AUTO_DIM          
            if ( (switchInputs.bits.Up == 1) && (switchInputsLast.bits.Up == 0) )
            {
                /* brightness adjustments */               }
                ucDigitOffTimeCt = DISP_DOT_FULL_BRIGHT;       /* brightest */
            }
            else if ( (switchInputs.bits.Down == 1) && (switchInputsLast.bits.Down == 0) )
            {
                /* brightness adjustments */
                ucDigitOffTimeCt = DISP_DOT_FULL_DIM;       /* dimmest */
            }
#endif
                
            /* Rev 2.003 */
            /* press the up button or down button first, then mode button, hold for 10 seconds to show stats */
            if ( (switchInputs.bits.Up == 1) || (switchInputs.bits.Down == 1)  )
            {
                if ( switchInputs.bits.RunMode == 1)
                {
                    /* hold for 10 seconds to get to stats display */
                    if ( (uiTimerTick_1s - uiLastTick) >= SWITCH_HOLD_TIME_DISP_STATS_CT )
                    {
                        if (switchInputs.bits.Up == 1)
                        {
                            /* display relay cycles */
                            statDispMode = STAT_DISP_RELAY;
                            
                        }
                        else
                        {
                            /* display number of hours running without relay actuated */
                            statDispMode = STAT_DISP_HOURS_OFF;
                        }
                        operatingState = OP_STATE_DISP_STATS;
                    }                    
                }
                else
                {
                     uiLastTick = uiTimerTick_1s;
                }

            }              
            else if ( switchInputsPressed.bits.ProgMode == 1 || (forceSettings.bits.Time == 1) )
            {
                displayInhibitFlash = DISP_INHIBIT_FLASH_SET;
                //forceSettings.bits.Time =0;     /* clear flag */
            
                operatingState = OP_STATE_PROG_TIME;
                operatingMode = MODE_AWAKE_PROGRAM;
                progMode = PROG_MODE_TIME;
                displayMode = DISP_MODE_TIME;   
                
                /* turn off RTC timer for now, zero RTC ticker */   
                TA0CCTL0 = 0;
/*4/10/17*/
/*why zero this? It screws things up if we set the main 1s ticker = 1s RTC ticker to free up space */                
//                uiTimerTick_RTC_1s = 0;
                
                /* manually going into the programming settings, so clear the forced flags */
                if  (switchInputsPressed.bits.ProgMode == 1)
                {
                    forceSettings.byte = 0;
                }
            }
            else if (switchInputsPressed.bits.RunMode == 1)
            {
                /* if timed start config hasn't been set yet, go to its setting. In the case of maintain config not being set either, start at units */
                if ( (forceSettings.bits.TimedConfig == 1)  && (forceSettings.bits.MaintainConfig == 1) )
                {       
                    /* we already force a time set right on power up, so go to the next setting */
                    displayInhibitFlash = DISP_INHIBIT_FLASH_SET;
                    //forceSettings.bits.Time =0;     /* clear flag */      
                            
//                   operatingState = OP_STATE_PROG_START;
//                   operatingMode = MODE_AWAKE_PROGRAM;
//                   progMode = PROG_MODE_START_TIME;
//                   displayMode = DISP_MODE_START_TIME;
                    
                    operatingState = OP_STATE_PROG_UNITS;
                    operatingMode = MODE_AWAKE_PROGRAM;
                    runMode = RUN_MODE_TIMER;
                    progMode = PROG_MODE_TEMP_UNITS;
                    displayMode = DISP_MODE_TEMP_UNITS;  
                    
                    /* we will set the maintain mode configuration (temp units) in this process, so clear force flag */
//                    forceSettings.bits.MaintainConfig = 0;
                }
                /* if timed start config hasn't been set yet, go to its setting. In the case of maintain config already being set, skip units, and go to start time */
                else if ( (forceSettings.bits.TimedConfig == 1)  && (forceSettings.bits.MaintainConfig == 0) )
                {
                    /* we already force a time set right on power up, so go to the next setting */
                    displayInhibitFlash = DISP_INHIBIT_FLASH_SET;
                    
                    operatingState = OP_STATE_PROG_START;
                    operatingMode = MODE_AWAKE_PROGRAM;
                    runMode = RUN_MODE_TIMER;
                    progMode = PROG_MODE_START_TIME;
                    displayMode = DISP_MODE_START_TIME;                  
                }
                else
                {
                    /* configurations set, go to run mode, timed start */
                    operatingState = OP_STATE_RUN_TIMER;
                    operatingMode = MODE_AWAKE_RUN;
                    runMode = RUN_MODE_TIMER;
                    progMode = PROG_MODE_OFF;
                    ucDisplayReady = 0;
                    //discreteLed.bits.MaintainMode = 0;
                    //discreteLed.bits.TimedMode = 1;          
                    displayMode = DISP_MODE_INIT;         
                }  
                
                discreteLed.bits.MaintainMode = 0;
                discreteLed.bits.TimedMode = 1;
            }      
        }
        break;
        
        case OP_STATE_RUN_TIMER:
        { 
            ucDispEnableAutoDim = 1;                    /* enable auto dimming */
            
            /* program set button pressed */
            if (switchInputsPressed.bits.ProgMode == 1)
            {
                displayInhibitFlash = DISP_INHIBIT_FLASH_SET;
                //forceSettings.bits.Time =0;     /* clear flag */
            
                operatingState = OP_STATE_PROG_TIME;
                operatingMode = MODE_AWAKE_PROGRAM;
                progMode = PROG_MODE_TIME;
                displayMode = DISP_MODE_TIME;   
                
                /* turn off RTC timer for now, zero RTC ticker */   
                TA0CCTL0 = 0;
/*4/10/17*/
/*why zero this? It screws things up if we set the main 1s ticker = 1s RTC ticker to free up space */
//                uiTimerTick_RTC_1s = 0;
                
                /* set flag because we're going into programming from timed start run mode */
                forceSettings.bits.ReProgTimedConfig = 1;
            }
               
            /* run mode button pressed */       
            if (switchInputsPressed.bits.RunMode == 1)
            {  
                /* if maintain config hasn't been set yet, go to its setting */        
                if (forceSettings.bits.MaintainConfig == 1)
                {
                    operatingState = OP_STATE_PROG_UNITS;
                    progMode = PROG_MODE_TEMP_UNITS;
                    runMode = RUN_MODE_MAINTAIN;  
                    displayMode = DISP_MODE_TEMP_UNITS;        
                }
                else    
                {
                    operatingState = OP_STATE_RUN_MAINTAIN;
                    runMode = RUN_MODE_MAINTAIN;  
                    ucDisplayReady = 0; 
                    discreteLed.bits.MaintainMode = 1;
                    discreteLed.bits.TimedMode = 0;
                    
                    /* display */
                    /* this is just initialization, relay state machine handles if after here */
                    displayMode = DISP_MODE_TEMP;
                    uiLastTickDisplay = uiTimerTick_1s;
                }
            }
        }
        break;
        
        case OP_STATE_RUN_MAINTAIN:
        {
            ucDispEnableAutoDim = 1;                    /* enable auto dimming */
            
            /* program set button pressed */
            if (switchInputsPressed.bits.ProgMode == 1)
            {
                operatingState = OP_STATE_PROG_UNITS;
                operatingMode = MODE_AWAKE_PROGRAM;
                progMode = PROG_MODE_TEMP_UNITS;
                runMode = RUN_MODE_MAINTAIN;  
                displayMode = DISP_MODE_TEMP_UNITS;   
                     
                /* set flag because we're going into programming from maintain run mode */
                forceSettings.bits.ReProgMaintainConfig = 1;                               
            }
                        
            /* run mode button pressed */
            if (switchInputsPressed.bits.RunMode == 1)
            {     
                operatingState = OP_STATE_RUN_ON;
                runMode = RUN_MODE_ON;
                discreteLed.bits.MaintainMode = 0;
                discreteLed.bits.TimedMode = 0;
            }            
        }          
        break;     
                   
        case OP_STATE_RUN_ON:
        {          
            if (switchInputsPressed.bits.RunMode == 1)
            {
                operatingState = OP_STATE_IDLE;
                runMode = RUN_MODE_OFF;
                discreteLed.bits.MaintainMode = 0;
                discreteLed.bits.TimedMode = 0;
                
                /* display */
                //displayMode = DISP_MODE_ON;                
            }
/*4/10/17*/
/*add factory default sequence*/
//            else if (switchInputsPressed.bits.ProgMode == 1) 
            /* press UP + DOWN + SET to force a factory default */
            else if  (switchInputs.byte == 0x07)
            {
                calConfigData.ucTimedConfigSaved = 0xff;
                calConfigData.ucMaintainConfigSaved = 0xff;
                CalSaveConfig();
                WDTCTL = (WDTPW | WDTCNTCL | WDTSSEL);
                while (1)  
                {     
                    /* reset via watchdog */
                }                
            }
        }
        break;
        
        case OP_STATE_PROG_TIME:
        {
            ucDispEnableAutoDim = 0;                    /* disable auto dimming */
            
            SetTime(&ucTimeHours, &ucTimeMinutes, &ucTimePm);  
            
            if ( switchInputsPressed.bits.ProgMode == 1 )
            {
                /* if we got here by the force flag, just set this one setting and exit back to idle */
                if (forceSettings.bits.Time == 1)
                {
                    forceSettings.bits.Time = 0;     /* clear force flag */
                    /* exit programming state */
                    operatingState = OP_STATE_IDLE;
                    operatingMode = MODE_AWAKE_RUN;
                    progMode = PROG_MODE_OFF;
                    displayMode = DISP_MODE_INIT;   
                }
                else
                {
//                    operatingState = OP_STATE_PROG_START;
//                    progMode = PROG_MODE_START_TIME;
//                    displayMode = DISP_MODE_START_TIME;
//                    
                    
                    operatingState = OP_STATE_PROG_UNITS;
                    progMode = PROG_MODE_TEMP_UNITS;
                    displayMode = DISP_MODE_TEMP_UNITS;   
                    
                }
                
                /* turn RTC back on */
                TA0CCTL0 |= CCIE;  
                
                /*save time set flag */
                //calConfigData.ucTimeSet = CAL_TIME_SAVED_FLASH_KEY;          
                ucTimeSet =   CAL_TIME_SAVED_FLASH_KEY;  
            }    
        }
        break;
        
        case OP_STATE_PROG_UNITS:
        {
            ucDispEnableAutoDim = 0;                    /* disable auto dimming */
            
            if ( ( (switchInputs.bits.Up == 1) && (switchInputsLast.bits.Up == 0) ) || ( (switchInputs.bits.Down == 1) && (switchInputsLast.bits.Down == 0) ) )
            {
                /* toggle units */
                if (calConfigData.tempUnits == TEMP_UNITS_F)
                {
                    calConfigData.tempUnits = TEMP_UNITS_C;
                }
                else
                {
                    calConfigData.tempUnits = TEMP_UNITS_F;
                }
                displayInhibitFlash = DISP_INHIBIT_FLASH_SET; 
            }
             
            if ( switchInputsPressed.bits.ProgMode == 1 )  
            {           
//                if ( (forceSettings.bits.MaintainConfig == 1) && (runMode == RUN_MODE_MAINTAIN) )
                if ( ( (forceSettings.bits.MaintainConfig == 1) && (runMode == RUN_MODE_MAINTAIN) ) || (forceSettings.bits.ReProgMaintainConfig == 1) )       
                {
                    /* we got here by forcing the setting via mainatain run mode, so go back there */
                    operatingState = OP_STATE_RUN_MAINTAIN;
                    operatingMode = MODE_AWAKE_RUN;
                    runMode = RUN_MODE_MAINTAIN;  
                    ucDisplayReady = 0; 
                    discreteLed.bits.MaintainMode = 1;
                    discreteLed.bits.TimedMode = 0;
                    
                    /* display */
                    /* this is just initialization, relay state machine handles if after here */
                    displayMode = DISP_MODE_TEMP;
                    uiLastTickDisplay = uiTimerTick_1s;      
                    
                    /* we're now programmed for maintain mode */
                    forceSettings.bits.MaintainConfig = 0;    
                    forceSettings.bits.ReProgMaintainConfig = 0;                          
                }
                else
                {
                    /* we got here via timed start programming, continue that progression */
//                    operatingState = OP_STATE_PROG_MAINTAIN;         
//                    progMode = PROG_MODE_MAINTAIN_TIME;
//                    displayMode = DISP_MODE_MAINTAIN_TIME;      
                    
                    operatingState = OP_STATE_PROG_START;
                    operatingMode = MODE_AWAKE_PROGRAM;
                    progMode = PROG_MODE_START_TIME;
                    displayMode = DISP_MODE_START_TIME;                 
                }
    
                /* regardless of how we got here, temp units not set, so maintain config is complete */
                forceSettings.bits.MaintainConfig = 0;   
                /* save new settings to flash */
                calConfigData.ucMaintainConfigSaved = CAL_SAVED_FLASH_KEY;
                CalSaveConfig(); 
            }    
            
            if ( switchInputsPressed.bits.RunMode == 1 ) 
            {   
                /* run mode button pressed */
                if (runMode ==RUN_MODE_TIMER)
                {
                    /* we got here to program the units for timed mode, so now with another mode press go to maintain mode */
                    if (forceSettings.bits.MaintainConfig == 0)
                    {
                        /* units are set, so kick back to maintain mode */
                        operatingState = OP_STATE_RUN_MAINTAIN;
                        operatingMode = MODE_AWAKE_RUN;
                        //runMode = RUN_MODE_MAINTAIN;  
                        ucDisplayReady = 0; 
                        //discreteLed.bits.MaintainMode = 1;
                        //discreteLed.bits.TimedMode = 0;
                        
                        /* display */
                        /* this is just initialization, relay state machine handles if after here */
                        displayMode = DISP_MODE_TEMP;
                        uiLastTickDisplay = uiTimerTick_1s;    
                    }
                    else
                    {
                        /* we still have to set the units, but now we're going to be in maintain run mode */
                        /* so we stay in this operating state for now */
                         displayMode = DISP_MODE_TEMP_UNITS;
                    } 
                    runMode = RUN_MODE_MAINTAIN;
                    discreteLed.bits.MaintainMode = 1;
                    discreteLed.bits.TimedMode = 0;                      
                }
                else if (runMode == RUN_MODE_MAINTAIN)
                {
                    /* we got here to program the units for maintain mode, so now with another mode press go to the on mode */
                    operatingState = OP_STATE_RUN_ON;
                    operatingMode = MODE_AWAKE_RUN;
                    runMode = RUN_MODE_ON;
                    discreteLed.bits.MaintainMode = 0;
                    discreteLed.bits.TimedMode = 0;
                }
                
            }                            
        }
        break;    
            
        case OP_STATE_PROG_START:
        {
            ucDispEnableAutoDim = 0;                    /* disable auto dimming */
            
            SetTime(&calConfigData.ucStartTimeHours, &calConfigData.ucStartTimeMinutes, &calConfigData.ucStartTimePm);
            
            /* Program mode button press */
            if ( switchInputsPressed.bits.ProgMode == 1 )
            {     
//                operatingState = OP_STATE_PROG_UNITS;
//                progMode = PROG_MODE_TEMP_UNITS;
//                displayMode = DISP_MODE_TEMP_UNITS;                         
                              
                operatingState = OP_STATE_PROG_MAINTAIN;         
                progMode = PROG_MODE_MAINTAIN_TIME;
                displayMode = DISP_MODE_MAINTAIN_TIME;                
            }    
               
            /* Run mode button press */   
            if ( switchInputsPressed.bits.RunMode == 1 ) 
            {   
                /* we got here to program the units for timed mode, so now with another mode press go to maintain mode */
                if (forceSettings.bits.MaintainConfig == 0)
                {
                    /* units are set, so kick back to maintain mode */
                    operatingState = OP_STATE_RUN_MAINTAIN;
                    operatingMode = MODE_AWAKE_RUN; 
                    runMode = RUN_MODE_MAINTAIN;
                    ucDisplayReady = 0; 
                    
                    /* display */
                    /* this is just initialization, relay state machine handles if after here */
                    displayMode = DISP_MODE_TEMP;
                    uiLastTickDisplay = uiTimerTick_1s;  
                    discreteLed.bits.MaintainMode = 1;
                    discreteLed.bits.TimedMode = 0;  
                }                     
            }                           
        }           
        break;
        

        
        case OP_STATE_PROG_MAINTAIN:
        {
            ucDispEnableAutoDim = 0;                    /* disable auto dimming */
            
                if ( (switchInputs.bits.Up == 1) && (switchInputsLast.bits.Up == 0) )
                {
                    /* increment maintain configuration value in hours, limit to 9 hours */
                    if (calConfigData.ucMaintainTimeAfterStarTime < 9)
                    {
                        calConfigData.ucMaintainTimeAfterStarTime++;
                        displayInhibitFlash = DISP_INHIBIT_FLASH_SET;
                    }
                }
                else if ( (switchInputs.bits.Down == 1) && (switchInputsLast.bits.Down == 0) )
                {
                    /* increment maintain configuration value in hours, limit to 9 hours */
                    if (calConfigData.ucMaintainTimeAfterStarTime > 0)
                    {
                        calConfigData.ucMaintainTimeAfterStarTime--;
                        displayInhibitFlash = DISP_INHIBIT_FLASH_SET;
                    }                    
                }
                
                /* normal set button press to get out of this state */
                if ( switchInputsPressed.bits.ProgMode == 1 )    
                {   
                    if ( (forceSettings.bits.TimedConfig == 1) || (forceSettings.bits.ReProgTimedConfig == 1) )
                    {
                        /* we got here by forcing the settings trying to go into timed run mode, so exit back there */
                        operatingState = OP_STATE_RUN_TIMER;
                        runMode = RUN_MODE_TIMER;
                        progMode = PROG_MODE_OFF;
                        operatingMode = MODE_AWAKE_RUN;
                        ucDisplayReady = 0;
                        discreteLed.bits.MaintainMode = 0;
                        discreteLed.bits.TimedMode = 1;   
                        displayMode = DISP_MODE_INIT;    
                        
                        /* clear force flags */
                        forceSettings.bits.TimedConfig = 0;
                        forceSettings.bits.ReProgTimedConfig = 0;
                    }
                    else
                    {          
                        /* exit programming state */
                        operatingState = OP_STATE_IDLE;
                        operatingMode = MODE_AWAKE_RUN;
                        progMode = PROG_MODE_OFF;
                        displayMode = DISP_MODE_INIT;  
                    } 
                    
                    /* save new settings to flash */
                    /* set both saved flags. Even though this was the time settings, it also had to 
                    set units, which is the only maintain mode configuration, so it was inherently
                    done as well */
                    calConfigData.ucTimedConfigSaved = CAL_SAVED_FLASH_KEY;
                    calConfigData.ucMaintainConfigSaved = CAL_SAVED_FLASH_KEY;
                    CalSaveConfig(); 
                }  
                
                
//                if ( switchInputsPressed.bits.RunMode == 1 ) 
//                {
//                    if (forceSettings.bits.TimedConfig == 1)    
//                    {
//                        /* we're in the forced state, and mode button pressed, meaning we should go to maintain mode */
//                        /* if maintain config hasn't been set yet, go to its setting */        
//                        if (forceSettings.bits.MaintainConfig == 1)
//                        {
//                            operatingState = OP_STATE_PROG_UNITS;
//                            progMode = PROG_MODE_TEMP_UNITS;
//                            displayMode = DISP_MODE_TEMP_UNITS;        
//                        }
//                        else    
//                        {
//                            operatingState = OP_STATE_RUN_MAINTAIN;
//                            runMode = RUN_MODE_MAINTAIN;  
//                            ucDisplayReady = 0; 
//                            discreteLed.bits.MaintainMode = 1;
//                            discreteLed.bits.TimedMode = 0;
//                            
//                            /* display */
//                            /* this is just initialization, relay state machine handles if after here */
//                            displayMode = DISP_MODE_TEMP;
//                            uiLastTickDisplay = uiTimerTick_1s;
//                        }
//                    }
//                    else
//                    {
//                        /* special hidden menu button press, run mode, to go to calibration state */ 
//                        operatingState = OP_STATE_CALIBRATION;
//                        progMode = PROG_MODE_CALIBRATION;
//                        displayMode = DISP_MODE_CAL; 
//                        displayInhibitFlash = DISP_INHIBIT_FLASH_SET;
//                    }
//
//                }                            
        }
        break;
        
        case OP_STATE_DISP_STATS:
        {
            /* exit out of this mode by pressing any button */
            if ( (switchInputs.byte != 0) && (switchInputsLast.byte == 0) )
            {
                operatingState = OP_STATE_IDLE;
            }
            
            displayMode = DISP_MODE_STATS;
        }
        break;
        
        
//        case OP_STATE_CALIBRATION:
//        {
//            ucDispEnableAutoDim = 0;                    /* disable auto dimming */
//            
//            if ( (switchInputs.bits.Up == 1) && (switchInputsLast.bits.Up == 0) )
//            {
//                displayInhibitFlash = DISP_INHIBIT_FLASH_SET;
//                
//                if (calCalibrationData.uiTempMeasurementAdOffset < CAL_TEMP_MEAS_AD_OFFSET_MAX)
//                {
//                    calCalibrationData.uiTempMeasurementAdOffset++;
//                }
//            }
//            else if ( (switchInputs.bits.Down == 1) && (switchInputsLast.bits.Down == 0) )
//            { 
//                displayInhibitFlash = DISP_INHIBIT_FLASH_SET;
//                
//                if (calCalibrationData.uiTempMeasurementAdOffset > CAL_TEMP_MEAS_AD_OFFSET_MIN)
//                {
//                    calCalibrationData.uiTempMeasurementAdOffset--;
//                }
//            }
//                
//            if ( switchInputsPressed.bits.ProgMode == 1 )    
//            {                
//                /* exit programming state */
//                operatingState = OP_STATE_IDLE;
//                operatingMode = MODE_AWAKE_RUN;
//                progMode = PROG_MODE_OFF;
//                displayMode = DISP_MODE_INIT;   
//                /* save new settings to flash */
//                CalSaveCalibration(); 
//            }                         
//        } 
//        break;
    }
    

    
    /* set the flash flag */
    if (operatingMode == MODE_AWAKE_RUN)
    {
        if ( (operatingState == OP_STATE_DISP_STATS) && (calStatData.ucStatFlashError == 1) )
        {
            /* special case, stats displayed with data flash memory error */
            displayFlash = DISP_FLASH;
        }
        else
        {
            displayFlash = DISP_NO_FLASH;  
        }
    }
    else
    {
        /* programming state */
        displayFlash = DISP_FLASH;  
    }   
    
    /* refresh last value */
    switchInputsLast.byte = switchInputs.byte;
}



void SetTimeDisplay(unsigned char ucHours, unsigned char ucMinutes, unsigned char ucPm)
/*******************************************************************************
* DESCRIPTIION: Translates the time to display hex values.
*
* INPUTS: None
*
* RETURNS: None
*
*******************************************************************************/
{
    /* anode 1, digit 1*/
    if (ucHours > 9)
    {
        /* set to "1" */
        ucDisplayHexValue[0] = ucDigiToHex[1];    
    }
    else
    {
        /* blank */   
        ucDisplayHexValue[0] =  0;   
    }
    
    /* anode 2, digit 2 */
    if (ucHours > 9)
    {
        /* 10 or greater, subtract 10 to get LSB digit of hours */
        ucDisplayHexValue[1] = ucDigiToHex[ucHours - 10];
    }
    else
    {
        /* single digit is the value */   
        ucDisplayHexValue[1] =  ucDigiToHex[ucHours];
    }  

// move to UpdateDisplayValues()             
    /* anode 3, clock colon */
//    ucDisplayHexValue[2] |= 0x03;
    
    /* anode 4, digit 3 */
    if (ucMinutes > 9)
    {
        /* msb of minutes */
        ucDisplayHexValue[3] = ucDigiToHex[ucMinutes/10];
    }
    else
    {
        /* zero */
        ucDisplayHexValue[3] =  ucDigiToHex[0];
    }
    
    /* anode 5, digit 4, and PM segment */
    /* lsb of minutes */
    if (ucPm == 1)
    {
        ucDisplayHexValue[4] = ucDigiToHex[ucMinutes%10] | 0x80;
    }
    else
    {
        ucDisplayHexValue[4] = ucDigiToHex[ucMinutes%10];
    }
}

void UpdateDisplayValues(void )
/*******************************************************************************
* DESCRIPTIION: Determines what to display 
*               During initial testing, with basically no other code running,
*               this routine hits every 37us.
*
* INPUTS: None
*
* RETURNS: None
*
*******************************************************************************/
{
//    static unsigned int uiLastTick = 0;
//    static unsigned int uiLastTickFlash = 0;
    static unsigned char ucFlashStateOff = 0;
    
    /* 090116 move below inside if branch */
    /* initialize 3rd position with discrete led settings */
//    ucDisplayHexValue[2] = discreteLed.byte;
    
    if (displayInhibitFlash == DISP_INHIBIT_FLASH_SET) 
    {
        uiLastTickFlash = uiTimerTick_250us;  
        displayInhibitFlash = DISP_INHIBIT_FLASH;
        ucFlashStateOff = 0;        /* clear flash flag */
    }
    else if (displayInhibitFlash == DISP_INHIBIT_FLASH)
    {
        if ((uiTimerTick_250us - uiLastTickFlash) >= DISP_INHIBIT_FLASH_CT)
        {
            displayInhibitFlash = DISP_INHIBIT_FLASH_NO;
            uiLastTickFlash = uiTimerTick_250us;
        }
    } 
    else if (displayFlash == DISP_FLASH)
    {
        if ( (uiTimerTick_250us - uiLastTickFlash) >= DISP_FLASH_TIME_CT)
        {
            ucFlashStateOff ^= 1;
/* debug */
//ucFlashStateOff = 1;            
            uiLastTickFlash = uiTimerTick_250us;
        }
    }
    else
    {
        ucFlashStateOff = 0;        /* clear flash flag */
    }
    
    if (ucFlashStateOff)
    {
        /* flash cycle, off, blank digits */
        ucDisplayHexValue[0] = 0;
        ucDisplayHexValue[1] = 0;
        ucDisplayHexValue[2] = 0;
        ucDisplayHexValue[3] = 0;
        ucDisplayHexValue[4] = 0;
    }
    else
    {   
//        /* if we're in prog mode, the display state is set in the programming state machine */
//        /* if we're in run mode, force a couple of states, otherwise let it cycle */
//        if (runMode == RUN_MODE_MAINTAIN)
//        {
//            displayMode = DISP_MODE_TEMP;
//        }
//        else if (runMode == RUN_MODE_ON)
//        {
//            displayMode = DISP_MODE_ON;
//        }
        
        /*12/21/16 - degree symbol flickering. Scope shows LSD for that segment toggling every 125us, presumably because it gets zeroed here, then set high 
        in the switch statement below. Because this updates slower than the display handler, the display handler interrupt may hit here then anain below all 
        in the same call, causing a toggle. Create temporary variable and set array slot at the end */
        /* initialize 3rd position with discrete led settings */
        //ucDisplayHexValue[2] = discreteLed.byte;
//        unsigned char ucDisplayHexValue0Calc;
        unsigned char ucDisplayHexValue2Calc = discreteLed.byte;
    
        switch (displayMode)
        {
            case DISP_MODE_INIT:
            default:
            {
                uiLastTickDisplay = uiTimerTick_1s;
                displayMode = DISP_MODE_TIME;                
            }
            break;
            
            case DISP_MODE_IDLE:
            {
                /* in idle mode, display "OFF" */
                ucDisplayHexValue[0] = ucCharToHex[5];  //"O"   
//                ucDisplayHexValue0Calc = ucCharToHex[5];  //"O"    
                ucDisplayHexValue[1] = ucCharToHex[2];  //"F";
//                ucDisplayHexValue[2] = 0;
                ucDisplayHexValue2Calc = 0;
                ucDisplayHexValue[3] = ucCharToHex[2];  //"F";
                ucDisplayHexValue[4] = 0;
                
//                if ( (uiTimerTick_250us - uiLastTickFlash) > 2000)
//                {
//                    uiLastTickFlash = uiTimerTick_250us;
//                    if (discreteLed.bits.MaintainMode == 1)
//                    {
//                        
//                        discreteLed.bits.TimedMode = 1;
//                        discreteLed.bits.MaintainMode = 0;
//                    }
//                    else
//                    {
//                        discreteLed.bits.TimedMode = 0;
//                        discreteLed.bits.MaintainMode = 1;
//                    }
//                }
            }
            break;
            
            case DISP_MODE_TIME:
            {
                /* use local copy to get rid of compiler warning for volatile access */
                unsigned char ucTimeHoursLocal = ucTimeHours;
                unsigned char ucTimePmLocal = ucTimePm;
                
                /* calulate display hex values */
                SetTimeDisplay(ucTimeHoursLocal, ucTimeMinutes, ucTimePmLocal);
//                SetTimeDisplay(ucTimeHoursLocal, ucTimeMinutes, &ucDisplayHexValue0Calc);
//                ucDisplayHexValue0Calc = ucDisplayHexValue[0];          /*actual array set in SetTimeDisplay */  
                              
                /* set the discrete LED indicating current time */       
//                ucDisplayHexValue[2] |= 0x08;
                ucDisplayHexValue2Calc |= 0x0B;     /* set bits for discrete led for current time, as well as colon */

                             
                /* 2.003 - move the pm control to SetTimeDisplay() to prevent flicker */
                /* set the PM indication, if necessary. This indicated by the last decimal place */
//                if (ucTimePm == 1)
//                {
//                    ucDisplayHexValue[4] |= 0x80;
//                }
                
                /* if we're in timed mode, or iddle automatically cycle through display values based on the timer */
                /* if program mode, states are forced in the programing state machine */
                /* maintain mode control is done in relay control state machine */
                if ( (operatingMode == MODE_AWAKE_RUN) && (uiDisplayAutoCycle == 1) )
                {
                    if ((uiTimerTick_1s - uiLastTickDisplay) >= DISP_RUN_SCREEN_TIME_CT)
                    {
                        uiLastTickDisplay = uiTimerTick_1s;
                        displayMode = DISP_MODE_START_TIME;
                    }
                }
            }
            break;
            
            case DISP_MODE_START_TIME:
            {
                /* calulate display hex values */
//               SetTimeDisplay(ucStartTimeHours, ucStartTimeMinutes);
                SetTimeDisplay(calConfigData.ucStartTimeHours, calConfigData.ucStartTimeMinutes, calConfigData.ucStartTimePm);  
//                SetTimeDisplay(calConfigData.ucStartTimeHours, calConfigData.ucStartTimeMinutes,  &ucDisplayHexValue0Calc);   
//                ucDisplayHexValue0Calc = ucDisplayHexValue[0];          /*actual array set in SetTimeDisplay */      
                
                /* set the discrete LED indicating start time */                     
//                ucDisplayHexValue[2] |=  0x10;
                ucDisplayHexValue2Calc |= 0x13;     /* set bits for discrete led for set time, as well as colon */
                      
               /* 2.003 - move the pm control to SetTimeDisplay() to prevent flicker */       
//                /* set the PM indication, if necessary. This indicated by the last decimal place */
//                if (calConfigData.ucStartTimePm)
//                {
//                    ucDisplayHexValue[4] |= 0x80;
//                }
 
                /* if we're in run mode, automatically cycle through display values based on the timer */
                /* if program mode, states are forced in the programing state machine */
                if ( (operatingMode == MODE_AWAKE_RUN) && (uiDisplayAutoCycle == 1) )   
                {               
                    if ((uiTimerTick_1s - uiLastTickDisplay) >= DISP_RUN_SCREEN_TIME_CT)
                    {
                        uiLastTickDisplay = uiTimerTick_1s;
                        displayMode = DISP_MODE_TEMP;                  
                    }       
                }         
            }
            break;
            
            case DISP_MODE_TEMP:
            {
//                int iTempParse;
//                
//                if (calConfigData.tempUnits == TEMP_UNITS_C)
//                {
//                    /* deg C */
//                    iTempParse = iTemperature10xC;
//                }
//                else
//                {
//                    /* deg F */
//                    iTempParse = iTemperature10xF;                    
//                }
//                
//                if (iTempParse < 0)
////                if (iTempParse < -4)
//                {
//                    /* negative value, so leading digit is "-" . Look up dash in symbol array*/
//                    ucDisplayHexValue[0] = ucCharToHex[0];    
////                    ucDisplayHexValue0Calc = ucCharToHex[0]; 
//                    
//                    /* convert to positive value so the math and the num to hex array indexing works */
//                    iTempParse = iTempParse * -1;
//                    
//                    /* since we use the first digit for the negative sign, and the last digit for C or F, only have 2 digits to work with */
//                    /* round up and divide by 10 to drop the fractional digit */
//                    iTempParse += 5;                    /* add 5 to round up */
//                    iTempParse = iTempParse / 10;       /* drop decimal */
//                    
//                    ucDisplayHexValue[3] = ucDigiToHex[iTempParse%10];      /* display lsb as modulo remainder */
//                    iTempParse = iTempParse /10;        /* divide by 10 again to get msb */
//                    
//                    if (iTempParse == 0)
//                    {
//                        /* zero, so blank screen */
//                        ucDisplayHexValue[1] = 0;
//                    }
//                    else
//                    {
//                        ucDisplayHexValue[1] = ucDigiToHex[iTempParse];
//                    }
//                }
//                else
//                {
//                    /* positive value, no leading "-" */
//                    /* 0.1's place */
//                    ucDisplayHexValue[3] = ucDigiToHex[iTempParse%10];      /* display lsb as modulo remainder */
//                    /* divide by 10 for next digit */
//                    iTempParse = iTempParse / 10; 
//                    /* 1's place */
//                    ucDisplayHexValue[1] = ucDigiToHex[iTempParse%10];      /* display lsb as modulo remainder */
//                    /* decimal point */
//                    ucDisplayHexValue[1] |= 0x80;
//                    /* divide by 10 for next digit */
//                    iTempParse = iTempParse / 10; 
//                    /* 10's place */
//                    if (iTempParse == 0)
//                    {
//                        /* zero, so blank screen */
//                        ucDisplayHexValue[0] = 0;
//                    }
//                    else
//                    {
//                        ucDisplayHexValue[0] = ucDigiToHex[iTempParse];
////                        ucDisplayHexValue0Calc = ucDigiToHex[iTempParse];   
//                    }
//                }


/*4/13/17*/
/* fix bug where negative sign is displayed for 0 deg. It's actually not 0, but slightly negative by a fractional amount, but the fractional
amount gets dropped and it appears as a negative 0. Tried changing the comparison above to -4 insteat of 0, meaning < -0.4, which results in
zero (less than that gets rounded up to -1). But ran out of code space. So just re-write below, getting rid of the fractional positive values to 
streamline things. */
                int iTempParse;
                
                if (calConfigData.tempUnits == TEMP_UNITS_C)
                {
                    /* deg C */
                    iTempParse = iTemperature10xC;
                }
                else
                {
                    /* deg F */
                    iTempParse = iTemperature10xF;                    
                }
                
                
                if (iTempParse < 0)       
                {
                    iTempParse = iTempParse - 5;            /* negative, round down */
                }
                else
                {
                    iTempParse = iTempParse + 5;            /* positive, round up */
                }

                    
                /* divide by 10, drop the fractional portion */
                iTempParse = iTempParse/10;
                
                if (iTempParse < 0)
                {
                    /* negative value, so leading digit is "-" . Look up dash in symbol array*/
                    ucDisplayHexValue[0] = ucCharToHex[0]; 
                    /* from here on, it's the same regardless of polarity, soconvert the integer value to positive */
                     iTempParse = iTempParse * -1;
                }
                else
                {
                    /* positive value, so leading digit is blank */  
                    ucDisplayHexValue[0] = 0; 
                }
                  
                ucDisplayHexValue[3] = ucDigiToHex[iTempParse%10];      /* display lsb as modulo remainder */
                iTempParse = iTempParse /10;        /* divide by 10 again to get msb */ 
                if (iTempParse == 0)
                {
                    /* zero, so blank screen */
                    ucDisplayHexValue[1] = 0;
                }
                else
                {
                    ucDisplayHexValue[1] = ucDigiToHex[iTempParse];
                }   






              
                /* anode 3, turn on degree */
                //ucDisplayHexValue[2] |= 0x04;
                ucDisplayHexValue2Calc |= 0x04;
                    
//                /* turn on run mode led */
//                if (runMode == RUN_MODE_MAINTAIN)
//                {
//                    /* turn on maintain mode LED */
//                    ucDisplayHexValue[2] |= 0x20;
//                }
//                else if (runMode == RUN_MODE_TIMER)
//                {
//                    /* turn on timer mode led */
//                    ucDisplayHexValue[2] |= 0x40;
//                }
                    
                if (calConfigData.tempUnits == TEMP_UNITS_C)
                {
                    /* deg C */
                    ucDisplayHexValue[4] = ucCharToHex[1];
                }
                else
                {
                    /* deg F */
                    ucDisplayHexValue[4] = ucCharToHex[2];                    
                }

                /* if we're in run mode, automatically cycle through display values based on the timer */
                /* if program mode, states are forced in the programing state machine */
                if ( (operatingMode == MODE_AWAKE_RUN) && (uiDisplayAutoCycle == 1) )   
                {                
                    if ((uiTimerTick_1s - uiLastTickDisplay) >= DISP_RUN_SCREEN_TIME_CT)
                    {
                        uiLastTickDisplay = uiTimerTick_1s;
                        displayMode = DISP_MODE_TIME;                   
                    }  
                }              
            }
            break;
            
            case DISP_MODE_TEMP_UNITS:
            {
                ucDisplayHexValue[0] = 0;  
//                ucDisplayHexValue0Calc = 0;
                ucDisplayHexValue[1] = 0;                  
                /* anode 3, turn on degree */
                //ucDisplayHexValue[2] |= 0x04;
                ucDisplayHexValue2Calc |= 0x04;
                
                ucDisplayHexValue[3] = 0;
                if (calConfigData.tempUnits == TEMP_UNITS_C)
                {
                    /* deg C */
                    ucDisplayHexValue[4] = ucCharToHex[1];
                }
                else
                {
                    /* deg F */
                    ucDisplayHexValue[4] = ucCharToHex[2];                    
                }
            }
            break;
            
            case DISP_MODE_MAINTAIN_TIME:
            {
                /* 1st digit blank */
                ucDisplayHexValue[0] = 0; 
//                ucDisplayHexValue0Calc = 0;  
                
                /* 2nd digit set to maintain time */
                ucDisplayHexValue[1] = ucDigiToHex[calConfigData.ucMaintainTimeAfterStarTime];
                if ( (forceSettings.bits.TimedConfig == 1) || (forceSettings.bits.ReProgTimedConfig == 1) )
                {                     
                    /* forcing the timed config setting, measning we're in timed run mode */
                    /* in this case we want both the timed start led on to show we're in that mode,
                    and the maintain mode led on to show we're currently programming that setting */
                    //ucDisplayHexValue[2] = 0x60;        /* maintain mode led */
                    ucDisplayHexValue2Calc = 0x60;          /* maintain mode and timed led */
                }
                else
                {                   
                    //ucDisplayHexValue[2] = 0x40;        /* maintain mode led */
                    ucDisplayHexValue2Calc  = 0x40;        /* maintain mode led */
                }
                
                /* display "Hr" */
                ucDisplayHexValue[3] = ucCharToHex[3];
                ucDisplayHexValue[4] = ucCharToHex[4];
            }
            break;
            
//            case DISP_MODE_CAL:
//            {
//                unsigned char ucCalDigit;
//                
//                ucDisplayHexValue[0] = 0;
//                ucDisplayHexValue[1] = 0;   
////                ucDisplayHexValue0Calc = 0;                       
//                //ucDisplayHexValue[2] = 0x78;    /* turn on the 4 discrete led's to show calibration mode */
//                ucDisplayHexValue2Calc = 0x78;    /* turn on the 4 discrete led's to show calibration mode */
//                ucDisplayHexValue[3] = 0; 
//                
//                
//                if (calCalibrationData.uiTempMeasurementAdOffset < 0)
//                {
//                    /* negative value, -9 to -1 */
//                    /* negative value, so leading digit is "-" . Look up dash in symbol array*/
//                    ucDisplayHexValue[0] = ucCharToHex[0];  
////                    ucDisplayHexValue0Calc = ucCharToHex[0];          
//                    ucCalDigit =  -1 * calCalibrationData.uiTempMeasurementAdOffset;              
//                }
//                else
//                {
//                    /* zero, pos value, 0 to 9 */
//                    ucCalDigit = calCalibrationData.uiTempMeasurementAdOffset;
//                }
//                
//                /* display value */
//                ucDisplayHexValue[4] = ucDigiToHex[ucCalDigit];   
//            }
//            break;
            
            case DISP_MODE_ON:
            {
                /* 1st, 2nd digit blank */
                ucDisplayHexValue[0] = 0;   
//                ucDisplayHexValue0Calc = 0;
                ucDisplayHexValue[1] = 0;

                /* display "On" */
                ucDisplayHexValue[3] = ucCharToHex[5];
                ucDisplayHexValue[4] = ucCharToHex[6];   
                
                /* set to time so when run mode exits on state, this forces back to the rotating screens */
                displayMode = DISP_MODE_TIME;              
            }
            break;
            
            case DISP_MODE_REDY:
            {
                ucDisplayHexValue[0] = ucCharToHex[4];  //"r"  
//                ucDisplayHexValue0Calc = ucCharToHex[4];  //"r"  
                ucDisplayHexValue[1] = ucCharToHex[7];  //"E"
               
                ucDisplayHexValue[3] = ucCharToHex[8];  //"d"
                ucDisplayHexValue[4] = ucCharToHex[9];  //"y"
            }
            break;
            
            case DISP_MODE_STATS:
            {
//                unsigned char ucIndex;
                unsigned long int ulCalc;
                unsigned int uiIndex;
                unsigned char ucDecimal = 0;
                unsigned char ucDecimal2 = 0;
                
//                if (statDispMode == STAT_DISP_RELAY)
//                {
//                    if (calStatData.fltTotalNumRelayCycles <= 999900)
//                    {
//                        ucDecimal = 0x80;
//                        /* divide by a hundred so we are in terms of 1,000's, but capture 1 decimal place */
//                        ulCalc = (unsigned long) calStatData.fltTotalNumRelayCycles/100;
//                    }
//                    else if (calStatData.fltTotalNumRelayCycles <= 9999000)
//                    {
//                        /* divide by 1000 so we are in terms of 1,000's, no decimal place */
//                        ulCalc = (unsigned long) calStatData.fltTotalNumRelayCycles/1000;
//                    }    
//                    else
//                    {    
//                        /* very high value, clamp, no decimal place */
//                        ulCalc = 9999;
//                    }
//                    
//                    
//                    uiIndex = (unsigned int) ulCalc%10;
//                    ucDisplayHexValue[4] = ucDigiToHex[uiIndex]; 
//                    
//                    /* 1's place */
//                    ulCalc = ulCalc/10;
//                    uiIndex = (unsigned int) ulCalc%10;
//                    ucDisplayHexValue[3] = ucDigiToHex[uiIndex] | ucDecimal;         /* add decimal point */
//                    
//                    
//                    /* 10's place */
//                    ulCalc = ulCalc/10;
//                    uiIndex = (unsigned int) ulCalc%10;
//                    ucDisplayHexValue[1] = ucDigiToHex[uiIndex];     
//                    
//                    /* 100's place */    
//                    ulCalc = ulCalc/10;           
//                    uiIndex = (unsigned int) ulCalc;
//                    ucDisplayHexValue[0] = ucDigiToHex[uiIndex];   
//                }
//                else
//                {
//                    /* 2 decimal places, or max 99,990 hours, or 5999400 minutes */
//                    if (calStatData.fltTotalNumMinRunRelayOff <= 5999400)
//                    {
//                        ucDecimal2 = 0x80;
//                        /* divide by 10 so we are in terms of 1,000's, but capture 2 decimal place. Also convert minutes to hours, effectively divide by 600 */
//                        ulCalc = (unsigned long) calStatData.fltTotalNumMinRunRelayOff/600;
//                    }
//                    else /* 1 decimal place, or max 999,900 hours, or 59,994,000 minutes */
//                    {
//                        ucDecimal = 0x80;
//                        /* divide by 100 so we are in terms of 1,000's, but capture 1 decimal place. Also convert minutes to hours, effectively divide by 6000 */
//                        ulCalc = (unsigned long) calStatData.fltTotalNumMinRunRelayOff/6000;
//                    }    
//                    
//                    uiIndex = (unsigned int) ulCalc%10;
//                    ucDisplayHexValue[4] = ucDigiToHex[uiIndex]; 
//                    
//                    /* 1's place */
//                    ulCalc = ulCalc/10;
//                    uiIndex = (unsigned int) ulCalc%10;
//                    ucDisplayHexValue[3] = ucDigiToHex[uiIndex] | ucDecimal;         /* add decimal point */
//                    
//                    
//                    /* 10's place */
//                    ulCalc = ulCalc/10;
//                    uiIndex = (unsigned int) ulCalc%10;
//                    ucDisplayHexValue[1] = ucDigiToHex[uiIndex]| ucDecimal2;         /* add decimal point */;     
//                    
//                    /* 100's place */    
//                    ulCalc = ulCalc/10;           
//                    uiIndex = (unsigned int) ulCalc;
//                    ucDisplayHexValue[0] = ucDigiToHex[uiIndex];                      
//                }
//                
                
                
                if (statDispMode == STAT_DISP_RELAY)
                {
                    if (calStatData.fltTotalNumRelayCycles <= 999900)
                    {
                        ucDecimal = 0x80;
                        /* divide by a hundred so we are in terms of 1,000's, but capture 1 decimal place */
                        ulCalc = (unsigned long) calStatData.fltTotalNumRelayCycles/100;
                    }
                    else if (calStatData.fltTotalNumRelayCycles <= 9999000)
                    {
                        /* divide by 1000 so we are in terms of 1,000's, no decimal place */
                        ulCalc = (unsigned long) calStatData.fltTotalNumRelayCycles/1000;
                    }    
                    else
                    {    
                        /* very high value, clamp, no decimal place */
                        ulCalc = 9999;
                    }
                }  
                else
                {
                    /* 2 decimal places, or max 99,990 hours, or 5999400 minutes */
                    if (calStatData.fltTotalNumMinRunRelayOff <= 5999400)
                    {
                        ucDecimal2 = 0x80;
                        /* divide by 10 so we are in terms of 1,000's, but capture 2 decimal place. Also convert minutes to hours, effectively divide by 600 */
                        ulCalc = (unsigned long) calStatData.fltTotalNumMinRunRelayOff/600;
                    }
                    else /* 1 decimal place, or max 999,900 hours, or 59,994,000 minutes */
                    {
                        ucDecimal = 0x80;
                        /* divide by 100 so we are in terms of 1,000's, but capture 1 decimal place. Also convert minutes to hours, effectively divide by 6000 */
                        ulCalc = (unsigned long) calStatData.fltTotalNumMinRunRelayOff/6000;
                    }                        
                }
                                   
                uiIndex = (unsigned int) ulCalc%10;
                ucDisplayHexValue[4] = ucDigiToHex[uiIndex]; 
                
                /* 1's place */
                ulCalc = ulCalc/10;
                uiIndex = (unsigned int) ulCalc%10;
                ucDisplayHexValue[3] = ucDigiToHex[uiIndex] | ucDecimal;         /* add decimal point */
                             
                /* 10's place */
                ulCalc = ulCalc/10;
                uiIndex = (unsigned int) ulCalc%10;
                ucDisplayHexValue[1] = ucDigiToHex[uiIndex] | ucDecimal2;         /* add decimal point */     
                
                /* 100's place */    
                ulCalc = ulCalc/10;           
                uiIndex = (unsigned int) ulCalc;
                ucDisplayHexValue[0] = ucDigiToHex[uiIndex];                   
                
            }
            break;
        }
        
        /* move the temporary value for anode 3 into the display array */
        /* temporary value use to fix flicker issues */
//        ucDisplayHexValue[0] = ucDisplayHexValue0Calc;
        ucDisplayHexValue[2] = ucDisplayHexValue2Calc;
         
               
    }
}

void UpdateDisplay(void )
/*******************************************************************************
* DESCRIPTIION: Services all of the raw display LED's 
*               During initial testing, with basically no other code running,
*               this routine hits every 37us.
*
*               9/1/16 - routine take 6.4us - 9.6us to execute      
*               going to move into interrupts
*
* INPUTS: None
*
* RETURNS: None
*
*******************************************************************************/
{
    static unsigned int uiLastTick = 0;

//P1OUT |= DEBUG_P15;

///* DEBUG */
//if (ucDisplayHexValue[2] != 0)
//{
//    P1OUT &= ~DEBUG_P15;
//}
//else
//{
//   P1OUT |= DEBUG_P15; 
//}

/* 01/10/17 - display still occationally shows digit on power down or up. Force to init mode
if we go into sleep mode */
    if (operatingMode == MODE_SLEEP)
    {
        displayState = DISP_STATE_INIT;
    }
    
    /* display update state machine */
    switch (displayState)
    {
        case DISP_STATE_INIT:
        default:
        {
            /* init led's off so when we wake they're off */
            P1OUT &= ~ (A1_CTRL | A2_CTRL | A3_CTRL | A4_CTRL | A5_CTRL);
            P3OUT &= ~(A_CTRL | B_CTRL | C_CTRL | D_CTRL | E_CTRL | F_CTRL | G_CTRL | DP_CTRL);
            uiLastTick = uiTimerTick_125us;
            displayState = DISP_STATE_A1;
        }
        break;
        
        case DISP_STATE_A1:      /* anode 1, this is the 7-segment MSB, digit 1 */
        {  
            /* must wait a bit before turning off the anode in the last state to turning on here, because if we don't we get 
            "bleed over" from the next states's cathode setting while still on the last states anode */
            /* this will add 250us delay */
#ifdef DIMMABLE
            if ((uiTimerTick_125us - uiLastTick) >= ucDigitOffTimeCt)
#else
            if ((uiTimerTick_125us - uiLastTick) >= DISP_DIGIT_OFF_TIME_CT)
#endif
            {
                /* set cathodes */
                P3OUT = ucDisplayHexValue[0];                
                /* anode 1 */
                P1OUT |= A1_CTRL; 
            }
            
            if ((uiTimerTick_125us - uiLastTick) >= DISP_DIGIT_ON_TIME_CT)
            {
                /* go to next digit */
                uiLastTick = uiTimerTick_125us;
                displayState = DISP_STATE_A2;

                /* turn off anode 1 */
                P1OUT &= ~A1_CTRL;             
            }
        }
        break;
        
        case DISP_STATE_A2:      /* anode 2, this is the 7-segment digit 2 */   
        {
            /* must wait a bit before turning off the anode in the last state to turning on here, because if we don't we get 
            "bleed over" from the next states's cathode setting while still on the last states anode */
            /* this will add 250us delay */
#ifdef DIMMABLE
            if ((uiTimerTick_125us - uiLastTick) >= ucDigitOffTimeCt)
#else
            if ((uiTimerTick_125us - uiLastTick) >= DISP_DIGIT_OFF_TIME_CT)
#endif
            {
                /* set cathodes */
                P3OUT = ucDisplayHexValue[1];                
                /* anode 2 */
                P1OUT |= A2_CTRL;                
            }
            
            
            if ((uiTimerTick_125us - uiLastTick) >= DISP_DIGIT_ON_TIME_CT)
            {
                /* turn off anode 2 */
                P1OUT &= ~A2_CTRL;
                
                /* go to next digit */
                uiLastTick = uiTimerTick_125us;
                displayState = DISP_STATE_A3;
            }            
        }
        break;
        
          
        case DISP_STATE_A3:      /* anode 3, this is the 7-segment colon, degree symbol, and discrete LED's */
        {
            /* must wait a bit before turning off the anode in the last state to turning on here, because if we don't we get 
            "bleed over" from the next states's cathode setting while still on the last states anode */
            /* this will add 250us delay */
#ifdef DIMMABLE
            if ((uiTimerTick_125us - uiLastTick) >= ucDigitOffTimeCt)
#else
            if ((uiTimerTick_125us - uiLastTick) >= DISP_DIGIT_OFF_TIME_CT)
#endif
            {
                /* set cathodes */             
                P3OUT = ucDisplayHexValue[2];    
/* DEBUG FLICKER */
//P3OUT = 0X04;           
                /* anode 3 */
                P1OUT |= A3_CTRL;  
            }
            
            if ((uiTimerTick_125us - uiLastTick) >= DISP_DIGIT_ON_TIME_CT)
            {
                /* go to next digit */
                uiLastTick = uiTimerTick_125us;
                displayState = DISP_STATE_A4;
                
                /* turn off anode 3 */
                P1OUT &= ~A3_CTRL;
            }            
        }
        break;
        
        case DISP_STATE_A4:      /* anode 4, this is the 7-segment digit 3 */
        {
            /* must wait a bit before turning off the anode in the last state to turning on here, because if we don't we get 
            "bleed over" from the next states's cathode setting while still on the last states anode */
            /* this will add 250us delay */
#ifdef DIMMABLE
            if ((uiTimerTick_125us - uiLastTick) >= ucDigitOffTimeCt)
#else
            if ((uiTimerTick_125us - uiLastTick) >= DISP_DIGIT_OFF_TIME_CT)
#endif
            { 
                /* set cathodes */
                P3OUT = ucDisplayHexValue[3];         
                /* anode 4 */
                P1OUT |= A4_CTRL;                              
            }
            
            if ((uiTimerTick_125us - uiLastTick) >= DISP_DIGIT_ON_TIME_CT)
            {
                /* go to next digit */
                uiLastTick = uiTimerTick_125us;
                displayState = DISP_STATE_A5;
                
                /* turn off anode 4 */
                P1OUT &= ~A4_CTRL;
            }            
        }
        break;
        
        case DISP_STATE_A5:       /* anode 5, this is the 7-segment LSB, digit 4 */
        {
            /* must wait a bit before turning off the anode in the last state to turning on here, because if we don't we get 
            "bleed over" from the next states's cathode setting while still on the last states anode */
            /* this will add 250us delay */
#ifdef DIMMABLE
            if ((uiTimerTick_125us - uiLastTick) >= ucDigitOffTimeCt)
#else
            if ((uiTimerTick_125us - uiLastTick) >= DISP_DIGIT_OFF_TIME_CT)
#endif
            {  
                /* set cathodes */
                P3OUT = ucDisplayHexValue[4];                      
                /* anode 5 */
                P1OUT |= A5_CTRL;                    
            }
            
            if ((uiTimerTick_125us - uiLastTick) >= DISP_DIGIT_ON_TIME_CT)
            {
                /* go to next digit */
                uiLastTick = uiTimerTick_125us;
                displayState = DISP_STATE_A1;
                
                /* turn off anode 5 */
                P1OUT &= ~A5_CTRL;
            }            
        }
        break;
    }
    
//P1OUT &= ~DEBUG_P15;    
    
}




//void UpdateClockTimeFromStart(void )
///*******************************************************************************
//* DESCRIPTIION: calculates the number of minutes away from the start time
//*               calculating times is a bit confusing. Maybe would have been better
//*               to do military time, but that would have require it's own 
//*               conversions. Refer to "time remaining calculations.xlsx" for
//*               sample calculations used to derive this function logic
//*
//* INPUTS: None
//*
//* RETURNS: None
//*
//*******************************************************************************/
//{  
//    unsigned char ucTimeAwayMinutes = 0;
//    unsigned char ucTimeAwayHours = 0;
//    unsigned char ucBorrowHour = 0;
//     
//    /* calulate minutes */ 
//    if (ucTimeMinutes <= calConfigData.ucStartTimeMinutes)
//    {
//        ucTimeAwayMinutes = (calConfigData.ucStartTimeMinutes - ucTimeMinutes);
//    }
//    else
//    {
//        /* borrow an hour */
//        ucTimeAwayMinutes = (60 + calConfigData.ucStartTimeMinutes - ucTimeMinutes);
//        ucBorrowHour = 1;
//    }
//    
//    /* calculate hours */
//    /* special cases with either time value of 12 */
//    if (calConfigData.ucStartTimeHours == 12)
//    {
//        /* Both current time and start time are 12 something *****************************/
//        if (ucTimeHours == 12)   
//        {
//            if (calConfigData.ucStartTimePm == ucTimePm)
//            {
//                if (ucBorrowHour == 1)    
//                {
//                    ucTimeAwayHours = 23;       /* current time greater than start time, so next day */
//                }
//                else
//                {
//                    ucTimeAwayHours = 0;        /* no full hours, just minutes */
//                }
//            }
//            else
//            {
//                if (ucBorrowHour == 1)    
//                {
//                    ucTimeAwayHours = 11;     
//                }
//                else
//                {
//                    ucTimeAwayHours = 12;
//                }                
//            }
//        }
//        else
//        {
//            /* start time 12 something, current time is something else *************************/
//            if (calConfigData.ucStartTimePm == ucTimePm)
//            {
//                /* same am/pm, so another 12 hours away */
//                ucTimeAwayHours = calConfigData.ucStartTimeHours - ucTimeHours - ucBorrowHour + 12;
//            }
//            else
//            {   
//                ucTimeAwayHours = calConfigData.ucStartTimeHours - ucTimeHours - ucBorrowHour;
//            }            
//        }        
//    }
//    else /* start time is not 12 */
//    {
///*4/25/17*/
///* the math in this case isn't correct */
////        if (ucTimeHours == 12)   
////        {
////            if (calConfigData.ucStartTimePm == ucTimePm)
////            {
////                ucTimeAwayHours = ucTimeHours - calConfigData.ucStartTimeHours - ucBorrowHour;
////            }
////            else
////            {
////                /* different am/pm, so another 12 hours away */
////                ucTimeAwayHours = ucTimeHours - calConfigData.ucStartTimeHours - ucBorrowHour + 12;
////            }            
////        }
//        if (ucTimeHours == 12)   
//        {
//            /* if time is 12:00, treat the numeric start hour as the number of hours directly */
//            if (calConfigData.ucStartTimePm == ucTimePm)
//            {
//                ucTimeAwayHours = calConfigData.ucStartTimeHours - ucBorrowHour;
//            }
//            else
//            {
//                /* different am/pm, so another 12 hours away */
//                ucTimeAwayHours = calConfigData.ucStartTimeHours - ucBorrowHour + 12;
//            }            
//        }        
//        else
//        {
//            /* neither time is 12 */
//            if (ucTimeHours > calConfigData.ucStartTimeHours)
//            {                          
//                if (calConfigData.ucStartTimePm == ucTimePm)
//                {
//                    /* same am/pm, so another 12 hours away */
//                    ucTimeAwayHours = 24 - ucTimeHours + calConfigData.ucStartTimeHours - ucBorrowHour;
//                }
//                else
//                {
//                    ucTimeAwayHours = 12 - ucTimeHours + calConfigData.ucStartTimeHours - ucBorrowHour;
//                }          
//            }
//            else    /* time < start */
//            {
//                if (calConfigData.ucStartTimePm == ucTimePm)
//                {        
//                    ucTimeAwayHours = calConfigData.ucStartTimeHours - ucTimeHours - ucBorrowHour;
//                }
//                else
//                {
//                    /* different am/pm, so another 12 hours away */
//                    ucTimeAwayHours = calConfigData.ucStartTimeHours - ucTimeHours - ucBorrowHour + 12;
//                }           
//            } 
//            
//        }
//    }
//    
//
//    /* minutes remaining */
//    fltClockTimeAwayFromStart = ucTimeAwayMinutes + (ucTimeAwayHours*60);
//}

/*4/26/17*/
void UpdateClockTimeFromStart(void )
/*******************************************************************************
* DESCRIPTIION: calculates the number of minutes away from the start time
*               4/26/17 - original verion above didn't work right in some special case, 
*               so completely re-write and 
*               make calculations all relative to 12am. This steamlines 
*               the math and reduces the special cases
*
* INPUTS: None
*
* RETURNS: None
*
*******************************************************************************/
{   
    unsigned int uiTotalMinutesStartTime;
    unsigned int uiTotalMinutesClockTime;
    unsigned char ucTimeMinutesLocal;
    
    /* convert the times, both start time, and clock time, into minutes relative to 12:00am */
    /* start time */
    if (calConfigData.ucStartTimeHours < 12)
    {
        uiTotalMinutesStartTime =  (calConfigData.ucStartTimeHours * 60) +  calConfigData.ucStartTimeMinutes;
    }
    else
    {
        /* value of 12 hours really means we're less than 1 hours, so it's just equal to minutes */
        uiTotalMinutesStartTime = calConfigData.ucStartTimeMinutes;              
    }
    
    if (calConfigData.ucStartTimePm  == 1)
    {
        /* if pm flag is set, we're another 12 hours aways from 12 am, or 720 minutes */
        uiTotalMinutesStartTime += 720;
    }
    
    /* clock time */
    /* get local copy to get rid of compiler warning */
    ucTimeMinutesLocal = ucTimeMinutes;
    if (ucTimeHours < 12)
    {    
        uiTotalMinutesClockTime =  (ucTimeHours * 60) +  ucTimeMinutesLocal;
    }
    else
    {
        /* value of 12 hours really means we're less than 1 hours, so it's just equal to minutes */
        uiTotalMinutesClockTime = ucTimeMinutesLocal;              
    }
    
    if (ucTimePm  == 1)
    {
        /* if pm flag is set, we're another 12 hours aways from 12 am, or 720 minutes */
        uiTotalMinutesClockTime += 720;
    }    
    
    
    
    /* now calculate time remaining */
    if (uiTotalMinutesClockTime > uiTotalMinutesStartTime)
    {
        /* minutes remaining */
        /* 24 hours - (clock - start) */
        fltClockTimeAwayFromStart = 1440 - uiTotalMinutesClockTime + uiTotalMinutesStartTime;       
    }
    else
    {
        /* minutes remaining */
        /* start - clock */
        fltClockTimeAwayFromStart =  uiTotalMinutesStartTime -  uiTotalMinutesClockTime;      
    }
    
    
}



void MaintainModeControl(void )
/*******************************************************************************
* DESCRIPTIION: Controls the timing in maintain mode, relay on and off. Used by 
*               both timed start and continous maintain mode
*
* INPUTS: None
*
* RETURNS: None
*
*******************************************************************************/
{   
    if (relayState == RELAY_OFF)
    {
        /* relay currently off */
//        if ( (uiTimerTick_1min - uiRelayLastTick) >= fltRelayTimeMin )   
        if ( (uiTimerTick_1s - uiRelayLastTick) >= fltRelayTimeSec )           
        {          
            /* not the first on-time for maintain mode, use the normal maintain on-time */
//            fltRelayTimeMin = Interp2D (fltTemperature, (float*) &fltMaintainTemp[0], (float*) &fltMaintainOnTime[0], INTERP_MAINTAIN_LENGTH );  
            fltRelayTimeSec = Interp2D (fltTemperature, (float*) &fltMaintainTemp[0], (float*) &fltMaintainOnTime[0], INTERP_MAINTAIN_LENGTH );       
//            uiRelayLastTick = uiTimerTick_1min;
            uiRelayLastTick = uiTimerTick_1s;
//            relayState = RELAY_ON;
//            if (fltRelayTimeMin > 0)
            if (fltRelayTimeSec > 0)
            {
                /* only turn relay on if on-time is non-zero */
                relayState = RELAY_ON;
            }
            else
            {
                /* refresh the off time */
//                fltRelayTimeMin = Interp2D (fltTemperature, (float*) &fltMaintainTemp[0], (float*) &fltMaintainOffTime[0], INTERP_MAINTAIN_LENGTH ); 
                fltRelayTimeSec = Interp2D (fltTemperature, (float*) &fltMaintainTemp[0], (float*) &fltMaintainOffTime[0], INTERP_MAINTAIN_LENGTH ); 
            }
        }
    }
    else
    {
        /* relay currently on */   
//        if ( (uiTimerTick_1min - uiRelayLastTick) >= fltRelayTimeMin )
        if ( (uiTimerTick_1s - uiRelayLastTick) >= fltRelayTimeSec )
        {
            /* get new off-time */
//            fltRelayTimeMin = Interp2D (fltTemperature, (float*) &fltMaintainTemp[0], (float*) &fltMaintainOffTime[0], INTERP_MAINTAIN_LENGTH );  
            fltRelayTimeSec = Interp2D (fltTemperature, (float*) &fltMaintainTemp[0], (float*) &fltMaintainOffTime[0], INTERP_MAINTAIN_LENGTH );  
//            uiRelayLastTick = uiTimerTick_1min;
            uiRelayLastTick = uiTimerTick_1s;
            relayState = RELAY_OFF;
            
            /* regardless of which off-cycle we're in, we want to display ready after the first full on-cycle, so set flag here */
            ucDisplayReady = 1;
        }                
    }   
    
    
    /* in maintain mode, alternate display between measured temperature and "rEdy" when ready */
    /* if not ready, just display temperature */
    if ((uiTimerTick_1s - uiLastTickDisplay) >= DISP_RUN_SCREEN_TIME_CT)
    {
        uiLastTickDisplay = uiTimerTick_1s;
        
        if ( (displayMode == DISP_MODE_TEMP) &&  (ucDisplayReady == 1) )   
/*DEBUG*/
//        if ( (displayMode == DISP_MODE_TEMP))   
        {
            displayMode = DISP_MODE_REDY;
        }
        else
        {
            displayMode = DISP_MODE_TEMP;   
        }
    }
     
}

void RelayControl(void )
/*******************************************************************************
* DESCRIPTIION: Controls relay. This is the main controller code
*
* INPUTS: None
*
* RETURNS: None
*
*******************************************************************************/
{     
    static RunMode runModeLast;
    static RelayState relayStateLast;
    static unsigned int uiLastTick = 0;
    static unsigned int uiRelayOffLastTick = 0;
    static unsigned int uiRelayOffTick_1s = 0;          /* seconds counter for relay off */
            
    
    /* only run when we're not in program mode */
    if (operatingMode == MODE_AWAKE_RUN)
    {
        if (runMode != runModeLast)
        {
            /* chaned run mode, so set relay mode to the same */
            runModeRelay = runMode;
            
            if (runModeRelay == RUN_MODE_MAINTAIN)
            {
                /*  it's the first on-time, make it longer, equal to the timed start time */
                /* per Jeff 7/29/16, limit this to 3.5 hours */
                fltRelayTimeMin = Interp2D (fltTemperature, (float*) &fltTimedTemp[0], (float*) &fltTimedTime[0], INTERP_TIMED_LENGTH );

                if (fltRelayTimeMin > RELAY_ON_TIME_1ST_CYCLE_MAINTAIN_MAX)
                {
                    fltRelayTimeMin = RELAY_ON_TIME_1ST_CYCLE_MAINTAIN_MAX;
                }
                
                /* 1/10/17 change maintain times from minutes to seconds */
                fltRelayTimeSec = fltRelayTimeMin * 60.0;
                
//                uiRelayLastTick = uiTimerTick_1min;     /* timer for maintain mode on/off, in minutes */
                uiRelayLastTick = uiTimerTick_1s;     /* timer for maintain mode on/off, in minutes */
                uiLastTick = uiTimerTick_1s;            /* timer for 10 second delay */
                //relayState = RELAY_ON;      
                maintainModeRelayState = MAINTAIN_MODE_RELAY_DISABLED;      /* keep the relay off for 10 seconds initially per Jeff 8/29/16 */      
            }
            else if (runModeRelay == RUN_MODE_TIMER)
            {
                /*starting in timed mode, init parameters */   
                timerModeRelayState = TIMER_MODE_RELAY_OFF;     /* init timer mode state machine */
            }
        }
        
//        if ( (runMode == RUN_MODE_MAINTAIN) && (runModeLast != RUN_MODE_MAINTAIN) )
//        {
//            /*starting in maintain mode, init parameters */                                                                                                              
//            fltRelayTimeMin = Interp2D (fltTemperature, (float*) &fltMaintainTemp[0], (float*) &fltMaintainOnTime[0], INTERP_MAINTAIN_LENGTH );  
//            uiLastTick = uiTimerTick_1min;
//            relayState = RELAY_ON;
//        }   
        
        switch (runModeRelay)
        {
            case RUN_MODE_OFF:
            default:
            {
                relayState = RELAY_OFF;
                uiDisplayAutoCycle = 1;         /* allow display to auto cycle through screens */
            }
            break;
            
            case RUN_MODE_TIMER:
            {
                switch (timerModeRelayState)
                {
                    case TIMER_MODE_RELAY_OFF:
                    default:
                    {
                        relayState = RELAY_OFF;
                        
                        uiDisplayAutoCycle = 1;         /* allow display to auto cycle through screens */
                        
                        /* when the clock time remaining before the start time is less than or equal to the 
                        time calculate based on temperature, kick on relay */
                        if (fltClockTimeAwayFromStart <= fltTemperatureTimeAwayFromStart)
                        {
                            if (fltTemperatureTimeAwayFromStart == 0)
                            {
                                /* if the calculated time is zero, skip the on state to avoid briefly turning relay on */
                                timerModeRelayState = TIMER_MODE_RELAY_MAINTAIN;    
                                relayState = RELAY_OFF;
                                fltRelayTimeMin = Interp2D (fltTemperature, (float*) &fltMaintainTemp[0], (float*) &fltMaintainOffTime[0], INTERP_MAINTAIN_LENGTH );
                                uiRelayLastTick = uiTimerTick_1min;
                                
/*srs*/
/*03/29/17 turn on maintain mode led (along with timed led, which is already on) when we go to maintain mode*/
/* also set lasttick to prevent bouncing back and forth */
                                discreteLed.bits.MaintainMode = 1;                                
//                                uiLastTick = uiTimerTick_1min;          /* this ticker used to determine when to exit maintain mode */
/* move to dedicated ticker. check outside this function so it can be done on battery as well as powered */
                                uiLastTickMaintain = uiMaintainTimeTick_1min;
                            }
                            else
                            {
                                unsigned char ucTimeCalculation;
                                unsigned int uiRealTimeMinutesToStart;
                                
                                relayState = RELAY_ON;
                                timerModeRelayState = TIMER_MODE_RELAY_ON;
                                ucCancelTimedStart = 0;     /* reset cancel flags */
                                ucCancelMaintain = 0;
                                
/*3/31/17*/
                                /* calculate the clock time to stop timed start mode. In most cases this should just come out to be the same
                                as the start time in the config. The exception is if we enter this mode without enough time per the temperature table.
                                In that case, the start time will be pushed out from the configured start time */
                                /* calculate hours from now */
                                uiTimeCalculation = (unsigned int) fltTemperatureTimeAwayFromStart/60;   
                                ucTimeCalculation = (unsigned char) uiTimeCalculation;
                                timedStartConfig.ucStartTimeHours = ucTimeHours + ucTimeCalculation;
                                /* calculate minutes from now */
                                uiTimeCalculation = (unsigned int) fltTemperatureTimeAwayFromStart%60;
                                ucTimeCalculation = (unsigned char) uiTimeCalculation; 
                                timedStartConfig.ucStartTimeMinutes = ucTimeMinutes + ucTimeCalculation;
                                
/*4/19/17*/
/* correct calculation for minutes > 60 */                               
//                                /* if minutes = 60, bump hour and zero minutes */
//                                if (timedStartConfig.ucStartTimeMinutes == 60)
//                                {
//                                    timedStartConfig.ucStartTimeHours++;
//                                    timedStartConfig.ucStartTimeMinutes = 0;
//                                    
//                                }

                                /* if minutes >= 60, bump hour and calculate minutes */
                                if (timedStartConfig.ucStartTimeMinutes >= 60)
                                {
                                    timedStartConfig.ucStartTimeHours++;
                                    timedStartConfig.ucStartTimeMinutes = timedStartConfig.ucStartTimeMinutes - 60;
                                    
                                }
                                
                                /* set pm flag to current time */
                                timedStartConfig.ucStartTimePm = ucTimePm;

/*4/25/17 - doesn't handle correctly */                                
//                                /* handling if hours have rolled over 12:00 */
//                                if (timedStartConfig.ucStartTimeHours > 12)
//                                {
//                                    timedStartConfig.ucStartTimeHours = timedStartConfig.ucStartTimeHours - 12;
//                                    
//                                    /* flip pm flag */
//                                    timedStartConfig.ucStartTimePm ^= 1;
//                                }




                                /* handling if hours have rolled over from 12:59 */
                                if (timedStartConfig.ucStartTimeHours > 12)
                                {
                                    timedStartConfig.ucStartTimeHours = timedStartConfig.ucStartTimeHours - 12;                                   
                                }     
                                
                                 
                                /* handling of PM flag */
                                /* calculate the real time number of minutes left until start, meaning factor in the real time clock value
                                past 12:00. Total 12 hour period = 720 hours. So if the total sum is >= 720, need to flip pm bit */
                                uiRealTimeMinutesToStart = (unsigned int) fltTemperatureTimeAwayFromStart;
                                uiRealTimeMinutesToStart += ucTimeMinutes; 
                                /* if hours are something other than 12, calculate the addtional minutes */
                                if (ucTimeHours < 12)
                                {
                                    /* use temporary variable because compiler may treat as unsigned char, and the value can be > 255 */
                                    unsigned int uiRealTimeMinutesToStart_Hours;
                                    
                                    uiRealTimeMinutesToStart_Hours = (ucTimeHours*60); 
                                    uiRealTimeMinutesToStart += uiRealTimeMinutesToStart_Hours;
                                }
                                if (uiRealTimeMinutesToStart >= 720)
                                {
                                    /* flip pm flag */
                                    timedStartConfig.ucStartTimePm ^= 1;                                    
                                }
                                
                                
                                                          
                            }
                        }
                    }
                    break;
                    
                    case TIMER_MODE_RELAY_ON:
                    {
                        if (ucCancelTimedStart == 1)    /* cancel flag is done in main loop. Done there so the real time can be monitored with loss of ac power */
//                        if (fltClockTimeAwayFromStart == 0)
                        {
                            /* done, now go to maintain mode */
                            timerModeRelayState = TIMER_MODE_RELAY_MAINTAIN;    
/*srs*/
/*03/29/17 turn on maintain mode led (along with timed led, which is already on) when we go to maintain mode*/
                            discreteLed.bits.MaintainMode = 1;
                            
                            /*starting in maintain mode, init parameters */   
                            /* in this case, we start in the off state */                                                                                                           
//                            fltRelayTimeMin = Interp2D (fltTemperature, (float*) &fltMaintainTemp[0], (float*) &fltMaintainOnTime[0], INTERP_MAINTAIN_LENGTH );  
                            fltRelayTimeSec = Interp2D (fltTemperature, (float*) &fltMaintainTemp[0], (float*) &fltMaintainOffTime[0], INTERP_MAINTAIN_LENGTH );  
//                            uiRelayLastTick = uiTimerTick_1min;     /* this ticker used for maintain mode control */
                            uiRelayLastTick = uiTimerTick_1s;     /* this ticker used for maintain mode control */
                            uiLastTick = uiTimerTick_1min;          /* this ticker used to determine when to exit maintain mode */
/* move to dedicated ticker. check outside this function so it can be done on battery as well as powered */
                            uiLastTickMaintain = uiMaintainTimeTick_1min;                            
//                            relayState = RELAY_ON;  
                            relayState = RELAY_OFF;    
                        } 
                        else
                        {
                            relayState = RELAY_ON;
                            uiDisplayAutoCycle = 1;         /* allow display to auto cycle through screens */                   
                        }
                    }
                    break;
                    
                   case TIMER_MODE_RELAY_MAINTAIN:
                   
                   {
                        uiDisplayAutoCycle = 0;         /* do not allow display to auto cycle through screens, maintain mode handles it  */
                        ucDisplayReady = 1;             /* display "redy" since warm up time has completed */
                        
                        MaintainModeControl();
                       
                        /* after configured maintain mode time, turn off and get ready for the next day */
                     
//                        if ( (uiTimerTick_1min - uiLastTick) >= (calConfigData.ucMaintainTimeAfterStarTime*60) )
/*3/30/17 - check to cancel outside this function so it can be updated even on battery power */
                        if (ucCancelMaintain == 1)
/*DEBUG*/   
//                        if ( (uiTimerTick_1min - uiLastTick) >= (calConfigData.ucMaintainTimeAfterStarTime*2) )
                        {
                            timerModeRelayState = TIMER_MODE_RELAY_OFF;
                            relayState = RELAY_OFF;    
                            ucDisplayReady = 0;             /* turn off "redy" */
                            displayMode = DISP_MODE_INIT;   /* reset display mode to cycle times and temp */
                            
/*srs*/
/*03/29/17 turn off maintain mode led when we time out of maintain mode*/
                            discreteLed.bits.MaintainMode = 0;                            
                        }
                   }
                   break;   
                }
            }
            break;
            
            case RUN_MODE_MAINTAIN:
            {
                uiDisplayAutoCycle = 0;         /* do not allow display to auto cycle through screens, maintain mode handles it  */ 
                
                switch (maintainModeRelayState)
                {
                    case MAINTAIN_MODE_RELAY_DISABLED:
                    {
                        relayState = RELAY_OFF;     /* keep relay off for 10 seconds */
                        
                        /* delay turning on relay initially for 10 seconds */
                        if ( (uiTimerTick_1s - uiLastTick) >= MAINTAIN_MODE_DELAY_CT)
                        {
                            /* waited 10 seconds, now kick in to maintain mode, turn relay on */
                            maintainModeRelayState = MAINTAIN_MODE_RELAY_ENABLED; 
//                            uiRelayLastTick = uiTimerTick_1min;     /* timer for maintain mode on/off, in minutes */
                            uiRelayLastTick = uiTimerTick_1s;     /* timer for maintain mode on/off, in seconds */
                            /* only turn relay on if cycle time is non-zero */
                            if (fltRelayTimeMin != 0)
                            {
                                relayState = RELAY_ON;
                            }
                            else
                            {
                                relayState = RELAY_OFF;  
                                /* on-time is zero, meaning it's too warm, so display redy now */
                                ucDisplayReady = 1;
                            }
                        }
                    }
                    break;
                    
                    case MAINTAIN_MODE_RELAY_ENABLED:
                    {   
                        /* allow maintain mode control function control relay state */
                        MaintainModeControl();
                    }
                    break;                   
                }
            }
            break;
            
            case RUN_MODE_ON:
            {
                relayState = RELAY_ON;   
                uiDisplayAutoCycle = 0;         /* do not allow display to auto cycle through screens, maintain mode handles it  */ 
                displayMode = DISP_MODE_ON;
            }
            break;
        }
        
        if (relayState == RELAY_ON)
        {
            P1OUT |= RELAY;   
            if (relayStateLast == RELAY_OFF)
            {   
                calStatData.fltTotalNumRelayCycles++;           /* count number of relay actuations */
            }
            uiRelayOffLastTick = uiTimerTick_250us;                /* zero off timer */
        }
        else
        {  
            P1OUT &= ~RELAY;   
            
            /* count the number of minutes we are powered and in a run mode, but not energizing relay, i.e. energy savings */ 
            if (runModeRelay != RUN_MODE_OFF)
            {
                if ( (uiTimerTick_250us - uiRelayOffLastTick) >= 4000)
                { 
                    /* bump relay off ticker */
                    uiRelayOffTick_1s++;
                    uiRelayOffLastTick = uiTimerTick_250us;
                }
               
                if (uiRelayOffTick_1s >= 60)
                {
                    /* bump 1 minute relay off ticker, zero second ticker */
                    uiRelayOffTick_1s = 0;
                    calStatData.fltTotalNumMinRunRelayOff++;
                }
            }
            else
            {
                uiRelayOffLastTick = uiTimerTick_250us;
            }
        }  
        
        runModeLast = runMode;              /* refresh last state */
    }
    else
    {
        /* not in run mode, turn relay off */
        P1OUT &= ~RELAY;
        runModeLast = RUN_MODE_OFF;
    }
    
    relayStateLast = relayState;
    
}

void CheckToGoToSleep(void )
/*******************************************************************************
* DESCRIPTIION: Checks to see if AC power is present. Go to sleep if not. 
*               NOTE: operatingMode will stay set to sleep until AC power is 
*               plugged back in because of the way the 'if' branch is constructed
*               Therefore, when we are running on battery, the hardware will wake 
*               up once a second via the RTC interrupt, but the operatingMode will
*               remain set to SLEEP. So all the stuff in the main loop looking to 
*               be in run mode (i.e. NOT sleep) will not execute.
*
* INPUTS: None
*
* RETURNS: None
*
*******************************************************************************/
{
#ifdef REV_1_HW
    if ( (P1IN & AC_PRESENT) == 0)
#else
    if ( (P1IN & AC_PRESENT) == AC_PRESENT)
#endif
    {
        /*save stats first. Only do this once, as this line will hit constantly when unplugged */
        if (operatingMode != MODE_SLEEP)
        {
            CalSaveStats();     /* save stats first */
        }
        
        GoToSleep();
    }
    else
    {
        if (operatingMode == MODE_SLEEP)
        {
            /* plugged back in, so do wake up stuff */
            WakeUp();
        }
    }
}

void CheckToCancelMaintainMode(void )
/*******************************************************************************
* DESCRIPTIION: Checks to see if timed mode should be canceled. Done here
*               outside relay control funtion so it can constantly be checked 
*               against the RTC regardless of whether AC power is present
*               or not.
*
* INPUTS: None
*
* RETURNS: None
*
*******************************************************************************/
{
    if (timerModeRelayState ==  TIMER_MODE_RELAY_MAINTAIN)
    {
        if ((uiMaintainTimeTick_1min - uiLastTickMaintain) > (calConfigData.ucMaintainTimeAfterStarTime*60))
        {
            ucCancelMaintain = 1;       
        }
    }
}

void CheckToCancelTimedMode(void )
/*******************************************************************************
* DESCRIPTIION: Checks to see if timed mode should be canceled. Done here
*               outside relay control funtion so it can constantly be checked 
*               against the RTC regardless of whether AC power is present
*               or not.
*
* INPUTS: None
*
* RETURNS: None
*
*******************************************************************************/
{
/*3/31/17*/
/* change target start from config to a new variable. This allows the start time to be pushed out
in case there isn't enough time per the temperature */
//    if (timerModeRelayState == TIMER_MODE_RELAY_ON)
//    {
//        /* look to cancel when the current time is the same as start time */  
//        if (ucTimeHours == calConfigData.ucStartTimeHours)
//        {
//            if ( (ucTimeMinutes == calConfigData.ucStartTimeMinutes) && (ucTimePm == calConfigData.ucStartTimePm) )
//            {
//                ucCancelTimedStart = 1;
//            }        
//        }
//    }
    
    if (timerModeRelayState == TIMER_MODE_RELAY_ON)
    {
        /* look to cancel when the current time is the same as start time */  
        if (ucTimeHours == timedStartConfig.ucStartTimeHours)
        {
            if ( (ucTimeMinutes == timedStartConfig.ucStartTimeMinutes) && (ucTimePm == timedStartConfig.ucStartTimePm) )
            {
                ucCancelTimedStart = 1;
            }        
        }
    }   
    
    
}

void CrystalTemperatureCompensation(void )
/*******************************************************************************
* DESCRIPTIION: temperature compensates the crystal by accruing an error value.
*               this value is on the order of 10's of us per second. So check
*               the temperature, add the corresponding error to the accrued
*               value, then bump the clock when the accrued value reaches 1s
*
* INPUTS: None
*
* RETURNS: None
*
*******************************************************************************/
{  
    static unsigned int uiLastTickRTC = 0;
    
    /* calculate error once per second  */
    if (uiTimerTick_RTC_1s != uiLastTickRTC) 
    {
        fltRTCAccruedError_us += Interp2D(fltTemperature, (float*) &fltXtalErrorTemp[0], (float*) &fltXtalErrorTime_us[0], INTERP_XTAL_LENGTH); 
//        fltRTCAccruedError_us += 1;
        
        /*  1 million = 1 second */
        if (fltRTCAccruedError_us >= 1000000)
        {
            /* reached 1 sec of error, so bump RTC ticker another second */
            uiTimerTick_RTC_1s++;
            fltRTCAccruedError_us = 0;      /* zero and start accruing for next sec of error */
        }
    }
    
    uiLastTickRTC = uiTimerTick_RTC_1s;
}


/*******************************************************************************
* PUBLIC FUNCTION DEFINITIONS (storage class is blank)
* (functions called from outside this file, prototypes in .h file)
*******************************************************************************/

void UpdateTempTimeFromStart(void )
/*******************************************************************************
* DESCRIPTIION: calculates the frational number of hours away from the start
*               time we should be based on the temperature measurement
*
* INPUTS: None
*
* RETURNS: None
*
*******************************************************************************/
{  
    fltTemperatureTimeAwayFromStart = Interp2D (fltTemperature, (float*) &fltTimedTemp[0], (float*) &fltTimedTime[0], INTERP_TIMED_LENGTH);
}


//void GoToSleep(OperatingMode operatingModeNew)
void GoToSleep(void)
/*******************************************************************************
* DESCRIPTION: This function does all the housekeeping before sleep and puts
*               the processor in low power sleep mode
*
* INPUTS: sleepMode - sleep mode defines what we do in sleep and what to look for to wake
*
* RETURNS: None
*
*******************************************************************************/
{  
    WDTCTL = WDTPW + WDTHOLD;                 // Stop watchdog timer 
    
    /* if current operating mode is not sleep, save it for when we wake up */
    /* if it's sleep, it means we're hitting this every second when the RTC wakes up */
    if (operatingMode != MODE_SLEEP)
    {
        operatingModeLast = operatingMode;
    }
    operatingMode = MODE_SLEEP;
    SetupClock_Sleep();
    
    /* init led's off so when we wake they're off */
    P1OUT &= ~ (A1_CTRL | A2_CTRL | A3_CTRL | A4_CTRL | A5_CTRL);
    /* 010917 SRS */
    /* noticed LED segments not always off on power down or up, turn off cathodes as well */
    P3OUT &= ~(A_CTRL | B_CTRL | C_CTRL | D_CTRL | E_CTRL | F_CTRL | G_CTRL | DP_CTRL);

/* 1/3/17 */
/* turn off no matter what */
//#ifdef AD_VREF   
    /* turn off a/d reference */
    ADC10CTL0 = 0;
//#endif
    
    /* ensure RTC interrrup enabled, only interrupt to wake up */
    TA0CCTL0 |= CCIE;

/*DEBUG*/
//P1OUT ^= DEBUG_P15;

    __bis_SR_register(LPM3_bits + GIE);     // Enter LPM3, enable interrupts
   
}

void WakeUp(void)
/*******************************************************************************
* DESCRIPTION: This function does all the housekeeping when we wake up from
*               low power sleep mode
*
* INPUTS: None
*
* RETURNS: None
*
*******************************************************************************/
{
/* 010917 - problem with quick power cycle before time is even programmed - wakes up
and exits out of time set mode, and cycles through the timed start settings as if timed
start was running. Change to stay in program mode if time isn't set. If in the middle of
other settings, exit back to idle state */
//    if (operatingModeLast == MODE_AWAKE_PROGRAM)
//    {
//        /* was in program mode before power was pulled, go back to run mode */
//        operatingMode = MODE_AWAKE_RUN;
//        runMode = RUN_MODE_OFF;
//        progMode = PROG_MODE_OFF;
//    }
//    else
//    {
//        /* was in run mode before power was pulled, stay in run mode */
//        operatingMode = MODE_AWAKE_RUN;
//    }
  
    if (operatingModeLast == MODE_AWAKE_PROGRAM)
    {
        /* was in program mode before power was pulled */
        if (progMode == PROG_MODE_TIME)
        {
            /* if progamming time, go back to that */
            operatingMode = MODE_AWAKE_PROGRAM; 
            progMode = PROG_MODE_TIME;
        }
        else
        {
            /* if programming anything else, go to off state */
            operatingMode = MODE_AWAKE_RUN; 
            progMode = PROG_MODE_OFF;
        }
        
        runMode = RUN_MODE_OFF;        
    }
    else
    {
        /* was in run mode before power was pulled, stay in run mode */
        operatingMode = MODE_AWAKE_RUN;
    } 
                    
                    
//    ClearTickers();
    SetupClock_Wake();                  /* set up the clock */
    SetupTimerA1_Wake();                /* set up A1 for main timer ticker */
    ucAdSeedFilter = 1;                 /* seed temperature mesaurement to eliminate settling time */
    
    /* go full bright on display */
    uiLastSwitchPressTick = uiTimerTick_1s; 
    ucDigitOffTimeCt = DISP_DOT_FULL_BRIGHT;        /* display full brightness */
    
    /* re-start display at beginning */
    if (runMode == RUN_MODE_TIMER)
    {
        displayMode = DISP_MODE_INIT;
        displayState = DISP_STATE_INIT; 
        uiLastTickDisplay = uiTimerTick_1s;
    }  
    
    /* reset flash ticker */
    uiLastTickFlash = uiTimerTick_250us;
        
}


/***************************************************************************************/
void main(void)
{

//  WDTCTL = WDTPW + WDTHOLD;                 // Stop watchdog timer
    __enable_interrupt();

    /* set up port pins */
    SetupPort();

    /* set up the clock */
    SetupClock_Wake();


    SetupTimerA1_Wake();   
    
    SetupTimerA0();
    

    /* set up ad */
    ADSetup();

#ifdef DEBUG_UART    
   UARTSetupUART();
#endif

    /* wait for 1 second to add delay before accessing data flash */
//    while (uiTimerTick_250us < 4000)
//    {
////        P1OUT |= (LED_4 | LED_5);   /* turn on indicator LED's to show reset event */ 
//        P2OUT |= (LED_1 | LED_2 | LED_3);   /* turn on indicator LED's to show reset event */   
//        P3OUT ^= WD_EXT;
//    }
    CalLoad();

    /* turn em all off */
//    P2OUT &= ~(LED_1 | LED_2 | LED_3);
    /* turn on 1 color */
//    P2OUT |= (LED_2 );
    
//    UARTEnableRx();
    
    /* go into set mode if time hasn't been set yet */
//    if (calConfigData.ucTimeSet != CAL_TIME_SAVED_FLASH_KEY)
//    {
        /* go into set mode whenever we reset/lose both ac and battery power */
//        ucForceSetMode = 1;
//    }

#ifdef CONFIG_RAM
    forceSettings.byte = 0x07;          /* set all forced settings true */
#else
    forceSettings.byte = 0x01;          /* init, set time forced settings true */
    /* look for timed start config */
    if (calConfigData.ucTimedConfigSaved != CAL_SAVED_FLASH_KEY)
    {
        forceSettings.bits.TimedConfig = 1;
    }
    /* look for maintain config */
    if (calConfigData.ucMaintainConfigSaved != CAL_SAVED_FLASH_KEY)
    {
        forceSettings.bits.MaintainConfig = 1;
    }    
    
#endif
    
    
    while (1)
    {


#ifndef NO_WD
            /* strobe the watchdog */
        
            WDTCTL = (WDTPW | WDTCNTCL | WDTSSEL);
//            WDTCTL = (WDTPW | WDTCNTCL);

#endif    
  
#ifndef DISP_INTERRUPT
        /* refresh display */
        UpdateDisplay();    
#endif
        
        /* check RTC to cancel timed mode */
        CheckToCancelTimedMode();
        
        /* check RTC to cancel maintain mode */
        CheckToCancelMaintainMode();
        
        /* check if AC power gone for low power mode */
        CheckToGoToSleep();
        
        /* break up the function calls below to allow fast display service time */
        /* all precise timing is done in each individual functions */
        /* only service if we're AC powered, not battery power */
        
        if (operatingMode != MODE_SLEEP)
        {
            /*4/3/17 add temperature compensation for crystal clock error */                      
            CrystalTemperatureCompensation();  
                          
            switch (ucOSstate)   
            {   
                case 0:
                default:
                {
                    /* update timers */
                    UpdateTimers();
                    ucOSstate++;
                }
                break;
                
                case 1:
                {
                    /* read switches */
                    DebounceSwitches();                   
                    ucOSstate++;
                }
                break;
            
                case 2:
                {
                    /* process switch states */
                    ProcessSwitches();                    
                    ucOSstate++;
                } 
                break;
                
                case 3:
                {
                    /* update with new values */
                    UpdateDisplayValues();                    
                    ucOSstate++;
                }
                break;
                
                case 4:
                {
                    /* trigger a new buffer of samples when ready*/
                    ADTrigger();                   
                    ucOSstate++;
                }
                break;
                
                case 5:
                {
                    /* process the sample when ready */
                    ADProcessSamples();                      
                    ucOSstate++;
                }
                break;
                
                case 6:
                {
                    /* calculate how far from start time */
                    UpdateClockTimeFromStart();              
                    ucOSstate++;
                }
                break;
                   
                case 7:
                {
                    /* handle turning on/off the relay */
                    RelayControl();                        
                    /* start over */    
                    ucOSstate = 0;
                }
                break;      
            } 
        }  

    }
}


/******************************************************************************/
//                  Interrupts
/******************************************************************************/


// **** Timer A0, CC0 Register ISR ******************************************
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A(void)
{


//    if (operatingMode != MODE_AWAKE)
//    {
//        __bic_SR_register_on_exit(LPM4_bits);
//        WakeUp();            /* housekeeping upon wakeup from low power mode */
//    }  
 
    if (operatingMode == MODE_SLEEP)
    {
         __bic_SR_register_on_exit(LPM3_bits);
/*DEBUG*/         
//P1OUT ^= DEBUG_P15;         
        /* sleep mode, wake up and trigger IR led */
        /* move the wake up call to CheckToGoToSleep() */
//        WakeUp();       
    }  
    else
    {
        /* toggle pin to measure RTC, but not on battery */
        P1OUT ^= DEBUG_P15;           
    }
    
//    if (calConfigData.ucTimeSet == CAL_TIME_SAVED_FLASH_KEY)
    if (ucTimeSet == CAL_TIME_SAVED_FLASH_KEY)
    {      
        /* increment RTC ticker */  
        uiTimerTick_RTC_1s++; 
    }
    
    /* increment 1s general purpose ticker */ 
    uiTimerTick_1s++; 
    
    if (uiTimerTick_RTC_1s > 59)
    {
        /* 60 seconds elapsed, bump minutes */ 
        ucTimeMinutes++; 
        
        /* general purpose 1 minute ticker */
        uiTimerTick_1min++;
        
        /* keep maintain ticker running even on battery */ 
        uiMaintainTimeTick_1min++;
        /* zero counter */
        uiTimerTick_RTC_1s = 0; 
        
        if (ucTimeMinutes > 59)
        {
            /* 60 minutes elapsed, bump hours */
            ucTimeHours++; 
            ucTimeMinutes = 0;
            
            if (ucTimeHours == 12)
            {
                /* flip am/pm */
                ucTimePm ^= 1;
            }
            else if (ucTimeHours == 13)
            {
                /* flip hours */
                ucTimeHours = 1;     
            }
        }
    }
     
/*DEBUG*/
//P2OUT ^= 0x02; 
/*DEBUG*/         
//P1OUT ^= DEBUG_P15;   
}

// Timer A0, CCR1 & CCR2 Register ISR **********************************
#pragma vector=TIMER0_A1_VECTOR
__interrupt void Timer0_A1(void)
{
//    if (operatingMode != MODE_AWAKE)
//    {
//        __bic_SR_register_on_exit(LPM4_bits);
//        WakeUp();            /* housekeeping upon wakeup from low power mode */
//    }
}

/****** Timer A1, CCR0 Register ISR ********************************/
#pragma vector=TIMER1_A0_VECTOR
__interrupt void Timer1_A0 (void)
{
    uiTimerTick_250us++;        /* main timer tick */
    uiTimerTick_125us++;         /* fast display timer tick */    
  
#ifdef DISP_INTERRUPT     
    UpdateDisplay();
#endif
}

// Timer A1, CCR1 & CCR2 Register ISR ***************************************
#pragma vector=TIMER1_A1_VECTOR
__interrupt void Timer1_A1(void)
{
  switch (__even_in_range(TA1IV, 10))        // Efficient switch-implementation
  {
    case  2:                       // TACCR1
    {
        /* CCR1 both increment the 2x system ticker */   
        uiTimerTick_125us++;             /* fast display timer tick */ 
        
/*DEBUG*/         
//P1OUT ^= DEBUG_P15;           
    }
    break;
    
    case  4:                                // TACCR2
    case 10:                                // overflow     
    {                 
    }
    break;
  }      

#ifdef DISP_INTERRUPT  
  UpdateDisplay();   
#endif
}


// Port 1 interrupt service routine
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
{
//    /* wake up */
////    __bic_SR_register_on_exit(LPM4_bits);
////     WakeUp();            /* housekeeping upon wakeup from low power mode */
//    if (operatingMode != MODE_AWAKE)
//    {
//        __bic_SR_register_on_exit(LPM4_bits);
//        WakeUp();            /* housekeeping upon wakeup from low power mode */
//    }
//         
//    P1IE &= ~SW_2;
}

// Port 2 interrupt service routine
#pragma vector=PORT2_VECTOR
__interrupt void Port_2(void)
{
////    __bic_SR_register_on_exit(LPM4_bits);
////     WakeUp();            /* housekeeping upon wakeup from low power mode */
//     
//    if (operatingMode != MODE_AWAKE)
//    {
//        __bic_SR_register_on_exit(LPM4_bits);
//        WakeUp();            /* housekeeping upon wakeup from low power mode */
//    }
}

#pragma vector=NMI_VECTOR
__interrupt void NMI_Handler(void)
{
//  unsigned long i;
//
//  // Handle oscillator fault here
//  if ((IFG1 & OFIFG) == OFIFG)
//  {
//    // Osc fault, major bad day
//    __disable_interrupt();
//    DCOCTL = CALDCO_1MHZ;               // Slow down DCO
//    BCSCTL1 = CALBC1_1MHZ;
//    P1SEL &= ~LIGHT_LED_PWM;                  // Set LED to straight output (no PWM)
////    while (1)
////    {
//      P1OUT |= LIGHT_LED_PWM;                 // Turn LED on
//      for (i = 0; i < 5000; i++);
//      P1OUT &= ~LIGHT_LED_PWM;                  // Turn LED off (assume no timers to allow blink)
//      for (i = 0; i < 100000; i++);
////    }
//    LPM4;                               // Die gracefully here (requires hardware reset)
//  }
}
