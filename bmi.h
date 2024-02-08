/*******************************************************************************
* bmi.h
*
* Copyright 2016 ITE & BMI as an unpublished work.
* Engine Block Heater Timer
* All Rights Reserved.
*
* 6/23/16, Rev 1, Streicher / ITE
*   
********************************************************************************

*******************************************************************************/

#ifndef _BMI_H
#define _BMI_H

/*******************************************************************************
* INCLUDE FILES (all #include statements)
*******************************************************************************/

#include <MSP430x21x2.h>     		
#include <stdio.h>

/*******************************************************************************
* CONSTANTS AND MACRO DEFINITIONS (#define statements)
*******************************************************************************/

/* preprocessor options: */
/*
NO_WD                       no watchdog timer 
DEBUG_UART                  sends temperature data out uart, screws up 2 segments of display 
AD_VREF                     rev 2 A/D circuit - user internal 2.5V reference to power thermistor and use for a/d ref
DIMMABLE                    adjust brightness by lengthening inhibit time. May appear to pulse the dimmer it becomes
DISP_INTERRUPT              call the display handler from the main ticker interrupts
AUTO_DIM                    automatically dims the display after xx seconds of button press.
REMOVE_SAMPLE_PROCESSING    comment out a/d processing to save program space to debug without optimization
*/



#define FW_REV_MAJOR                (2)             /* 0 - 255 major rev */
#define FW_REV_MINOR                (12)             /* 0 - 255 minor rev */
#define SERIAL_REV                  (1)             /* 0 - 255 rev of serial protocol */
 
/* clock and timers **********************************************************/
/* system clock frequency, MHz */
#define SYS_CLOCK_MHZ                   (8)

/* main system ticker time, us */
#define SYS_TICK_US                     (250)
#define SYS_TICK_FAST_US                (125)

/* timer counter A1 value for TA1CCR0. this is the counter
value for the main system ticker. */ 
/* = 250us / 1/8MHz */
#define MAIN_TIMER_CTS     (2000) 
/* need to increase speed of display processing, but didn't want to monkey with the
250us timer, which is used all over. So create a new ticker 2 times as fast,
and just add CCR1 to increment it */
#define MAIN_TIMER_2X_CTS    (1000)


///* timer counter A1 value for TA1CCR0. this is the counter value
//used in sleep mode to strobe the hand sensor's IR Led. */
///* use the internal VLO clock, 12kHz, so 130ms/(1/12kHz) = 1560 */
//
//    #define SLEEP_TIMER_CTS     (600)
//    /* interval to drive IR led */
//    #define HAND_LED_INTERVAL_MS            (50.0)
//    
//    /* interval to drive IR led for paper when we're awake */
//    /* the interval for the paper led goes to the hand interval when we're trying to sense a tear while asleep */
////    #define PAPER_LED_INTERVAL_AWAKE_MS     (10.0)
//    #define PAPER_LED_INTERVAL_AWAKE_MS     (50.0)



/* switch inputs *****************************************************/
//#define SW_DEBOUNCE_CTS         (3)     /* number of consecutive reads to acknowledge a switch state change */
#define SW_DEBOUNCE_CTS         (10)     /* number of consecutive reads to acknowledge a switch state change */
#define SW_DEBOUNCE_UPDATE_RATE_US   (1000)  /* update rate of debounce processing, us */
#define SW_DEBOUNCE_UPDATE_RATE_CT  (SW_DEBOUNCE_UPDATE_RATE_US/SYS_TICK_US) 
//#define SWITCH_LONG_PRESS_MS    (3000)  /* length of time switch is held to consider it a long press */
//#define SWITCH_PRESSED_LED_ON_MS    (1000)  /* lenght of time the led stays on after a configuration button press */

#define SWITCH_HOLD_TIME_DISP_STATS_S   (3)    /* time to hold switches to display stats */ 

