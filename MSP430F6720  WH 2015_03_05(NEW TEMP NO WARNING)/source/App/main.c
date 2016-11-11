#include "configuration.h" // configuration of environment file
#include "Bus_Raw_Protocol.h"
#include "source/App/DebugUART.h"
#include "source/App/DCEmeter.h"
//#include "ToolKit.h"
//#include <msp430f6720.h>
/******** SD24B ********/
uint32_t sample_V,sample_I;       // SD24B Sample
uint64_t accSample_V,accSample_I; // accumlate ADC Sample
uint16_t V_rms,I_rms;             // V and I of RMS value
uint32_t P_rms;                   // P of RMS value
uint32_t res1;                    // temp for RES1
uint64_t res2;                    // temp for RES2
uint16_t sd24bmemL;               // temp for SD24BMEML
uint32_t sd24bmemH;               // temp for SD24BMEMH

/******** ADC10_A ********/
uint16_t sample_Temp;             // ADC10 Sample
int32_t adcresult_Temp;           // ADC10 Sample convert
int16_t result_Temp;              // Temperature value

                              //Decode_Flag = 1 when read Tail_Byte
/******** Struct ********/
struct JB_PowerInfo pvPowerInfo;          // PV_Value Struct
//static uint16_t RandomDelay;

/******** RESETTIMER ********/
uint16_t RESETTIMER;                // RESETTIMER
void main(void)
{
        RESETTIMER = 60;
	Emeter *pDcEmeter;
  	const unsigned char Str[] = "!!! Reset !!! \n\r";
  	SystemInit(); //System Init

  	Init_Bus_Loacal_Device_State_Struct();

  	__bis_SR_register(GIE); //
#if 0
#define INFOD_START   (0x1800)
        FLASH_segmentErase((uint8_t*)INFOD_START);
        
#endif  
  	DB_Burst_Rs485_Tx( (unsigned char *)Str, sizeof(Str));
	
	//
	// Get Pointer of Emeter Struct
	pDcEmeter = Get_DcEmeter();
  	while(1)
  	{  
    
		//RandomDelay = (uint16_t ) urand()%500;
	
		//
		// Load DC Emeter 
		if ( pDcEmeter->SampleDoneFlag == 1)
		{
			Cal_DcEmeter_RMS_Value ();
			Update_PV_Values();
		}
	

    	if(Decode_Flag)
    	{
     		Decode_Flag = 0;
      		Decode_Proccess();
      
    	}
		
		Check_Registed_TimeOut();
  	}
  
}

