/*******************************************************************************
 *  emeter-toolkit.h -
 *
 *  Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/ 
 * 
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions 
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the   
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

/*! \file */

#if !defined(_EMETER_TOOLKIT_H_)
#define _EMETER_TOOLKIT_H_

#if defined(__TI_COMPILER_VERSION__)
/* IAR defines this, but CCS does not */
typedef unsigned short istate_t;
#endif

#if defined(__MSP430__)
#include <msp430.h>

#include <isr_compat.h>
#endif

#if defined(__IAR_SYSTEMS_ICC__)
#include <msp430Lib/iar/extra_peripheral_definitions.h>
#endif
#if (defined(__TI_COMPILER_VERSION__)  &&  __TI_COMPILER_VERSION__ >= 4000000)
#include <ccs/extra_peripheral_definitions.h>
#endif

#if defined(__GNUC__)  ||  defined(__IAR_SYSTEMS_ICC__)  ||  (defined(__TI_COMPILER_VERSION__)  &&  __TI_COMPILER_VERSION__ >= 4000000)
#define EMETER_TOOLKIT_SUPPORT_64BIT
#endif

#if defined(__GNUC__)  &&  !defined(__MSP430__)
#define __infomem__ /**/
#define __uninitialized__ /**/
#elif defined(__GNUC__)  &&  defined(__MSP430__)
#define __infomem__ __attribute__ ((section(".infomem")))
#define __infomem_uninitialized__ __attribute__ ((section(".infomemnobits")))
#define __uninitialized__ __attribute__ ((section(".noinit")))
#elif defined(__IAR_SYSTEMS_ICC__)  ||  defined(__TI_COMPILER_VERSION__)
#define __inline__ inline
#define __uninitialized__ /**/
#endif

#if defined(__MSP430__)
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE (!FALSE)
#endif
#endif

#if (defined(__IAR_SYSTEMS_ICC__)  ||  defined(__TI_COMPILER_VERSION__))  &&  defined(__MSP430_HAS_MPY32__)  &&  !defined(__TOOLKIT_USE_SOFT_MPY__)
static __inline__ int16_t q1_15_mul(int16_t x, int16_t y)
{
	int32_t res;
    istate_t istate;

    istate = __get_interrupt_state();
    __disable_interrupt();
    MPYS = y;
    OP2 = x;
    res = RES32[0] << 1;
    __set_interrupt_state(istate);
    return res >> 16;
}
#else
extern int16_t q1_15_mul(int16_t x, int16_t y);
#endif

#if (defined(__IAR_SYSTEMS_ICC__)  ||  defined(__TI_COMPILER_VERSION__))  &&  defined(__MSP430_HAS_MPY32__)  &&  !defined(__TOOLKIT_USE_SOFT_MPY__)
static __inline__ int16_t q1_15_mulr(int16_t x, int16_t y)
{
	int32_t res;
    istate_t istate;

    istate = __get_interrupt_state();
    __disable_interrupt();
    MPYS = y;
    OP2 = x;
    res = (RES32[0] + 0x4000L) << 1;
    __set_interrupt_state(istate);
    return res >> 16;
}
#else
extern int16_t q1_15_mulr(int16_t x, int16_t y);
#endif

#if (defined(__IAR_SYSTEMS_ICC__)  ||  defined(__TI_COMPILER_VERSION__))  &&  defined(__MSP430_HAS_MPY32__)  &&  !defined(__TOOLKIT_USE_SOFT_MPY__)
static __inline__ int32_t imul16(int16_t x, int16_t y)
{
	int32_t res;
    istate_t istate;

    istate = __get_interrupt_state();
    __disable_interrupt();
    MPYS = y;
    OP2 = x;
    res = RES32[0];
    __set_interrupt_state(istate);
    return res;
}
#else
extern int32_t imul16(int16_t x, int16_t y);
#endif

/*! \brief Find the square root of a 16 bit unsigned integer. The result is a 8.8 bit fractional integer.
    \param x The value for which the square root is required.
    \return The square root in the form 8 integer bits : 8 fractional bits.
*/
extern uint16_t isqrt16(uint16_t x);

/*! \brief Find the square root of a 32 bit unsigned integer. The result is a 16.16 bit fractional integer.
    \param x The value for which the square root is required.
    \return The square root in the form 16 integer bits : 16 fractional bits.
*/
extern uint32_t isqrt32(uint32_t x);