/* display controls **************************************************/
/* raw display driver routine uses faster clock */
#define DISP_DIGIT_ON_TIME_US   (1000)    /* time that each digit is on, 5 digits total, so total cycle time to refresh is this x5 */
#define DISP_DIGIT_ON_TIME_CT    (DISP_DIGIT_ON_TIME_US /SYS_TICK_FAST_US)
#define DISP_DIGIT_OFF_TIME_CT  (1)     /* time between digits to keep everything off to prevent "bleed-over" from adjacent digit */


#define DISP_RUN_SCREEN_TIME_S  (2)     /* time to display any given value in run mode, in seconds */
#define DISP_RUN_SCREEN_TIME_CT DISP_RUN_SCREEN_TIME_S 
#define DISP_FLASH_TIME_MS      (400)   /* on/off time in flash mode */
#define DISP_FLASH_TIME_CT      (DISP_FLASH_TIME_MS* (1000/SYS_TICK_US))
#define DISP_INHIBIT_FLASH_MS   (300)   /* time to inhibit flashing after a button press to easier read the display while setting */
#define DISP_INHIBIT_FLASH_CT   (DISP_INHIBIT_FLASH_MS*(1000/SYS_TICK_US))
#define DISP_AUTO_DIM_TIME_S    (15)     /* time after a button press to dim display */
#define DISP_AUTO_DIM_TIME_CT   DISP_AUTO_DIM_TIME_S    /* 1 sec ticker */
#define DISP_DOT_FULL_BRIGHT    (1)     /* display digit off time full brightness */
#define DISP_DOT_FULL_DIM       (7)     /* display digit off time fully dimmed */
#define SWITCH_HOLD_TIME_DISP_STATS_CT  (SWITCH_HOLD_TIME_DISP_STATS_S) 


/* clock display settings */
#define CLOCK_DEFAULT_MIN       (0)
#define CLOCK_DEFAULT_HR        (12)
#define CLOCK_DEFAULT_PM        (0)


/* a/d trigger ******************************************************/
#define AD_TRIGGER_MS           (100)       /* sampling rate of a/d - triggers a buffer of 8 samples per channel */
#define AD_TRIGGER_CT           (AD_TRIGGER_MS*(1000/SYS_TICK_US))

/* temperature measurement range */
#define AD_MEAS_TEMP_MAX        (80)        /* max allowable temperature measurement, degC */
#define AD_MEAS_TEMP_MIN        (0)         /* min allowable temperature measurement, degC */

/* a/d filtering ****************************************************/
//Filtered value = filtered value + (new sample - filtered value)*K
// K = sampling rate (ms) / (LPF tau (ms) + sampling rate (ms))
#define AD_TEMP_LPF_TAU_MS      (5000)       /* low pass filter tau for temperature measurements */
/* calculated from above */
#define AD_TEMP_LPF_K   (AD_TRIGGER_MS * 1.0)/(AD_TEMP_LPF_TAU_MS * 1.0 + AD_TRIGGER_MS * 1.0)
#define NUM_SAMPLES_SEED_FILTER (10)    /* number of samples to turn off filter to reduce settling time */

/* relay control ******************************************************/
#define RELAY_TIME_DEFAULT      (1.0)     /* interpolated on or off time for relay     */
#define RELAY_TIME_SEC_DEFAULT      (60.0)     /* interpolated on or off time for relay, seconds     */
#define RELAY_ON_TIME_1ST_CYCLE_MAINTAIN_MAX    (210)       /* max time allowed for relay to be on during initial on-cycle of a new maintain mode, minutes */
#define MAINTAIN_MODE_DELAY_S           (10)    /* time to delay energizing relay when going into maintain mode, seconds */
#define MAINTAIN_MODE_DELAY_CT  (MAINTAIN_MODE_DELAY_S)

/* port pins **********************************************************/


/* PORT 1 */
#define A1_CTRL         BIT0
#define A2_CTRL         BIT1
#define A3_CTRL         BIT2
#define A4_CTRL         BIT3
#define A5_CTRL         BIT4
#define DEBUG_P15       BIT5
#define AC_PRESENT      BIT6
#define RELAY           BIT7



