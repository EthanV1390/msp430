/*
    Ethan Van Deusen
    Modifiable Timing Light
    main.h
 */
#ifndef MAIN_H_
#define MAIN_H_

#include <msp430.h>

extern volatile unsigned int timerTick125;              // Timer counter

// Function Prototypes
extern void applyChanges(void);
extern void setupTimerA0(void);
extern void setupTimerA1(void);
extern void flashProcess(void);

/************* Port Configuration *************/
//Port 1, 7-segment display segment control
    #define A_CTRL  BIT0    // Segment A
    #define B_CTRL  BIT1    // Segment B
    #define C_CTRL  BIT2    // Segment C
    #define D_CTRL  BIT3    // Segment D
    #define E_CTRL  BIT4    // Segment E
    #define F_CTRL  BIT5    // Segment F
    #define G_CTRL  BIT6    // Segment G
    #define DP_CTRL BIT7    // Segment DP
//Port 2, Button control
    #define TOGGLE_BUTTON BIT1  // S2
    #define UP_BUTTON     BIT2  // S1
    #define DOWN_BUTTON   BIT3  // S3
    #define APPLY_BUTTON  BIT5  // S4
//Port 3, 7-segment display anode(digit) control
    #define A1_CTRL BIT0    // Digit 1
    #define A2_CTRL BIT1    // Digit 2
    #define A3_CTRL BIT2    // Colon, LED's, deg sign
    #define A4_CTRL BIT3    // Digit 3
    #define A5_CTRL BIT4    // Digit 4
#endif /* MAIN_H_ */
