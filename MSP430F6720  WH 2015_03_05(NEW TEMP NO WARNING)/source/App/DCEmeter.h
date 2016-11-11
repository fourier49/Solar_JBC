

#ifndef __DCEMETER_H__
#define __DCEMETER_H__


//*****************************************************************************
//
// DC Emeter Sturct 
//
//*****************************************************************************



typedef struct Emeter_Val
{
	
	int64_t I_sq;
	int64_t V_sq;
	
	int64_t log_I_sq;
	int64_t log_V_sq;

	
	uint16_t ISample_CNT;
	uint16_t VSample_CNT;	
	
	uint64_t pureVrms;
	uint64_t pureIrms;
	uint32_t VrmsRaw;
	uint32_t IrmsRaw;
	
	uint32_t DigiVrms;
	uint32_t ShuntVol;
	uint32_t DigiIrms;
	uint64_t DigiavgPower;
	
	uint8_t SampleDoneFlag;
	
	
	//
	// Temperature Measurement
	int16_t Diode_Temp;
	
	
	//
	// Random Number
	uint32_t Random_Number;
	
}Emeter;

//*****************************************************************************
//
// API Functions Prototype
//
//*****************************************************************************
Emeter *Get_DcEmeter (void);

#endif //  __DCEMETER_H__
