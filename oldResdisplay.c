/*
    Ethan Van Deusen
    Modifiable Timing Light
    display.c

    Sets up the display and translates pwm values into 7SD segments
    Also blinks preset information and switches between presets and value types
 */

#include "buttons.h"
#include "main.h"
#include "pwm.h"
#include "display.h"

// Blink control variables
volatile unsigned char blinkStatus = 0;
volatile unsigned int blinkTick = 0;
volatile unsigned int blinkTime = BLINK_TIME;

volatile unsigned char progFlag = 0;
volatile unsigned char suFlag = 0;

float tempFreq = 0;
unsigned int tempDc = 0;

// Seven Segment Display Characters, Index in display.h
const unsigned char ucCharToHex[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, 0x3F, 0x39, 0x71, 0x76, 0x31, 0x3F, 0x54, 0x79, 0x5E, 0x6E, 0xBF, 0x40, 0xF3, 0x0, 0x58};
unsigned int decShown = BIT7;   // Adds a decimal to the 10s place
unsigned int pmShown = 0;       // Displays 'pm' on last digit, used to indicate switching to program mode
unsigned int degShown = 0;      // Displays the degree symbol, used to indicate switching to single use mode
void updateDisplay(void)
{
    static unsigned int LastTick = 0; // Timer counter to compare against timerTick125 for waiting
    switch (dispState)
    {
        case dispInit:  // Default setup state
        default:
        {
            P1OUT &= ~(A1_CTRL | A2_CTRL | A3_CTRL | A4_CTRL | A5_CTRL);
            P3OUT &= ~(A_CTRL | B_CTRL | C_CTRL | D_CTRL | E_CTRL | F_CTRL | G_CTRL | DP_CTRL);
            LastTick = timerTick125;

            // Set P1 and P3 to output
            P3DIR |= 0xFF;
            P1DIR |= 0xFF;
            dispState = dispDigit1;
        }
        break;
        case dispDigit1:
        {
            if ((timerTick125 - LastTick) >= DIGIT_OFF_TIME) // 1 tick of 125us
                {
                    P3OUT = ucCharToHex[digit1Val] & ~(BIT7);
                    P1OUT |= A1_CTRL;
                }
            if ((timerTick125 - LastTick) >= DIGIT_ON_TIME) // 8 ticks of 125us = 1000 us = 1 ms
                {
                    LastTick = timerTick125;
                    dispState = dispDigit2;
                    P1OUT &= ~A1_CTRL;
                }
        }
        break;
        case dispDigit2:
        {
            if ((timerTick125 - LastTick) >= DIGIT_OFF_TIME)
                {
                    P3OUT = ucCharToHex[digit2Val];
                    P1OUT |= A2_CTRL;
                }
            if ((timerTick125 - LastTick) >= DIGIT_ON_TIME)
                {
                    LastTick = timerTick125;
                    dispState = dispDigit3;
                    P1OUT &= ~A2_CTRL;
                }
        }
        break;
        case dispDigit3:
        {
            if ((timerTick125 - LastTick) >= DIGIT_OFF_TIME)
                {
                    P3OUT = degShown;
                    P1OUT |= A3_CTRL;
                }
            if ((timerTick125 - LastTick) >= DIGIT_ON_TIME)
                {
                    LastTick = timerTick125;
                    dispState = dispDigit4;
                    P1OUT &= ~A3_CTRL;
                }
        }
        break;
        case dispDigit4:
        {
            if ((timerTick125 - LastTick) >= DIGIT_OFF_TIME)
                {
                    P3OUT = ucCharToHex[digit3Val] | decShown;
                    P1OUT |= A4_CTRL;
                }
            if ((timerTick125 - LastTick) >= DIGIT_ON_TIME)
                {
                    LastTick = timerTick125;
                    dispState = dispDigit5;
                    P1OUT &= ~A4_CTRL;
                }
        }
        break;
        case dispDigit5:
        {
            if ((timerTick125 - LastTick) >= DIGIT_OFF_TIME)
                {
                    P3OUT = ucCharToHex[digit4Val] | pmShown;
                    P1OUT |= A5_CTRL;
                }
            if ((timerTick125 - LastTick) >= DIGIT_ON_TIME)
                {
                    LastTick = timerTick125;
                    dispState = dispDigit1;
                    P1OUT &= ~A5_CTRL;
                }
        }
        break;
    }
}

void switchValType(void)    // Switches between displaying frequency and duty value on the display
{
    if(valType == frequency)
    {
        decShown = 0;
        valType = dutyCycle;
        separateDigits(currentFreq, currentDc);
    }
    else if(valType == dutyCycle)
    {
        decShown = BIT7;
        valType = frequency;
        separateDigits(currentFreq, currentDc);
    }
}

void switchPreset(int direction)  // Switches to the next preset
{
    if((presetNum == 0 && direction == 1) || (presetNum == 3 && direction == -1))
    {
        if(direction == 1)
        {
            presetNum = 3;
            applyChanges();
        }
        else if(direction == -1)
        {
            presetNum = 0;
            applyChanges();
        }
    }
    else
    {
        presetNum -= direction;

        if(presetNum == 0)
        {
            currentFreq = presetFreq[presetNum];   // Sets the frequency to preset value
            currentDc = presetDc[presetNum];       // Sets the duty cycle to preset value
            applyChanges();
        }
        else if(presetNum == 1)
        {
            currentFreq = presetFreq[presetNum];
            currentDc = presetDc[presetNum];
            applyChanges();
        }
        else if(presetNum == 2)
        {
            currentFreq = presetFreq[presetNum];
            currentDc = presetDc[presetNum];
            applyChanges();
        }
        else if(presetNum == 3)
        {
            currentFreq = presetFreq[presetNum];
            currentDc = presetDc[presetNum];
            applyChanges();
        }
    }

}

