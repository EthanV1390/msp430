/*
    Ethan Van Deusen
    Modifiable Timing Light
    flash.c

    Handles writing and reading data into flash memory. All data is stored as 10 times their value and then
    divided by 10 when read from flash, allows storing of the tens place.

    Frequency values for preset 1-3: Block B, 0x1080 - 0x1085, CheckSum stored at 0x1086
    Duty Cycle values for preset 1-3: Block C, 0x1040 - 0x1045, CheckSum stored at 0x1046
 */

#include <msp430.h>
#include "buttons.h"
#include "main.h"
#include "pwm.h"
#include "display.h"
#include "flash.h"

#define FLASH_SECTOR_B  (0x1080)    // Address of flash sector B, Frequency
#define FLASH_SECTOR_C  (0x1040)    // Address of flash sector C, Duty Cycle
#define FLASH_SECTOR_D  (0x1000)    // Address of flash sector D, unused
#define DATA_NUM_BYTES 3            // Array size - 1, Preset 1-3

unsigned int flashBuffer[3];            // Buffer that holds the int values of the starting array (multiplied by 10 and divided by 10 after read from flash)
static unsigned int calcCheckSum = 0;   // Recalculated checkSum value to compare against the checkSum calculated when data is written to flash
static unsigned int savedCheckSum;      // Saved checksum value

typedef enum
{
    NO_ERROR = 0,
    ERROR
} FlashError;   // Flash error status

FlashError setupFlash(void) // This routine sets up for a flash erase/write. The watchdog is turned off, the DCO is set to 1MHz, and the flash timing generator is set
{
    FlashError flashError = NO_ERROR;         // Assume no flash error
    WDTCTL = WDTPW + WDTHOLD;                 // Stop watchdog timer

    if (CALBC1_1MHZ == 0xFF || CALDCO_1MHZ == 0xFF)
    {
        flashError = ERROR;                   // Clock values bad
    }
    else        // No error, set the clock
    {
        BCSCTL1 = CALBC1_1MHZ;
        DCOCTL = CALDCO_1MHZ;
        FCTL2 = FWKEY + FSSEL0 + FN1;         // MCLK/3 for Flash Timing Generator
    }
    return (flashError);
}

void prepForFlash(float array[])    // Converts float values into ints for loading/unloading into flash
{
    char i;
    for(i = 0; i < 3; i++)
    {
        flashBuffer[i] = (unsigned int)(array[i] * 10);
    }
}

void unloadFromFlash(unsigned int array[])  // Takes int array from flash and converts it back to the starting float values
{
    char i;
    float floatVal;
    for(i = 0; i < 3; i++)
    {
        floatVal = array[i];
        if(valType == frequency)
            presetFreq[i] = (floatVal/10);
        else if(valType == dutyCycle)
            presetDc[i] = (floatVal/10);
    }
}

void setDefaults(void)  // When checkSum fails or default button action, reset preset values to default
{
    if(valType == frequency)
    {
        presetFreq[0] = 1;
        presetFreq[1] = 60;
        presetFreq[2] = 500;
        presetFreq[3] = .1;
        flashWrite(presetFreq); // Writes into flash
    }
    else if(valType == dutyCycle)
    {
        presetDc[0] = 50;
        presetDc[1] = 50;
        presetDc[2] = 75;
        presetDc[3] = 50;
        flashWrite(presetDc);  // Writes into flash
    }
    applyChanges();
}

void startWriteFlash(unsigned int blockAddress)
{
    unsigned char *eraseByte;                     // Pointer used to erase blocks
    eraseByte = (unsigned char *)blockAddress;    // Initialize Flash pointer
    FCTL3 = FWKEY + LOCKA;                        // Clear B,C,D Lock bit, keep Lock A
    FCTL1 = FWKEY + ERASE;                        // Set Erase bit
    *eraseByte = 0;                               // Dummy write to erase Flash seg
    FCTL1 = FWKEY + WRT;                          // Set WRT bit for write operation
}

void flashWrite(float array[])  // Writes data into flash
{
    unsigned int *ramByte;          // Pointer for data in RAM
    unsigned int *flashByte;        // Pointer for data in Flash
    unsigned int blockAddress;
    unsigned char byteCount;

    unsigned int checkSum = 0;          // Local checkSum variable
    FlashError flashError = NO_ERROR;   // Assume no error yet

    TA1CCTL0 = 0;                       // Halts timer interrupts
    flashError = setupFlash();          // Setup flash and check for error

    if (flashError == NO_ERROR)
    {
        if(valType == frequency)                // Selects block B or C, B for Frequency and C for Duty Cycle
            blockAddress = FLASH_SECTOR_B;
        else if(valType == dutyCycle)
            blockAddress = FLASH_SECTOR_C;

        prepForFlash(array);
        startWriteFlash(blockAddress);                     // Start block write
        ramByte = (unsigned int*) &flashBuffer[0];         // Initializes ramByte to hold address of data in ram   *ramByte = 100
        flashByte = (unsigned int*) blockAddress;          // Initializes flashByte to hold address of where data will go in flash

        for (byteCount = 0; byteCount < DATA_NUM_BYTES; byteCount++)
        {
            *flashByte = *ramByte;                   // Copies data from ram into flash
            checkSum += *ramByte;                    // Adds value to checkSum
            flashByte++;                             // Increment the flash address, *flashByte where the next bit of data is stored
            ramByte++;                               // Increment the data address,  *ramByte is where the next bit of data is copied from
        }
        // Save checksum
        ramByte++;
        *ramByte = checkSum;                         // Sets ramByte to the calculated checkSum value
        *flashByte = *ramByte;                       // Assigns flash address (One location after data stored)
        savedCheckSum = *ramByte;                    // Saves checkSum that was just calculated for later checking

        FCTL1 = FWKEY;                               // Clear WRT bit
        FCTL3 = FWKEY + LOCK + LOCKA;                // Set all LOCK bits
    }

    TA1CCTL0 |= CCIE;  // Re-enable main system ticker interrupts
    setupTimerA1();    // Set clock back
    setupTimerA0();    // Set clock back

    // Verify checksum
    flashByte = (unsigned int *) blockAddress;    // Set address to use for the start of check
    calcCheckSum = 0;   // Clear calcCheckSum value

    for (byteCount = 0; byteCount < DATA_NUM_BYTES; byteCount++)
    {
        calcCheckSum += *flashByte;         // Recalculates the checkSum of the data, should be the same as the value stored in savedCheckSum.
        flashByte++;
    }
    if (calcCheckSum != savedCheckSum)      // Compares calcCheckSum (Calculated when data is written) and savedCheckSum (Calculated when double checked above)
    {
        setDefaults();
    }
}

void flashLoad(void)
{
    unsigned int *ramByte;          // Pointer for data in RAM
    unsigned int *flashByte;        // Pointer for data in Flash
    unsigned char byteCount;
    unsigned int blockAddress;

    if(valType == frequency)            // Selects block B or C, B for Frequency and C for Duty Cycle
        blockAddress = FLASH_SECTOR_B;
    else if(valType == dutyCycle)
        blockAddress = FLASH_SECTOR_C;

    flashByte = (unsigned int *) blockAddress;       // Set address to start of the block in flash where the data will be read from
    ramByte = (unsigned int*) &flashBuffer[0];       // Set pointer to beginning of where data will go in ram from flash

    for (byteCount = 0; byteCount < DATA_NUM_BYTES; byteCount++)
    {
        *ramByte = *flashByte;                       // Get the current byte from flash
        ramByte++;
        flashByte++;
    }
    unloadFromFlash(flashBuffer);
}



