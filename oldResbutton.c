/*
    Ethan Van Deusen
    Modifiable Timing Light
    buttons.c

    Handles switch actions and button debouncing
 */

#include "buttons.h"
#include "main.h"
#include "pwm.h"
#include "display.h"
#include "flash.h"

volatile unsigned int debugLastTick = 0;
volatile unsigned int debugholdTick = 0;
volatile unsigned int debugTimePassed = 0;
volatile unsigned int debugTotalTimePassed = 0;


// Initialize buttons
Button Toggle = { .currState = idle, .debounceState = idle };
Button Apply = { .currState = idle, .debounceState = idle };
Button Up = { .currState = idle, .debounceState = idle };
Button Down = { .currState = idle, .debounceState = idle };

void debounce(unsigned char button, volatile ButtonState *state, volatile ButtonState *debounceState)
{
    static unsigned int LastTick = 0;
    static unsigned int HoldStart = 0;  // Variable to track when the button was first held
    unsigned char currState = (P2IN & button);

    switch (*debounceState)
    {
        case idle:
            if (currState == button)
            {
                *debounceState = pressed;
                LastTick = timerTick125;
                debugLastTick = timerTick125;
            }
            break;
        case pressed:
            if (currState == button)
            {
                if ((timerTick125 - LastTick) >= HELD_TIME)
                {
                    *state = held;             // Button held for 875 ms
                    *debounceState = held;
                    HoldStart = timerTick125;  // Record the time when the button was first held
                    debugholdTick = HoldStart;
                }
            }
            else
            {
                if ((timerTick125 - LastTick) >= DEBOUNCE_TIME)
                {
                    *state = pressed;       // Regular press
                    *debounceState = released;
                }
                else
                {
                    *debounceState = idle;  // Premature release, debounce timer not finished
                }
            }
            break;
        case released:
            if (currState != button)
                *debounceState = idle;
            break;
        case held:
            if (currState == button)
            {
                if ((timerTick125 - HoldStart) >= FIRST_HELD_TIME)
                {
                    debugTimePassed = (timerTick125 - HoldStart);
                    debugTotalTimePassed = (timerTick125 - LastTick);
                    *state = firstHeld;
                    *debounceState = firstHeld;
                }
                else if (currState != button)   // Button released
                {
                    *debounceState = idle;
                }
            }
            else
            {
                *debounceState = idle;
            }
            break;
        case firstHeld:
            if (currState == button)
              {
                  if ((timerTick125 - HoldStart) >= SECOND_HOLD_TIME)
                  {
                      debugTimePassed = (timerTick125 - HoldStart);
                      debugTotalTimePassed = (timerTick125 - LastTick);
                      *state = secondHold;
                      *debounceState = secondHold;
                  }
                  else if (currState != button)   // Button released
                  {
                      *debounceState = idle;
                  }
              }
            else
            {
                *debounceState = idle;
            }
            break;
        case secondHold:
            if (currState == button)
              {
                  if ((timerTick125 - HoldStart) >= THIRD_HOLD_TIME)
                  {
                      debugTimePassed = (timerTick125 - HoldStart);
                      debugTotalTimePassed = (timerTick125 - LastTick);
                      *state = thirdHold;
                      *debounceState = thirdHold;
                  }
                  else if (currState != button)   // Button released
                  {
                      *debounceState = idle;
                  }
              }
            else
            {
                *debounceState = idle;
            }
            break;
        case thirdHold:
            if (currState != button)
            {
                *debounceState = idle;
            }
            break;
    }
}

