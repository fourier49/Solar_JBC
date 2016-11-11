

#ifndef __PROTOCOLTIMER_H__
#define __PROTOCOLTIMER_H__



#define ACLK_Rate	32768	// Hz
#define BRD_Timer_1mSec	ACLK_Rate/1000
//*****************************************************************************
//
// API Functions Prototype
//
//*****************************************************************************
void Start_BRD_Timer (uint16_t mSec);
void Init_Timer_For_BRDProtocol (void);
void Stop_BRD_Timer (void);
uint8_t Get_BRD_TimerLock_State (void);



//
//
void Init_RTC (void);
uint8_t GET_ClearRegistFlag (void);
void Reset_ClearRegistFlag_Counter (void);
#endif //  __PROTOCOLTIMER_H__
