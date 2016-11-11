#include "configuration.h"

void calTemp(uint16_t sample_Temp)
{
    MPYS = sample_Temp;                       // signed 16bits
    OP2 = Ktemp;
    
    res1 = RESHI;
    adcresult_Temp = res1 << 16 | RESLO;
    
    result_Temp = (adcresult_Temp - b) / m;
    __no_operation();                         // For debug only
}