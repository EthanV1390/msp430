/*******************************************************************************
* uart.h
*
* Copyright 2016 ITE & BMI as an unpublished work.
* Rubicon Dispenser
* All Rights Reserved.
*
* 7/11/16, Rev 1, Streicher / ITE
*   
********************************************************************************

*******************************************************************************/

#ifndef _UART_H
#define _UART_H

/*******************************************************************************
* INCLUDE FILES (all #include statements)
*******************************************************************************/

#include <MSP430x21x2.h>     		
#include <stdio.h>
#include "bmi.h"


/*******************************************************************************
* CONSTANTS AND MACRO DEFINITIONS (#define statements)
*******************************************************************************/

/*******************************************************************************
* TYPEDEFS, STRUCTURES, AND ENUM DECLARATIONS
*******************************************************************************/


/*******************************************************************************
* DATA DECLARATIONS (storage class is extern)
*******************************************************************************/


/*******************************************************************************
* FUNCTION DECLARATIONS (prototypes)
*******************************************************************************/
#ifdef DEBUG_UART 
    extern void UARTSetupUART(void);
    extern void UARTSendData(void);
#endif 
////extern void UARTSendConfig(void);
//extern void UARTDisableRx(void);
//extern void UARTEnableRx(void);
//extern void UARTTransmitBytes(unsigned int uiNumBytes, unsigned char *pucByte);
//extern void UARTProcessRx(void);
#endif
