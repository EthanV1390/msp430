/*
    Ethan Van Deusen
    Modifiable Timing Light
    main.c

    Timer setup and main function
*/
#include <msp430.h>
#include "pwm.h"
#include "buttons.h"
#include "display.h"
#include "flash.h"

/************************** Timer Setup **************************/
volatile unsigned int timerTick125 = 0;
void setupTimerA0(void) // Used for PWM output
{
    P2SEL |= (BIT6 | BIT7);                       // Configure XIN and XOUT for external crystal input
    BCSCTL3  |= (LFXT1S_0 | XCAP_3);              // LFXT1 = external crystal, Set capacitor to 12.5pF for crystal
    TA0CCTL2 |= OUTMOD_3;                         // Set timer's output mode to set/reset
    TA0CTL   |= (TASSEL_2 + MC_1 + TACLR + ID_0); // Source from SMCLK(Dynamic), Up mode, Clear timer count, Prescaler of 1(Dynamic)
}

void setupTimerA1(void) // 125us timer counter, used for display and buttons
{
    BCSCTL1 = (CALBC1_16MHZ);
    DCOCTL  = (CALDCO_16MHZ);
    BCSCTL2 = (SELM_0);                   // Selects DCO for SMCLK & MCLK
    TA1CCR0 = 2000;                       // Timer period register,   2000 / 16MHZ = 125us
    TA1CTL |= (TASSEL_2 + TACLR + MC_1);  // SMCLK, clear timer, count-up mode: Count up to CCR0 and reset
    TA1CCTL0 = CCIE;                      // Enable Timer1 CCR0 interrupt
    __bis_SR_register(GIE);               // Enable global interrupt
}

#pragma vector=TIMER1_A0_VECTOR
__interrupt void Timer1_A1_ISR(void)
{
    timerTick125++; // Incremented every 125us
}
/*****************************************************************/

void applyChanges(void)
{
    blinkStatus = 0;                         // Switched back to zero, If 1 then the device is in the middle of blinking, looks weird to switch out of prog mode and have to wait on a blank display
    pwm();                                   // Reruns pwm() to update LED frequency
    separateDigits(currentFreq, currentDc);  // Reruns seperateDigits to reassign 7SD digits
}

void flashProcess(void) // Writes preset frequency or duty cycle into flash and then loads into preset array, used when applying values
{
    if(valType == frequency)
    {
        flashWrite(presetFreq);
        valType = dutyCycle;
        flashWrite(presetDc);
        valType = frequency;
    }
    else if(valType == dutyCycle)
    {
        flashWrite(presetDc);
        valType = frequency;
        flashWrite(presetFreq);
        valType = dutyCycle;
    }
    flashLoad();
}

void main(void)
{
    WDTCTL = WDTPW | WDTHOLD;
    setupTimerA0();
    setupTimerA1();

    flashLoad();
    valType = frequency;
    flashLoad();
    applyChanges();
    presetFreq[3] = 0.1;
    presetDc[3] = 50;
    while(1)
    {
        processSwitches();
        updateDisplay();
        presetInfo();
    }
}