/* PORT 2 */
#ifdef AD_VREF

#define VTEMP           BIT0
#define PROG_MODE       BIT1
#define UP              BIT2
#define DOWN            BIT3
#define AD_VREF_OUTPUT  BIT4
#define RUN_MODE        BIT5

#else

#define VTEMP           BIT0
#define UP              BIT2
#define DOWN            BIT3
#define PROG_MODE       BIT4
#define RUN_MODE        BIT5

#endif

/* PORT 3 */
#define A_CTRL          BIT0
#define B_CTRL          BIT1
#define C_CTRL          BIT2
#define D_CTRL          BIT3
#define E_CTRL          BIT4
#define F_CTRL          BIT5
#define G_CTRL          BIT6
#define DP_CTRL         BIT7




/*******************************************************************************
* TYPEDEFS, STRUCTURES, AND ENUM DECLARATIONS
*******************************************************************************/
/* switch inputs */
typedef union
{
    unsigned char byte;
    struct
    {
        unsigned char Up                :1;     /* pb switch 1 */
        unsigned char Down              :1;     /* pb switch 2 */
        unsigned char ProgMode          :1;     /* pb switch 3 */  
        unsigned char RunMode           :1;     /* pb switch 4 */     
        unsigned char spare             :4;     
    } bits;
} SwitchInputs; 

//typedef union
//{
//    unsigned char byte;
//    struct
//    {
//        unsigned char UpPressed             :1;     /* pb switch 1 pressed */
//        unsigned char DownPressed           :1;     /* pb switch 2 pressed */
//        unsigned char ProgModePressed       :1;     /* pb switch 3 pressed */
//        unsigned char RunModePressed        :1;     /* pb switch 4 pressed */
//        unsigned char spare                 :4;     /* spare */  
//    } bits;
//} SwitchPressed;

typedef union
{
    unsigned char byte;
    struct
    {
        unsigned char Spare012              :3;     /* bit positions 0 & 1 & 2 are used by 7-segement */
        unsigned char Time                  :1;
        unsigned char StartTime             :1;
        unsigned char TimedMode             :1;
        unsigned char MaintainMode          :1;
        unsigned char Spare7                :1;     /* bit position 7 spare */
    } bits;
} DiscreteLed;
        
typedef union
{
    unsigned char byte;
    struct
    {
        unsigned char Time                  :1;     /* force user to set time upon initial power up */     
        unsigned char TimedConfig           :1;     /* force user to set timed start config upon initial power up */
        unsigned char MaintainConfig        :1;     /* force user to set maintain mode config upon initial power up */
        unsigned char ReProgTimedConfig     :1;     /* indicates user is re-programming timed start config from run mode */
        unsigned char ReProgMaintainConfig  :1;     /* indicates user is re-programming maintain config from run mode */
        unsigned char Spare                 :3;
    } bits;
} ForceSettings;
        

typedef struct
{
    unsigned char ucStartTimeHours;
    unsigned char ucStartTimeMinutes;
    unsigned char ucStartTimePm;
} TimedStartConfig;

typedef enum
{
    MODE_AWAKE_RUN,
    MODE_AWAKE_PROGRAM,
    MODE_TEST,
    MODE_SLEEP
} OperatingMode;

typedef enum
{
    RUN_MODE_OFF,
    RUN_MODE_TIMER,
    RUN_MODE_MAINTAIN,
    RUN_MODE_ON
} RunMode;

typedef enum
{
    PROG_MODE_OFF,
    PROG_MODE_TIME,
    PROG_MODE_START_TIME,
    PROG_MODE_TEMP_UNITS,
    PROG_MODE_MAINTAIN_TIME,
    PROG_MODE_CALIBRATION
} ProgMode;

typedef enum
{
    OP_STATE_IDLE,
    OP_STATE_RUN_TIMER,
    OP_STATE_RUN_MAINTAIN,
    OP_STATE_RUN_ON,
    OP_STATE_PROG_TIME,
    OP_STATE_PROG_UNITS,
    OP_STATE_PROG_START,
    OP_STATE_PROG_MAINTAIN,
//    OP_STATE_CALIBRATION
    OP_STATE_DISP_STATS,
} OperatingState;
    