void processSwitches(void)
{
    P2DIR &= (~TOGGLE_BUTTON | ~APPLY_BUTTON | ~UP_BUTTON | ~DOWN_BUTTON);

    debounce(TOGGLE_BUTTON, &Toggle.currState, &Toggle.debounceState);
    debounce(APPLY_BUTTON, &Apply.currState, &Apply.debounceState);
    debounce(UP_BUTTON, &Up.currState, &Up.debounceState);
    debounce(DOWN_BUTTON, &Down.currState, &Down.debounceState);

    /***** Toggle Button *****/
    if (Toggle.currState == held)
    {
        if(suFlag != 1)
        {
            if(presetNum != 3)  // Don't need program mode for single use mode
            {
                if(P2IN & TOGGLE_BUTTON)    // Gives feedback for when prog mode is switched into
                    pmShown = BIT7;         // Switch 'PM' on
                progModeChanges();          // Turns Prog mode on, if held again it exits without saving
                applyChanges();
            }

        }
        Toggle.currState = idle;
    }
    else if (Toggle.currState == pressed && progFlag != 1)  // Pressed when not in programming mode
    {
        switchValType();
        Toggle.currState = idle;
    }
    else if (Toggle.currState == pressed && progFlag == 1)
    {
        switchValType();
        Toggle.currState = idle;
    }
    /***** Apply Button *****/
    else if (Apply.currState == held && progFlag == 1)    // Apply and save   (Apply and exit prog mode)
    {
        progModeChanges();  // Switches out of prog mode
        flashProcess();
        applyChanges();
        Apply.currState = idle;
    }
    else if (Apply.currState == pressed && progFlag == 1) // Apply but keep editing (Stay in prog mode and don't write to flash yet)
    {
        flashProcess();
        applyChanges();
        Apply.currState = idle;
    }
    else if (Apply.currState == pressed && progFlag != 1) // Apply pressed outside of program mode
    {
        if (presetNum == 3) // Only happens with the single use preset
            singleUseMode();
        Apply.currState = idle;
    }
    else if (Apply.currState == firstHeld && progFlag != 1) // Apply firstHeld outside of program mode, resets defaults
    {
        if(presetNum != 3)  // Don't need program mode for single use mode
        {
            setDefaults();
        }
        Apply.currState = idle;
    }
    /***** Up Button *****/
    else if (Up.currState == thirdHold && (progFlag == 1 || suFlag == 1))
    {
        if(P2IN & UP_BUTTON)
        {
            fastIncreaseVal(FASTER_INCREMENT_SPEED, 100);
            if (suFlag == 1)
                applyChanges();
        }
        else
        {
            Up.currState = idle;
        }
    }
    else if (Up.currState == secondHold && (progFlag == 1 || suFlag == 1))
    {
        if(P2IN & UP_BUTTON)
        {
            fastIncreaseVal(SLOWER_INCREMENT_SPEED, 100);
            if (suFlag == 1)
                applyChanges();
        }
        else
        {
            Up.currState = idle;
        }
    }
    else if (Up.currState == firstHeld && (progFlag == 1 || suFlag == 1))
    {
        if(P2IN & UP_BUTTON)
        {
            fastIncreaseVal(SLOWER_INCREMENT_SPEED, 10);
            if (suFlag == 1)
                applyChanges();
        }
        else
        {
            Up.currState = idle;
        }
    }
    else if (Up.currState == held && (progFlag == 1 || suFlag == 1))
    {
        if(P2IN & UP_BUTTON)
        {
            fastIncreaseVal(INCREMENT_SPEED, 1);
            if (suFlag == 1)
                applyChanges();
        }
        else
        {
            Up.currState = idle;
        }
    }
    else if (Up.currState == pressed && (progFlag == 1 || suFlag == 1))
    {
        increaseVal();
        if (suFlag == 1)
            applyChanges();
        Up.currState = idle;
    }
    else if (Up.currState == pressed && progFlag != 1)
    {
        switchPreset(-1);
        blinkTick = timerTick125; // Reset time between blinks if button is pressed
        Up.currState = idle;
    }
    /***** Down Button *****/
    else if (Down.currState == thirdHold && (progFlag == 1 || suFlag == 1))
    {
        if(P2IN & DOWN_BUTTON)
        {
            fastDecreaseVal(FASTER_INCREMENT_SPEED, 100);
            if (suFlag == 1)
                applyChanges();
        }
        else
        {
            Up.currState = idle;
        }
    }
    else if (Down.currState == secondHold && (progFlag == 1 || suFlag == 1))
    {
        if(P2IN & DOWN_BUTTON)
        {
            fastDecreaseVal(SLOWER_INCREMENT_SPEED, 100);
            if (suFlag == 1)
                applyChanges();
        }
        else
        {
            Up.currState = idle;
        }
    }
    else if (Down.currState == firstHeld && (progFlag == 1 || suFlag == 1))
    {
        if(P2IN & DOWN_BUTTON)
        {
            fastDecreaseVal(SLOWER_INCREMENT_SPEED, 10);
            if (suFlag == 1)
                applyChanges();
        }
        else
        {
            Down.currState = idle;
        }
    }
    else if (Down.currState == held && (progFlag == 1 || suFlag == 1))
    {
        if(P2IN & DOWN_BUTTON)
        {
            fastDecreaseVal(INCREMENT_SPEED, 1);
            if (suFlag == 1)
                applyChanges();
        }
        else
        {
            Down.currState = idle;
        }
    }
    else if (Down.currState == pressed && (progFlag == 1 || suFlag == 1))
    {
        decreaseVal();
        if (suFlag == 1)
            applyChanges();
        Down.currState = idle;
    }
    else if (Down.currState == pressed && progFlag != 1)
    {
        switchPreset(1);
        blinkTick = timerTick125; // Reset time between blinks if button is pressed
        Down.currState = idle;
    }
}

