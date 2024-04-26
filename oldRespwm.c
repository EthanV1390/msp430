/*
    Ethan Van Deusen
    Modifiable Timing Light
    pwm.c

    Contains functions to convert the inputed frequency and duty cycle into the period for the timer register,
    also contains PWM function to assign values and configure P1.
*/

#include <msp430.h>
#include "pwm.h"
#include "buttons.h"
#include "display.h"
#include "flash.h"

unsigned char prescl = 1;         // Default prescaler 1
float currentFreq;
int currentDc;
char presetNum = 0;
float clockSpeed;

float debugPeriodInSec;
unsigned int debugPeriod;
unsigned int debugDC;


/************ Preset Values ***********/
float presetFreq[4];
float presetDc[4];
/*************************************/

unsigned int periodCalc(float freq, unsigned int prescaler)     // Converts inputed frequency into TA0CCR0
{
    float periodInSec = 1.0 / freq;                                                   // Calculate the period in seconds
    debugPeriodInSec = periodInSec;
    unsigned int periodInCycles = (unsigned int)((clockSpeed/prescaler) * periodInSec);  // Convert to clock cycles
    return periodInCycles;
}
unsigned int dcCalc(float dc, unsigned int period)                                    // Converts inputed duty cycle into TA0CCR1
{
    unsigned int dcPercent = period * (1-(dc/100));
    return dcPercent;
}

void pwm(void)
{
    currentFreq = presetFreq[presetNum];
    currentDc = presetDc[presetNum];

    if(suFlag != 1) // Shouldn't happen in single use mode
    {
        P1IN &= ~(BIT7);    // Switches LED off to ensure PWM cycle is restarted
        TA0CCR0 = 0;
        TA0CCR2 = 0;
    }
    // Dynamic prescaler and clock source needed to represent the full range of 0.1 - 999.9Hz, switches from the 16MHz DCO to 32,768Hz external crystal
    if (currentFreq >= 245) // 16MHz, No prescaler
    {
        clockSpeed = DCOSpeed;
        prescl = 1;
        TA0CTL = (TASSEL_2 + MC_1 + ID_0);
    }
    else if (currentFreq < 245 && currentFreq > 31) //  16MHz, 8 prescaler
    {
        clockSpeed = DCOSpeed;
        prescl = 8;
        TA0CTL = (TASSEL_2 + MC_1  + ID_3);
    }
    else if (currentFreq <= 31 && currentFreq > 1)   //  32,768Hz, No prescaler
    {
        clockSpeed = xternCrystalSpeed;
        prescl = 1;
        TA0CTL = (TASSEL_1 + MC_1  + ID_0);
    }
    else if (currentFreq <= 1 && currentFreq >= 0.1) //  32,768Hz, 8 prescaler
    {
        clockSpeed = xternCrystalSpeed;
        prescl = 8;
        TA0CTL = (TASSEL_1 + MC_1  + ID_3);
    }

    unsigned int period = periodCalc(currentFreq, prescl);
    debugPeriod = period;
    unsigned int dutyCycle = dcCalc(currentDc, period);
    debugDC = dutyCycle;

    // P1 Configuration
    P1DIR |= (BIT7);
    P1SEL |= (BIT7);        // Select P1.7 as timer output instead of GPIO

    // Assign timer registers
    TA0CCR0 = (period);
    TA0CCR2 = (dutyCycle);
}