typedef enum
{
    DISP_STATE_INIT,    /* initialized state */
    DISP_STATE_A1,      /* anode 1, this is the 7-segment MSB, digit 1 */
    DISP_STATE_A2,      /* anode 2, this is the 7-segment digit 2 */     
    DISP_STATE_A3,      /* anode 3, this is the 7-segment colon, degree symbol, and discrete LED's */
    DISP_STATE_A4,      /* anode 4, this is the 7-segment digit 3 */
    DISP_STATE_A5       /* anode 5, this is the 7-segment LSB, digit 4 */
} DisplayState;

//typedef enum
//{
//    RUN_DISP_MODE_TIME,
//    RUN_DISP_MODE_START_TIME,
//    RUN_DISP_MODE_TEMP,
////    DISP_MODE_MAINTAIN_TIME
//} RunDisplayMode;
//
//typedef enum
//{
//    PROG_DISP_MODE_TIME,
//    PROG_DISP_MODE_START_TIME,
//    PROG_DISP_MODE_MAINTAIN_TIME
//} ProgDisplayMode;

typedef enum
{
    DISP_MODE_INIT,
    DISP_MODE_IDLE,
    DISP_MODE_TIME,
    DISP_MODE_START_TIME,
    DISP_MODE_TEMP,
    DISP_MODE_TEMP_UNITS,
    DISP_MODE_MAINTAIN_TIME,
//    DISP_MODE_CAL,
    DISP_MODE_ON,
    DISP_MODE_REDY,
    DISP_MODE_STATS
} DisplayMode;


typedef enum
{
    DISP_NO_FLASH,
    DISP_FLASH,
} DisplayFlash;

typedef enum
{
    DISP_INHIBIT_FLASH_NO,
    DISP_INHIBIT_FLASH_SET,
    DISP_INHIBIT_FLASH
} DisplayInhibitFlash;

typedef enum
{
    STAT_DISP_RELAY,
    STAT_DISP_HOURS_OFF
} StatDispMode;

typedef enum
{
    TEMP_UNITS_F,
    TEMP_UNITS_C
} TempUnits;  
        
typedef enum
{
    RELAY_OFF,
    RELAY_ON
} RelayState;

typedef enum
{
    TIMER_MODE_RELAY_OFF,
    TIMER_MODE_RELAY_ON,
    TIMER_MODE_RELAY_MAINTAIN
} TimerModeRelayState;

typedef enum
{
    MAINTAIN_MODE_RELAY_DISABLED,
    MAINTAIN_MODE_RELAY_ENABLED
} MaintainModeRelayState;

/*******************************************************************************
* DATA DECLARATIONS (storage class is extern)
*******************************************************************************/
          
extern volatile unsigned int uiTimerTick_250us;           
extern volatile unsigned int uiTimerTick_RTC_1s;   
extern volatile unsigned int uiTimerTick_1s;
extern volatile unsigned int uiTimerTick_1min;
extern volatile unsigned int uiTimerTick_125us;
extern volatile unsigned int uiMaintainTimeTick_1min;

extern volatile unsigned char ucTimeHours;
extern volatile unsigned char ucTimeMinutes;
extern volatile unsigned char ucTimePm;

extern volatile OperatingMode operatingMode;
extern OperatingMode operatingModeLast;
extern RunMode runMode;
extern RunMode runModeRelay;    
extern ProgMode progMode;

extern TimerModeRelayState timerModeRelayState;
extern MaintainModeRelayState maintainModeRelayState;


/*******************************************************************************
* FUNCTION DECLARATIONS (prototypes)
*******************************************************************************/
void GoToSleep(void);
void WakeUp(void);
void SetupClock_Wake(void);
void SetupClock_Sleep(void);
void UpdateTempTimeFromStart(void);

#endif
