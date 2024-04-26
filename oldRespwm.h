/*
    Ethan Van Deusen
    Modifiable Timing Light
    pwm.h
 */
#ifndef PWM_H_
#define PWM_H_

#include <msp430.h>

#define xternCrystalSpeed  32768  // External crystal speed, used in PWM calculations
#define DCOSpeed  16000000        // Internal DCO speed, used in PWM calculations

extern unsigned char prescl;
extern char presetNum;

extern float presetFreq[];
extern float presetDc[];

// Function Prototypes
extern unsigned int periodCalc(float freq, unsigned int prescaler);
extern unsigned int dcCalc(float dc, unsigned int period);
extern void pwm(void);

/********************************* PWM Setup *********************************/
// Represents user inputs for frequency and duty cycle
    extern float currentFreq;
    extern int currentDc;

#endif /* PWM_H_ */
