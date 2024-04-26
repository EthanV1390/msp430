/*
    Ethan Van Deusen
    Modifiable Timing Light
    flash.h
 */
#ifndef FLASH_H_
#define FLASH_H

// Function Prototypes
extern void flashLoad(void);
extern void setDefaults(void);
extern void flashWrite(float array[]);
extern void prepForFlash(float array[]);
extern void unloadFromFlash(unsigned int array[]);

#endif /* FLASH_H_ */