/****************** Up/Down button actions ******************/

void increaseVal(void)
{
    if (valType == frequency)
    {
        currentFreq = ((presetFreq[presetNum] * 10) + 1) / 10;    // Math done this way to prevent the need for decimal calculations, much less memory usage
        presetFreq[presetNum] = currentFreq;
        separateDigits(currentFreq, currentDc);  // Reruns seperateDigits to reassign 7SD digits but NOT change PWM
    }
    else if (valType == dutyCycle)
    {
        presetDc[presetNum]++;
        currentDc = presetDc[presetNum];
        separateDigits(currentFreq, currentDc);  // Reruns seperateDigits to reassign 7SD digits but NOT change PWM
    }
}

void fastIncreaseVal(unsigned int incrementSpeed, unsigned char increment)
{
    static unsigned int LastTick = 0;
    if ((timerTick125 - LastTick) >= incrementSpeed)
    {
        LastTick = timerTick125;
        if (valType == frequency)
        {
            currentFreq = ((presetFreq[presetNum] * 10) + increment) / 10;
            presetFreq[presetNum] = currentFreq;
            separateDigits(currentFreq, currentDc);
        }
        else if (valType == dutyCycle)
        {
            currentDc = (presetDc[presetNum] + increment);
            presetDc[presetNum] = currentDc;
            separateDigits(currentFreq, currentDc);
        }
    }
}

void decreaseVal(void)
{
    if (valType == frequency)
    {
        currentFreq = ((presetFreq[presetNum] * 10) - 1) / 10;    // Math done this way to prevent the need for decimal calculations, much less memory usage
        presetFreq[presetNum] = currentFreq;
        separateDigits(currentFreq, currentDc);  // Reruns seperateDigits to reassign 7SD digits but NOT change PWM
    }
    else if (valType == dutyCycle)
    {
        presetDc[presetNum]--;
        currentDc = presetDc[presetNum];
        separateDigits(currentFreq, currentDc);  // Reruns seperateDigits to reassign 7SD digits but NOT change PWM
    }
}

void fastDecreaseVal(unsigned int incrementSpeed, unsigned char increment)
{
    static unsigned int LastTick = 0;
    if ((timerTick125 - LastTick) >= incrementSpeed)
    {
        LastTick = timerTick125;
        if (valType == frequency)
        {
            currentFreq = ((presetFreq[presetNum] * 10) - increment) / 10;
            presetFreq[presetNum] = currentFreq;
            separateDigits(currentFreq, currentDc);
        }
        else if (valType == dutyCycle)
        {
            currentDc = (presetDc[presetNum] - increment);
            presetDc[presetNum] = currentDc;
            separateDigits(currentFreq, currentDc);
        }
    }
}

