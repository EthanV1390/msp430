/*
    Ethan Van Deusen
    Modifiable Timing Light
    buttons.h
 */
#ifndef BUTTONS_H_
#define BUTTONS_H_

#include <msp430.h>

// Constant wait macros
#define DEBOUNCE_TIME 10            // 1.25 ms
#define INCREMENT_SPEED 1000        // 125 ms
#define SLOWER_INCREMENT_SPEED 2000 // 250 ms
#define FASTER_INCREMENT_SPEED 500  // 62.5 ms

#define HELD_TIME 7000              // 875 ms, Always happens before any of the following hold times
#define FIRST_HELD_TIME 30000       // 3.75 Sec
#define SECOND_HOLD_TIME 50000      // 6.25 Sec
#define THIRD_HOLD_TIME 60000       // 7.5 Sec
// Button states
typedef enum
{
    idle,
    pressed,
    released,
    held,
    firstHeld,
    secondHold,
    thirdHold,
} ButtonState;

// Button struct
typedef struct
{
    volatile ButtonState currState;
    volatile ButtonState debounceState;
} Button;

// Function prototypes
extern void debounce(unsigned char button, volatile ButtonState *state, volatile ButtonState *debounceState);
extern void processSwitches(void);
extern void decreaseVal(void);
extern void increaseVal(void);
extern void fastDecreaseVal(unsigned int incrementSpeed, unsigned char increment);
extern void fastIncreaseVal(unsigned int incrementSpeed, unsigned char increment);

#endif /* BUTTONS_H_ */
