#include "configuration.h"

#include "msp430Lib/emeter-toolkit.h"

#include "source/App/DCEmeter.h"


#define sqac_voltage                sqac64_24
#define sqac_current                sqac64_24
#define div_dc_voltage(y,x)         *y/x
#define div_dc_current(y,x)         *y/x

typedef union
{
    int16_t by16[2];
    int32_t by32;
} sh_lo_t;







static Emeter DcEmeter = {0};


Emeter *Get_DcEmeter (void)
{	
	return &DcEmeter;
}


//unsigned int Indx = 0;
//unsigned char Buf[20] = {0};



//*****************************************************************************
//
// Random Number Generator Seed Value
//
//*****************************************************************************
static uint32_t g_ulRandomSeed = 1;

//*****************************************************************************
//
//! Set the random number generator seed.
//!
//! \param ulSeed is the new seed value to use for the random number generator.
//!
//! This function is very similar to the C library <tt>srand()</tt> function.
//! It will set the seed value used in the <tt>urand()</tt> function.
//!
//! \return None
//
//*****************************************************************************
void
usrand(uint16_t ulSeed)
{
    g_ulRandomSeed = ulSeed;
}

//*****************************************************************************
//
//! Generate a new (pseudo) random number
//!
//! This function is very similar to the C library <tt>rand()</tt> function.
//! It will generate a pseudo-random number sequence based on the seed value.
//!
//! \return A pseudo-random number will be returned.
//
//*****************************************************************************
uint32_t  urand(void)
{
    //
    // Generate a new pseudo-random number with a linear congruence random
    // number generator.  This new random number becomes the seed for the next
    // random number.
    //
    g_ulRandomSeed = (g_ulRandomSeed * 1664525) + 1013904223+g_ulRandomSeed;

    //
    // Return the new random number.
    //
    return((uint32_t )g_ulRandomSeed);
}




 


// SD24B interrupt service routine
#pragma vector=SD24B_VECTOR
__interrupt void SD24BISR(void)
{
  	sh_lo_t val;


	int32_t CurrentSample, VoltageSample;
			
	if ((SD24BIFG & SD24IFG0))
	{
		val.by16[1] = (int16_t) SD24BMEMH0;
		val.by16[0] = (int16_t) SD24BMEML0;
		
		//
		// Load LSB to Random Seed
		usrand( (uint16_t) val.by16[0]);
		
		VoltageSample = val.by32;
		//
		// Clear Pending
		SD24BIFG &= ~SD24IFG0;
		
		//
		// Increase Sampler Counter
		(DcEmeter.VSample_CNT)++;
		
		//
		// Square and Accumulate
		sqac_voltage ( &(DcEmeter.V_sq), VoltageSample);
		

	}

	if ((SD24BIFG & SD24IFG1))
	{
		val.by16[1] = (int16_t) SD24BMEMH1;
		val.by16[0] = (int16_t) SD24BMEML1;
		
		CurrentSample = val.by32;
		SD24BIFG &= ~SD24IFG1;
		
		//
		// Increase Sampler Counter
		(DcEmeter.ISample_CNT)++;
		
		//
		// Median Filter
		
		//
		// Square and Accumulate
		sqac_current ( &(DcEmeter.I_sq), CurrentSample);
		
		
	}

	//
	// Saturation Test


	//
	// Filtering

	
	
	//
	// Logging  
	if ( (DcEmeter.ISample_CNT) == 1024 && (DcEmeter.VSample_CNT) == 1024)
	{
	  	
	  	DcEmeter.log_V_sq = DcEmeter.V_sq;
		DcEmeter.log_I_sq = DcEmeter.I_sq;
	  	
	  	DcEmeter.SampleDoneFlag = 1;
		
		//
		// Clear
		DcEmeter.ISample_CNT = 0;
		DcEmeter.VSample_CNT = 0;
		DcEmeter.V_sq = 0;
		DcEmeter.I_sq = 0;
		
		// -> Change this trigger to Timer Trigger and perform a IIR Filter feature.
		// Start ADC 10-bit Conversion.
		ADC10CTL0 |= ADC10ENC | ADC10SC;          // Sampling and conversion start*/
		
	}
	

	

}



