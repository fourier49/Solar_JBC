#include "configuration.h"
#include "ToolKit.h"
void calAccSample_I(uint32_t sample_I)
{
    MPY32L = sample_I;                                  // 32bits  low word 
    MPY32H = sample_I >> 16;                            // 32bits  high word
    OP2L = sample_I;                                    // 32bits  low word 
    OP2H = sample_I >> 16;                              // 32bits  high word 
                                                        // sample_I most 24bits, 24bits * 24bits = 48bits
    res1 = RES1;                                        // RES1:16~31 bit
    res2 = RES2;                                        // RES2:32~47 bit
    accSample_I += (res2 << 32) | (res1 << 16) | RES0;  // RES0: 0~15 bit
	   	
}

void calIRMS()
{
    //accSample_I = isqrt64(accSample_I);
    
    MPY32L = accSample_I;
    MPY32H = accSample_I >> 16;
    OP2 = Ki;                                           //16bits
    
    res1 = RES1;
    res2 = RES2;
    accSample_I = (res2 << 32) | (res1 << 16) | RES0;
    I_rms = accSample_I / Divisor;
    //I_rms = I_rms >> 2;
}