/*! \brief Find the square root of a 32 bit unsigned integer. The result is a 16 bit unsigned integer.
    \param x The value for which the square root is required.
    \return The square root, as a 16 bit unsigned integer.
*/
extern uint16_t isqrt32i(uint32_t x);

#if defined(EMETER_TOOLKIT_SUPPORT_64BIT)
/*! \brief Find the square root of a 64 bit unsigned integer. The result is a 32.32 bit fractional integer.
    \param x The value for which the square root is required.
    \return The square root in the form 32 integer bits : 32 fractional bits.
*/
extern uint64_t isqrt64(uint64_t x);

/*! \brief Find the square root of a 64 bit unsigned integer. The result is a 32 bit unsigned integer.
    \param x The value for which the square root is required.
    \return The square root, as a 32 bit unsigned integer.
*/
extern uint32_t isqrt64i(uint64_t x);
#endif

/*! \brief Remove the DC content from a 16 bit signal.
    \param p A pointer to a running DC estimation. This should be zeroed before the first call to dc_filter.
    \param x A signal sample to be filtered.
*/
extern int16_t dc_filter16(int32_t *p, int16_t x);

extern void dc_filter16_init(int32_t *p, int16_t x);

/*! \brief Remove the DC content from a 24 bit signal.
    \param p A pointer to a running DC estimation. This should be zeroed before the first call to dc_filter.
    \param x A signal sample to be filtered.
*/
extern int32_t dc_filter24(int16_t p[3], int32_t x);

extern void dc_filter24_init(int16_t p[3], int16_t x);

/*! \brief Convert a 16 bit unsigned integer to nibble packed BCD.
    \param bcd The buffer which will contain the BCD result.
    \param bin The 16 bit unsigned integer value to be converted.
*/
extern void bin2bcd16(uint8_t bcd[3], uint16_t bin);

/*! \brief Convert a 32 bit unsigned integer to nibble packed BCD.
    \param bcd The buffer which will contain the BCD result.
    \param bin The 32 bit unsigned integer value to be converted.
*/
extern void bin2bcd32(uint8_t bcd[5], uint32_t bin);

#if defined(EMETER_TOOLKIT_SUPPORT_64BIT)
/*! \brief Convert a 64 bit unsigned integer to nibble packed BCD.
    \param bcd The buffer which will contain the BCD result.
    \param bin The 64 bit unsigned integer value to be converted.
*/
extern void bin2bcd64(uint8_t bcd[10], uint64_t bin);
#endif

extern int16_t rand16(void);

extern int16_t dds_lookup(uint32_t phase_acc);
extern int16_t dds_interpolated_lookup(uint32_t phase_acc);
extern int16_t dds(uint32_t *phase_acc, int32_t phase_rate);

extern const int cos_table[];

#if defined(BCSCTL1_)  &&  defined(TACCR0_)
extern void set_dco(int freq);
#endif

/* If an accumulated value in one of our 48 bit ints is composed of
   many 32 bit values, then dividing by the number of values returns
   us to a 32 bit value. 48/16 => 32 division is, therefore, ideal for
   scalar dot product handling. */
int32_t div48(int16_t x[3], int16_t y);

/* A 16 bit into 48 bit divide, with upwards preshifting of the 48 bit number. */
int32_t div_sh48(int16_t x[3], int sh, int16_t y);

/* Multiply an int32_t by an int16_t, and return the top 32 bits of the
   48 bit result. */
#if (defined(__IAR_SYSTEMS_ICC__)  ||  defined(__TI_COMPILER_VERSION__))  &&  defined(__MSP430_HAS_MPY32__)  &&  !defined(__TOOLKIT_USE_SOFT_MPY__)
static __inline__ int32_t mul48_32_16(int32_t x, int16_t y)
{
    int32_t res;
    istate_t istate;

    istate = __get_interrupt_state();
    __disable_interrupt();
    MPYS32 = x;
    OP2 = y;
    _NOP();
    _NOP();
    res = *((int32_t *) &RES16[1]);
    __set_interrupt_state(istate);
    return res;
}

static __inline__ uint32_t mul48u_32_16(uint32_t x, uint16_t y)
{
    uint32_t res;
    istate_t istate;

    istate = __get_interrupt_state();
    __disable_interrupt();
    MPY32 = x;
    OP2 = y;
    _NOP();
    _NOP();
    res = *((uint32_t *) &RES16[1]);
    __set_interrupt_state(istate);
    return res;
}
#else
int32_t mul48_32_16(int32_t x, int16_t y);

