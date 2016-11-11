#include "configuration.h"
#include "ToolKit.h"
void calAccSample_V(uint32_t sample_V)
{
    MPY32L = sample_V;                                      // 32bit  low word
    MPY32H = sample_V >> 16;                                // 32bit  high word
    OP2L = sample_V;                                        // 32bit  low word
    OP2H = sample_V >> 16;                                  // 32bit  high word
                                                            // sample_V most 24bits, 24bits * 24bits = 48bits
    res1 = RES1;                                            // RES1:16~31 bit
    res2 = RES2;                                            // RES2:32~47 bit
    accSample_V += (res2 << 32) | (res1 << 16) | RES0;      // RES0: 0~15 bit
	   
}

void calVRMS()
{
    //accSample_V = isqrt64(accSample_V);

    MPY32L = accSample_V;
    MPY32H = accSample_V >> 16;
    OP2 = Kv;
    
    res1 = RES1;
    res2 = RES2;
    accSample_V = (res2 << 32) | (res1 << 16) | RES0;
    V_rms = (accSample_V / Divisor) >> 1;                   //Gain = 2;
    __no_operation();
}