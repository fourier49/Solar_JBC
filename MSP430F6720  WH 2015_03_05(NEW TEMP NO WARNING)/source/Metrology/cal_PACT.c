#include "configuration.h"
#include "ToolKit.h"

void calPACT()
{
    MPY = V_rms;
    OP2 = I_rms;
    res1 = RESHI;
    P_rms = res1 << 16 | RESLO;
    P_rms = P_rms / 100;   
}