// Separates the digits of the input values into 7-segment display segments
void separateDigits(float freq, float dc)
{
    // Cases where value is edited outside of the range
    if(currentFreq >= 1000)
    {
        freq = 999.9;
        currentFreq = 999.9;
        presetFreq[presetNum] = 999.9;
    }
    else if(currentFreq <= 0)
    {
        freq = 0.1;
        currentFreq = 0.1;
        presetFreq[presetNum] = 0.1;
    }
    if(currentDc > 100)
    {
        dc = 100;
        currentDc = 100;
        presetDc[presetNum] = 100;
    }
    else if(currentDc < 1)
    {
        dc = 1;
        currentDc = 1;
        presetDc[presetNum] = 1;
    }
    if(blinkStatus == 0)            // Normal display, set digits to PWM values
    {
        int intPart = (int)freq;    // Isolate integer part
        int dcDigit = (int)dc;
        float decValue = freq - intPart;
        decValue = (decValue * 10) + 0.5;
        int roundedVal = (int)decValue;
        switch (valType)
        {
            case (frequency):
                if (currentFreq >= 0.1 && currentFreq <= 1000) // 0.1 - 999.9
                {
                    decShown = BIT7;
                    digit1Val = (intPart / 100);
                    digit2Val = (intPart % 100) / 10;
                    digit3Val = (intPart % 10);
                    digit4Val = (roundedVal);
                    if (digit1Val == 0) {digit1Val = 23;} // If 0 then switch to blank
                    if (digit2Val == 0 && digit1Val == 23) {digit2Val = 23;}
                }
            break;
            case (dutyCycle):
                if(currentDc >= 1 && currentDc <= 100 )   // 1 - 100
                {
                    digit1Val = (23);
                    digit2Val = (dcDigit % 1000) / 100;
                    digit3Val = (dcDigit % 100) / 10;
                    digit4Val = (dcDigit % 10);
                    if (digit2Val == 0) {digit2Val = 23;} // If 0 then switch to blank
                    if (digit3Val == 0 && digit2Val == 23) {digit3Val = 23;}
                }
            break;
        }
    }
    else if(blinkStatus == 1)   // Update display with preset info
    {
        decShown = 0;
        digit1Val = digit1Blink;
        digit2Val = digit2Blink;
        digit3Val = digit3Blink;
        digit4Val = digit4Blink;
    }
}

void progModeChanges(void)
{
    {
        if(progFlag == 0)                                   // Not in prog mode, Switch into and start blinking
            {
                tempFreq = currentFreq;                     // Store starting values in case user wants to exit without saving
                tempDc = currentDc;
                applyChanges();
                blinkTime = PROG_BLINK_TIME;
                progFlag = 1;
            }
            else if(progFlag == 1 && (P2IN & APPLY_BUTTON))  // In prog mode, Exit and apply changes (Exited with apply button)
            {
                pmShown = 0;
                applyChanges();
                blinkTime = BLINK_TIME;
                progFlag = 0;
            }
            else if(progFlag == 1 && (P2IN & TOGGLE_BUTTON))  // In prog mode, Exit but do NOT apply changes (Exited with toggle button)
            {
                if(presetFreq[presetNum] != tempFreq || presetDc[presetNum] != tempDc) // If values have not changed then no need for this
                {
                    presetFreq[presetNum] = tempFreq;   // Reassign to old values
                    presetDc[presetNum] = tempDc;
                    currentFreq = tempFreq;
                    currentDc = tempDc;
                    flashProcess();
                    applyChanges();
                }
                blinkStatus = 0;          // Switched back to zero, If 1 then the device is in the middle of blinking, looks weird to switch out of prog mode and have to wait on a blank display
                pmShown = 0;             // Turn 'pm' off
                progFlag = 0;
                blinkTime = BLINK_TIME;
            }
    }

}

void singleUseMode()
{
    if(suFlag == 0)
    {
        suFlag = 1;
        degShown = BIT2;
    }
    else if(suFlag == 1)
    {
        suFlag = 0;
        degShown = 0;

    }
}

void presetInfo(void)   // Switches display from preset values to current preset information. Ex. 'P2 dc' for Preset 2 duty cycle. Also handles blinking in prog mode
{
    static unsigned int blinkTick = 0;
    if(P2IN & (UP_BUTTON | DOWN_BUTTON | TOGGLE_BUTTON))    // Switches back to the values when a button is pressed
    {
        blinkTick = timerTick125;
        blinkStatus = 0;
    }
    if((timerTick125 - blinkTick) >= blinkTime)
    {
        blinkStatus = (1 - blinkStatus); // Blink on and off every 3 seconds
        if(progFlag != 1)
        {
            digit1Blink = (22);               // P
            digit2Blink = (presetNum + 1);    // Preset number
            if(valType == frequency)
            {
                digit3Blink = (23);           // Blank
                digit4Blink = (12);           // F
            }
            else if(valType == dutyCycle)
            {
                digit3Blink = (18);           // d
                digit4Blink = (24);           // c
            }
        }
        else if(progFlag == 1)  // Blink on and off when in prog mode
        {
            digit1Blink = (23);
            digit2Blink = (23);
            digit3Blink = (23);
            digit4Blink = (23);
        }
        blinkTick = timerTick125;
        separateDigits(currentFreq, currentDc);
    }

}
