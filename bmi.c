void UpdateDisplay(void )
/*******************************************************************************
* DESCRIPTIION: Services all of the raw display LED's 
*               During initial testing, with basically no other code running,
*               this routine hits every 37us.
*
*               9/1/16 - routine take 6.4us - 9.6us to execute      
*               going to move into interrupts
*
* INPUTS: None
*
* RETURNS: None
*
*******************************************************************************/
{
    static unsigned int uiLastTick = 0;

//P1OUT |= DEBUG_P15;

///* DEBUG */
//if (ucDisplayHexValue[2] != 0)
//{
//    P1OUT &= ~DEBUG_P15;
//}
//else
//{
//   P1OUT |= DEBUG_P15; 
//}

/* 01/10/17 - display still occationally shows digit on power down or up. Force to init mode
if we go into sleep mode */
    if (operatingMode == MODE_SLEEP)
    {
        displayState = DISP_STATE_INIT;
    }
    
    /* display update state machine */
    switch (displayState)
    {
        case DISP_STATE_INIT:
        default:
        {
            /* init led's off so when we wake they're off */
            P1OUT &= ~ (A1_CTRL | A2_CTRL | A3_CTRL | A4_CTRL | A5_CTRL);
            P3OUT &= ~(A_CTRL | B_CTRL | C_CTRL | D_CTRL | E_CTRL | F_CTRL | G_CTRL | DP_CTRL);
            uiLastTick = uiTimerTick_125us;
            displayState = DISP_STATE_A1;
        }
        break;
        
        case DISP_STATE_A1:      /* anode 1, this is the 7-segment MSB, digit 1 */
        {  
            /* must wait a bit before turning off the anode in the last state to turning on here, because if we don't we get 
            "bleed over" from the next states's cathode setting while still on the last states anode */
            /* this will add 250us delay */
#ifdef DIMMABLE
            if ((uiTimerTick_125us - uiLastTick) >= ucDigitOffTimeCt)
#else
            if ((uiTimerTick_125us - uiLastTick) >= DISP_DIGIT_OFF_TIME_CT)
#endif
            {
                /* set cathodes */
                P3OUT = ucDisplayHexValue[0];                
                /* anode 1 */
                P1OUT |= A1_CTRL; 
            }
            
            if ((uiTimerTick_125us - uiLastTick) >= DISP_DIGIT_ON_TIME_CT)
            {
                /* go to next digit */
                uiLastTick = uiTimerTick_125us;
                displayState = DISP_STATE_A2;

                /* turn off anode 1 */
                P1OUT &= ~A1_CTRL;             
            }
        }
        break;
        
        case DISP_STATE_A2:      /* anode 2, this is the 7-segment digit 2 */   
        {
            /* must wait a bit before turning off the anode in the last state to turning on here, because if we don't we get 
            "bleed over" from the next states's cathode setting while still on the last states anode */
            /* this will add 250us delay */
#ifdef DIMMABLE
            if ((uiTimerTick_125us - uiLastTick) >= ucDigitOffTimeCt)
#else
            if ((uiTimerTick_125us - uiLastTick) >= DISP_DIGIT_OFF_TIME_CT)
#endif
            {
                /* set cathodes */
                P3OUT = ucDisplayHexValue[1];                
                /* anode 2 */
                P1OUT |= A2_CTRL;                
            }
            
            
            if ((uiTimerTick_125us - uiLastTick) >= DISP_DIGIT_ON_TIME_CT)
            {
                /* turn off anode 2 */
                P1OUT &= ~A2_CTRL;
                
                /* go to next digit */
                uiLastTick = uiTimerTick_125us;
                displayState = DISP_STATE_A3;
            }            
        }
        break;
        
          
        case DISP_STATE_A3:      /* anode 3, this is the 7-segment colon, degree symbol, and discrete LED's */
        {
            /* must wait a bit before turning off the anode in the last state to turning on here, because if we don't we get 
            "bleed over" from the next states's cathode setting while still on the last states anode */
            /* this will add 250us delay */
#ifdef DIMMABLE
            if ((uiTimerTick_125us - uiLastTick) >= ucDigitOffTimeCt)
#else
            if ((uiTimerTick_125us - uiLastTick) >= DISP_DIGIT_OFF_TIME_CT)
#endif
            {
                /* set cathodes */             
                P3OUT = ucDisplayHexValue[2];    
/* DEBUG FLICKER */
//P3OUT = 0X04;           
                /* anode 3 */
                P1OUT |= A3_CTRL;  
            }
            
            if ((uiTimerTick_125us - uiLastTick) >= DISP_DIGIT_ON_TIME_CT)
            {
                /* go to next digit */
                uiLastTick = uiTimerTick_125us;
                displayState = DISP_STATE_A4;
                
                /* turn off anode 3 */
                P1OUT &= ~A3_CTRL;
            }            
        }
        break;
        
        case DISP_STATE_A4:      /* anode 4, this is the 7-segment digit 3 */
        {
            /* must wait a bit before turning off the anode in the last state to turning on here, because if we don't we get 
            "bleed over" from the next states's cathode setting while still on the last states anode */
            /* this will add 250us delay */
#ifdef DIMMABLE
            if ((uiTimerTick_125us - uiLastTick) >= ucDigitOffTimeCt)
#else
            if ((uiTimerTick_125us - uiLastTick) >= DISP_DIGIT_OFF_TIME_CT)
#endif
            { 
                /* set cathodes */
                P3OUT = ucDisplayHexValue[3];         
                /* anode 4 */
                P1OUT |= A4_CTRL;                              
            }
            
            if ((uiTimerTick_125us - uiLastTick) >= DISP_DIGIT_ON_TIME_CT)
            {
                /* go to next digit */
                uiLastTick = uiTimerTick_125us;
                displayState = DISP_STATE_A5;
                
                /* turn off anode 4 */
                P1OUT &= ~A4_CTRL;
            }            
        }
        break;
        
        case DISP_STATE_A5:       /* anode 5, this is the 7-segment LSB, digit 4 */
        {
            /* must wait a bit before turning off the anode in the last state to turning on here, because if we don't we get 
            "bleed over" from the next states's cathode setting while still on the last states anode */
            /* this will add 250us delay */
#ifdef DIMMABLE
            if ((uiTimerTick_125us - uiLastTick) >= ucDigitOffTimeCt)
#else
            if ((uiTimerTick_125us - uiLastTick) >= DISP_DIGIT_OFF_TIME_CT)
#endif
            {  
                /* set cathodes */
                P3OUT = ucDisplayHexValue[4];                      
                /* anode 5 */
                P1OUT |= A5_CTRL;                    
            }
            
            if ((uiTimerTick_125us - uiLastTick) >= DISP_DIGIT_ON_TIME_CT)
            {
                /* go to next digit */
                uiLastTick = uiTimerTick_125us;
                displayState = DISP_STATE_A1;
                
                /* turn off anode 5 */
                P1OUT &= ~A5_CTRL;
            }            
        }
        break;
    }
    
//P1OUT &= ~DEBUG_P15;    
    
}
