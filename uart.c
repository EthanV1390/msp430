/*******************************************************************************
* uart.c
*
* Copyright 2016 ITE & BMI as an unpublished work.
* Rubicon Dispenser
* All Rights Reserved.
*
* 7/11/16, Rev 1, Streicher / ITE
*
********************************************************************************

*******************************************************************************/

/*******************************************************************************
* INCLUDE FILES (all #include statements)
*******************************************************************************/
#include <stdlib.h>
#include <stdbool.h>
#include "msp430x21x2.h"
#include "uart.h"
#include "bmi.h"
#include "cal.h"  
#include "ad.h"

/*******************************************************************************
* INTERNAL CONSTANTS AND MACRO DEFINITIONS
* (#define statements for definitions only used in this file)
*******************************************************************************/ 

#pragma location = 0x0E000
const unsigned char uc_FwRevMajor = FW_REV_MAJOR;
#pragma location = 0x0E001
const unsigned char ucFwRevMinor = FW_REV_MINOR;



/*******************************************************************************
* INTERNAL TYPEDEFS, STRUCTURES, AND ENUM DECLARATIONS
* (declarations only used in this file)
*******************************************************************************/

/*******************************************************************************
* EXTERNAL DATA DEFINITIONS (storage class is blank)
* (definitions for global variables declared as extern in .h file)
*******************************************************************************/

    
/*******************************************************************************
* PRIVATE FUNCTION DECLARATIONS & DEFINITIONS (static)
* (functions only called from within this file)
*******************************************************************************/


/*******************************************************************************
* PUBLIC FUNCTION DEFINITIONS (storage class is blank)
* (functions called from outside this file, prototypes in .h file)
*******************************************************************************/

#ifdef DEBUG_UART
void UARTSetupUART(void)
/*******************************************************************************
* DESCRIPTION: This function sets up the uart for communication, sourced from
*               system 8MHz clock, baud = 19200, 8-bit, one stop, no parity
*
* INPUTS: None
*
* RETURNS: None
*
*******************************************************************************/
{
  P3SEL = 0x30;                             // P3.4,5 = USCI_A0 TXD/RXD
  UCA0CTL1 |= UCSSEL_2;                     // SMCLK
//  UCA0BR0 = 160;                            // 8MHz/19200 = ~416.6
//  UCA0BR1 = 1;                              //
  UCA0BR0 = 65;                            // 8MHz/9600 = ~833
  UCA0BR1 = 3;                              //
  UCA0MCTL = UCBRS2 + UCBRS1;               // Modulation UCBRSx = 6
  UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
//  IE2 |= UCA0RXIE;                          // Enable USCI_A0 RX interrupt
}
#endif

#ifdef DEBUG_UART    
void UARTTransmitBytes(unsigned int uiNumBytes, unsigned char *pucByte)
/*******************************************************************************
* DESCRIPTION: This function transmits the bytes of any given variable or 
*               structure.
*
* INPUTS: uiNumBytes - number of bytes of variable/structure
*           pucByte - pointer to starting address of variable/structure
*
* RETURNS: None
*
*******************************************************************************/
{
    unsigned char ucSampleIndex = 0;

    while (ucSampleIndex < uiNumBytes )
    {
        if ((UCA0STAT & UCBUSY) == 0)
        {
            UCA0TXBUF = *pucByte;
            ucSampleIndex++;
            pucByte++;
        }
    }    
} 
#endif

#ifdef DEBUG_UART      
void UARTSendData(void)
/*******************************************************************************
* DESCRIPTION: This function sends out data over the UART
*
* INPUTS: None
*
* RETURNS: None
*
*******************************************************************************/
{
    UARTTransmitBytes(2,(unsigned char*) &iTemperature10xC);
}
#endif


//void UARTDisableRx(void)
///*******************************************************************************
//* DESCRIPTION: This function disables incomming Rx messages for the UART
//*
//* INPUTS: None
//*
//* RETURNS: None
//*
//*******************************************************************************/
//{
//    ucUartBytesReceived = 0;                    /* zero buffer pointer */
//    IE2 &= ~UCA0RXIE;                          // Enable USCI_A0 RX interrupt  
//}

//void UARTEnableRx(void)
///*******************************************************************************
//* DESCRIPTION: This function enables incomming Rx messages for the UART
//*
//* INPUTS: None
//*
//* RETURNS: None
//*
//*******************************************************************************/
//{
//    IE2 |= UCA0RXIE;                          // Enable USCI_A0 RX interrupt  
//    UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
//    ucUartBytesReceived = 0;                    /* zero buffer pointer */
//}



/******************************************************************************/
//                 Interrupts
/******************************************************************************/

#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
{
//    /* save new byte received to ad buffer - don't have anymore spare memory.
//    this is OK as we don't use the ad while the door is open, and that's the
//    only time we recieve any messages */ 
//    /* use temporary variable to clear volatile access warning */
//    unsigned char ucCurrentByte = ucUartBytesReceived;
////    uiAd_Data[ucUartBytesReceived] = UCA0RXBUF; 
//    uiUART_Data[ucCurrentByte] = UCA0RXBUF; 
//    if (ucUartBytesReceived < (UART_BUFFER_SIZE - 1))
//        ucUartBytesReceived++;                              /* bump counter */
//        
////    if (operatingMode != MODE_AWAKE)
////    {
////        __bic_SR_register_on_exit(LPM3_bits);
////        WakeUp();            /* housekeeping upon wakeup from low power mode */
////    }
//                    
}

#pragma vector=USCIAB0TX_VECTOR
__interrupt void USCI0TX_ISR(void)
{
//    if (operatingMode != MODE_AWAKE)
//    {
//        __bic_SR_register_on_exit(LPM3_bits);
//        WakeUp();            /* housekeeping upon wakeup from low power mode */
//    }
}


