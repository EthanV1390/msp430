/*
    Ethan Van Deusen
    Modifiable Timing Light
    display.h
 */
#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <msp430.h>

/********************************* Display Setup *********************************/

#define DIGIT_ON_TIME 8
#define DIGIT_OFF_TIME 1
#define BLINK_TIME 24000          // 3 Sec
#define PROG_BLINK_TIME 4000      // 0.5 Sec

//Variables for easily setting a digit's character from ucCharToHex[]
volatile static unsigned char digit1Val;
volatile static unsigned char digit2Val;
volatile static unsigned char digit3Val;
volatile static unsigned char digit4Val;

volatile static unsigned char digit1Blink;
volatile static unsigned char digit2Blink;
volatile static unsigned char digit3Blink;
volatile static unsigned char digit4Blink;

extern unsigned int decShown;       // Adds a decimal to a digit
extern unsigned int pmShown;        // Displays 'pm' on last digit, used to indicate switching to program mode
extern unsigned int degShown;       // Displays degrees symbol, used to indicate single use editing mode

extern volatile unsigned char blinkStatus;
extern volatile unsigned int blinkTick;       // Blink timer counter
extern volatile unsigned int blinkTime;

extern volatile unsigned char progFlag;       // Flag when device is in programming mode
extern volatile unsigned char suFlag;         // Flag when device is in single use editing mode

extern const unsigned char ucCharToHex[];
/*  ucCharToHex index
  0: (0) 0x3F    10: (0)  0x3F   20: (0.)  0xBF
  1: (1) 0x06    11: (C)  0x39   21: (-)   0x40
  2: (2) 0x5B    12: (F)  0x71   22: (p)   0xF3
  3: (3) 0x4F    13: (H)  0x76   23: ( )   0x0
  4: (4) 0x66    14: (r)  0x31   24: (c)   0x58
  5: (5) 0x6D    15: (O)  0x3F
  6: (6) 0x7D    16: (n)  0x54
  7: (7) 0x07    17: (E)  0x79
  8: (8) 0x7F    18: (d)  0x5E
  9: (9) 0x6F    19: (y)  0x6E
*/
typedef enum
{
    dispInit,        // Initial state
    dispDigit1,      // Anode 1, digit 1
    dispDigit2,      // Anode 2, digit 2
    dispDigit3,      // Anode 3, colon, degree symbol, and LED's
    dispDigit4,      // Anode 4, digit 3
    dispDigit5       // Anode 5, digit 4
} DisplayState;
volatile static DisplayState dispState = dispInit;

typedef enum
{
    dutyCycle,
    frequency,
} valDisplayed;
volatile valDisplayed valType;

// Function prototypes
extern void progModeChanges(void);
extern void presetInfo(void);
extern void separateDigits(float freq, float dc);
extern void switchPreset(int direction);
extern void switchValType(void);
extern void updateDisplay(void);
extern void singleUseMode();

#endif /* DISPLAY_H_ */
