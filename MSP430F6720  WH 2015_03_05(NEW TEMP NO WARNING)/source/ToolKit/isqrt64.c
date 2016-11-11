#include "configuration.h" // configuration of environment file
uint32_t isqrt64(register uint64_t h)
{  
    //bool s = __get_interrupt_state();
   
    uint32_t hi = 0xFFFFFFFF;
    uint32_t lo = 0;
   
    uint32_t mid = ( hi + lo ) >> 1;
    *(uint64_t*)&RES0 = 0x3FFFFFFF00000001ULL;
   
    MPY32CTL0 = 0;
   
    while( (lo < (hi-1)) && (*(uint64_t*)&RES0 != h))
    {  
        if( *(uint64_t*)&RES0 < h ) lo = mid;
        else hi = mid;
       
        mid = ( hi + lo ) >> 1;
       
        //*(uint64_t*)&RES0 = (int64_t)mid*mid;
        //__disable_interrupt();
            MPY32L = mid;
            MPY32H = mid >> 16;
            OP2L = mid;
            OP2H = mid >> 16;
        //__set_interrupt_state(s);
    }
   
    return(mid);
}