uint32_t mul48u_32_16(uint32_t x, uint16_t y);
#endif

void shift48(int16_t x[3], int how_far);

#if defined(EMETER_TOOLKIT_SUPPORT_64BIT)
/* When int64_t is available, this can be a useful thing to have. */
static __inline__ int64_t int48_to_64(int16_t x[3])
{
    int64_t y;

    y = x[2];
    y <<= 16;
    y |= (uint16_t) x[1];
    y <<= 16;
    y |= (uint16_t) x[0];
    return  y;
}

static __inline__ void int64_to_48(int16_t y[3], int64_t x)
{
    y[2] = x >> 32;
    y[1] = x >> 16;
    y[0] = x;
}

#define transfer64(y,x) (y = x, x = 0)
#define assign64(y,x)   (y = x)
#endif

static __inline__ void transfer48(int16_t y[3], int16_t x[3])
{
    y[2] = x[2];
    y[1] = x[1];
    y[0] = x[0];
    x[0] =
    x[1] =
    x[2] = 0;
}

static __inline__ void assign48(int16_t y[3], const int16_t x[3])
{
    y[2] = x[2];
    y[1] = x[1];
    y[0] = x[0];
}

void mac48_16(int16_t z[3], int16_t x, int16_t y);

void sqac48_16(int16_t z[3], int16_t x);

#if (defined(__IAR_SYSTEMS_ICC__)  ||  defined(__TI_COMPILER_VERSION__))  &&  defined(__MSP430_HAS_MPY32__)  &&  !defined(__TOOLKIT_USE_SOFT_MPY__)
static __inline__ void sqac64_24(int64_t *z, int32_t x)
{
    /* This does not protect against interrupts. It is only for use within an interrupt routine */
    MPYS32 = x;
    OP2_32X = x;
    *z += RES64;
}
#else
void sqac64_24(int64_t *z, int32_t x);
#endif

#if (defined(__IAR_SYSTEMS_ICC__)  ||  defined(__TI_COMPILER_VERSION__))  &&  defined(__MSP430_HAS_MPY32__)  &&  !defined(__TOOLKIT_USE_SOFT_MPY__)
static __inline__ void mac64_16_24(int64_t *z, int16_t x, int32_t y)
{
    /* This does not protect against interrupts. It is only for use within an interrupt routine */
    MPYS32 = y;
    OP2 = x;
    *z += RES64;
}
#else
void mac64_16_24(int64_t *z, int16_t x, int32_t y);
#endif

#if defined(__IAR_SYSTEMS_ICC__)  ||  defined(__TI_COMPILER_VERSION__)
void accum48(int16_t z[3], int32_t y);
#else
static __inline__ void accum48(int16_t x[3], int32_t y)
{
    /* Accumulate a 32 bit integer value into a 48 bit one represented
       by a 3 element int16_t array */
#if defined(__MSP430__)  &&  defined(__GNUC__)
    int16_t y_ex;

    __asm__ __volatile__ (
        " mov   %B[y],%[y_ex] \n"
        " rla   %[y_ex] \n"
        " subc  %[y_ex],%[y_ex] \n"
        " inv   %[y_ex] \n"
        " add   %A[y],0(%[x]) \n"
        " addc  %B[y],2(%[x]) \n"
        " addc  %[y_ex],4(%[x]) \n"
        : 
        : [x] "r"(x), [y] "r"(y), [y_ex] "r"(y_ex));
#else
    int64_t acc;

    acc = (uint16_t) x[2];
    acc <<= 16;
    acc |= (uint16_t) x[1];
    acc <<= 16;
    acc |= (uint16_t) x[0];
    acc += y;
    x[0] = acc;
    acc >>= 16;
    x[1] = acc;
    acc >>= 16;
    x[2] = acc;
#endif
}
#endif

static void __inline__ brief_pause(unsigned int n)
{
    while (n > 0)
    {
        n--;
        __no_operation();
    }
}

static void __inline__ restart_watchdog(void)
{
#if defined(__MSP430__)
    WDTCTL = (WDTCTL & 0xFF) | WDTPW | WDTCNTCL;
#endif
}

#endif