void Cal_DcEmeter_RMS_Value (void)
{
	int64_t tmp;
        int16_t i;
	int64_t x;
	//
	// Vrms
	//
	tmp = div_dc_voltage( &(DcEmeter.log_V_sq), 1024);

	DcEmeter.pureVrms = isqrt64(tmp);

	//
	// Scaling
	DcEmeter.VrmsRaw = (uint32_t) ((DcEmeter.pureVrms)>>32);
	i = (1000*1.146);	// Ref = 1.146V , Display window to mV x1000
	x = 0;
	mac64_16_24( &x, i, DcEmeter.VrmsRaw);
	DcEmeter.DigiVrms = (x*221/2)>>24;	// mV, Resistor Dividor -> 442k/2k = 221, Gain = 2, ADC Resolution 24 		


	//
	// Irms
	//
	tmp = div_dc_current( &(DcEmeter.log_I_sq), 1024);

	DcEmeter.pureIrms = isqrt64(tmp);

	//
	// Scaling
	DcEmeter.IrmsRaw = (uint32_t) ((DcEmeter.pureIrms)>>32);	// Remove Fractional Part
	i = (1000*1.146);	// Ref = 1.146V , Display window to V x1000
	x = 0;
	mac64_16_24( &x, i, DcEmeter.IrmsRaw);
	DcEmeter.ShuntVol= (x*10/8)>>24;	// uV, Resistor Dividor -> 442k/2k = 221, Gain = 8, ADC Resolution 24 					
	DcEmeter.DigiIrms = (x*1000/5*2/8)>>24;	// Display Window to mA, Shunt 5mOhm, Gain = 8, ADC Resolution 24 
        DcEmeter.DigiIrms = (uint32_t)(DcEmeter.DigiIrms/1.06); //Current Make up/////hardware bug for 1.06

	DcEmeter.DigiavgPower = (DcEmeter.DigiIrms*DcEmeter.DigiVrms)/1000;	// Display to mW
}

// ADC10 interrupt service routine
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void)
{
  	uint16_t sample_Temp;
    //uint8_t *ptr;
    switch (__even_in_range(ADC10IV, 12))
    {
        case  ADC10IV_NONE: break;                // No interrupt
        case  ADC10IV_ADC10OVIFG: break;          // conversion result overflow
        case  ADC10IV_ADC10TOVIFG: break;         // conversion time overflow
        case  ADC10IV_ADC10HIIFG: break;          // ADC10HI
        case  ADC10IV_ADC10LOIFG: break;          // ADC10LO
        case  ADC10IV_ADC10INIFG: break;          // ADC10IN
        case  ADC10IV_ADC10IFG:                   // ADC10
            
		  	//
		  	// Perform IIR Filter
            sample_Temp = ADC10MEM0;              // Store ADC10 channel 0 result
			
			
            calTemp(sample_Temp);                 // Calculate Temperature
           // fill_PV_PowerInfo();                  // fill PowerInfo

            break;
			
        default: break;
		
    }
    
}

/******** ADC10_A ********/                 // T = (cons_R - ntc_R) / diff_R + cons_T

//#define Ktemp 0xF4                          // 2.5 * 1000 * 100 / 1023 = 244.33
#define Ktemp 0x142                         // 3.3 * 1000 * 100 / 1023 = 322.58
#define r 13000                             // 13k 
#define ref_V 330                           // 3.3 * 100
#define cons_T 25                           // R25 temperature
#define cons_R 47000                        // R25
//#define diff 705                          // 1 degree is 705 ohm
void calTemp(uint16_t sample_Temp)
{
  	int32_t adcresult_Temp;
        int32_t ntc_R;
	
    MPY = sample_Temp;                       // signed 16bits
    OP2 = Ktemp;
    
    res1 = RESHI;
    adcresult_Temp = (res1 << 16 | RESLO)/1000;
    ntc_R = (adcresult_Temp*r)/(ref_V-adcresult_Temp);
    //DcEmeter.Diode_Temp = (cons_R - ntc_R) / diff + cons_T;
    DcEmeter.Diode_Temp = Temp_table(ntc_R);
    //DcEmeter.Diode_Temp = adcresult_Temp;
    //DcEmeter.Diode_Temp = ntc_R;
    
}

#define initial_temp -40
int32_t Temp_table(int32_t R)
{
  int32_t temp_R[35] = {348500, // -40c
                        249962, // -35c
                        181385, // -30c
                        133090, // -25c
                        98691, // -20c
                        73923, // -15c
                        55905, // -10c
                        42668, // -5c
                        32851, // 0c
                        25504, // 5c
                        19959, // 10c
                        15739, // 15c
                        12502, // 20c
                        10000, // 25c
                        8052, // 30c
                        6525, // 35c
                        5321, // 40c
                        4364, // 45c
                        3599, // 50c
                        2984, // 55c
                        2488, // 60c
                        2084, // 65c
                        1754, // 70c
                        1483, // 75c
                        1260, // 80c
                        1074, // 85c
                        920, // 90c
                        791, // 95c
                        683, // 100c
                        591, // 105c
                        514, // 110c
                        448, // 115c
                        392, // 120c
                        344, // 125c
                        303 // 130c
                        };
  int32_t diff_R = 0;
  int32_t Temp = 0;
  for(int i=0;i<35;i++){
      if(temp_R[i]<R && i>0){
          diff_R = (temp_R[i-1] - temp_R[i]) / 5;
          Temp = (i*5 + initial_temp) - ((R - temp_R[i]) / diff_R);
          if(Temp > 130)
            Temp = 130;
          else if(Temp < -40)
            Temp = -40;
          return Temp;
          //return R;
      }
  }
  return Temp;
}




