#include "configuration.h"

#include "source/App/ProtocolTimer.h"

#pragma data_alignment=2
static volatile uint8_t BRD_Timer_Lock = 0;
#pragma data_alignment=2
static volatile uint16_t RTCCounter = 0;
#pragma data_alignment=2
static volatile uint8_t Clear_Regist_Flag = 0;



//
// RTC
unsigned char data1[16]="00:00:00        ";//****
//*****************************************************************************
//	RTC interrupt service routine
// 	- 
//*****************************************************************************	
/*#pragma vector=RTC_VECTOR
__interrupt void RTC_ISR(void)
{

  while(1);
  
   counter++;
   
   if(counter==60)
   {
    min++;
    counter=0;
    if(min==60)
    {
     hour++;
     min=0;
     if(hour==24)
     {
      hour=0;
      data2[9]= data2[9]+1;
     }
    }
   }
   
   
   p=counter%10;
   data1[7]=p+0x30;
   p=counter/10;
   data1[6]=p+0x30;
   
   p=min%10;
   data1[4]=p+0x30;
   p=min/10;
   data1[3]=p+0x30;
   
   p=hour%10;
   data1[1]=p+0x30;
   p=hour/10;
   data1[0]=p+0x30;
   
   
   
   data1[2]=':';
   data1[5]=':';
   
   lcdwda(0,1,data1);
   lcdwda(0,0,data2);
   RTCCTL01 &= ~RTCTEVIFG;
   RTCPS1CTL &= ~RT1PSIFG;
  
  
}*/

//static volatile uint8_t RTCCounter = 0;
//static volatile uint8_t Clear_Regist_Flag = 0;
#pragma vector=TIMER3_A0_VECTOR
__interrupt void TIMER3_A0_ISR(void)
{
 	//
  	// Stop The Timer 
  	//TA3CTL = TASSEL_2|MC_0|TACLR;	
	
	//
	// Clear Interrupt Pending
  	RTCCounter++;
	
	
	if ( RTCCounter>= RESETTIMER)
	{
		Clear_Regist_Flag = 1;
		RTCCounter = 0;
	}
	  
}

void Reset_ClearRegistFlag_Counter (void)
{
	RTCCounter = 0;
	Clear_Regist_Flag = 0;
}

uint8_t GET_ClearRegistFlag (void)
{
  	if ( Clear_Regist_Flag == 1)
	{
	  	RTCCounter = 0;
		Clear_Regist_Flag = 0;
		return 1;
	}
	//
	// Else
	return 0;
}

//*****************************************************************************
//	Timer2 A0 interrupt service routine
// 	-
//*****************************************************************************		
#pragma vector=TIMER2_A0_VECTOR
__interrupt void TIMER2_A0_ISR(void)
{
 	//
  	// Stop The Timer 
  	TA2CTL = TASSEL_2|MC_0|TACLR;	
	
	//
	// Clear Interrupt Pending
	
	//
	// Release Lock
	BRD_Timer_Lock = 1;

}

//*****************************************************************************
//	Get the state of lock of Bus Raw Protocol 
//	- 1 mSecond resolution.
//*****************************************************************************
uint8_t Get_BRD_TimerLock_State (void)
{
	if (BRD_Timer_Lock == 1)
	{
		BRD_Timer_Lock = 0;	// Reset the Lock.
		return 1;	// Lock is Release.
	}
	else
		return 0;	// Timer is Locked
}
//*****************************************************************************
//	Start Bus Raw Data Protocol Timer
//	- 1 mSecond resolution.
//  - 32.768kHz Clock Source.
//*****************************************************************************
void Start_BRD_Timer (uint16_t mSec)
{
	
	//
	// Stop Timer
	TA2CTL = TASSEL_1|MC_0; // Stop Timer
				
	//
	// Load Value
	TA2CCR0 = mSec*BRD_Timer_1mSec;
	
	//
	// Start Timer
  	TA2CTL = TASSEL_1|	// Clk Source ACK
			 MC_1|		// Start
			 TACLR;		// Clear
			 
	//
	// Lock 
	BRD_Timer_Lock = 0;
}

//*****************************************************************************
//	Stop Bus Raw Data Protocol Timer
//*****************************************************************************
void Stop_BRD_Timer (void)
{
	TA2CTL = TASSEL_2|MC_0|TACLR;		
}

//*****************************************************************************
//	Initialize BRD Protocol Timer
//	- RTC Timer
//  - 1 mins timeout
//*****************************************************************************
void Init_Timer_For_BRDProtocol (void)
{
  	//
  	// Configure Timer
    TA2CTL = TASSEL_1|	// Use 32.768kHz as Clock Source
			 MC_0|		// Stop Timer
			 TACLR;	
  
	//
	// Enable Compared Interrupt 
    TA2CCTL0 = CCIE;                        // Compared interrupt enabled
    TA2CCR0 = BRD_Timer_1mSec;
}



//*****************************************************************************
//	Initialize RTC Timer
//	- RTC Timer
//*****************************************************************************
void Init_RTC (void)
{
  	//
  	// Unlock RTC Module
    /*RTCCTL0_H = RTCKEY;
	
	
    RTCCTL0_L = RTCTEVIE_L;	 // Enable RTC event interrupt

	
	RTCCTL13_H = RTCCALF1_H|RTCCALF0_H; // Frequency of RTC Calibration 1 Hz
	RTCCTL13_L = RTCMODE_L|				// Calendar Mode
	  			 RTCTEV__MIN;			// Time Event- Mins Changed.
	*/
	
   	//RTCPS0CTL = RT0PSDIV_7;                   // Set RTPS0 to /256
  
    	//
  	// Configure Timer
    TA3CTL = TASSEL_1|	// Use 32.768kHz as Clock Source
			 MC_1|		// Stop Timer
			 TACLR;	
  
	//
	// Enable Compared Interrupt 
    TA3CCTL0 = CCIE;                        // Compared interrupt enabled
    TA3CCR0 = ACLK_Rate;
  	
}


