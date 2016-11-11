#include "configuration.h"

#include <msp430f6720.h>

#include "msp430Lib/hal_UCS.h"
#include "msp430Lib/hal_pmm.h"

#include "source/App/ProtocolTimer.h"


//*****************************************************************************
//	System Configuration
//	
//*****************************************************************************
#define MCLK_DEF 16	// 16Mhz

//*****************************************************************************
//	System Debug
//	
//*****************************************************************************
#define Debug_UCA 0


//*****************************************************************************
//	Analog front-end Configuration
//
//*****************************************************************************
#define CURRENT_Shunt_GAIN	SD24GAIN_8
#define VOLTAGE_GAIN		SD24GAIN_2


//*****************************************************************************
//	Ememter Parameter
//
//*****************************************************************************
#define SAMPLE_RATE                         4096
#define SAMPLES_PER_10_SECONDS              40960ul

//*****************************************************************************
//	Analog front-end initialization routine.
//	- SD24 B.
//	Description:
//		Configures the 24-bit sigma-delta ADC module as analog front-end for
// 	a tamper-resistant meter using a shunt Micro-Resistor as current sensor.
//*****************************************************************************
//#define SD24_with_DMA
static void Init_SD24_Analog_Front_End(void)
{

    /*
     * The general configuration of the analog front-end,
     * that applies to all channels: clock selection (SMCLK) and divider
     * settings (depending on SMCLK frequency) and reference voltage
     * selections.
     */
    //REFCTL0 = REFMSTR + REFVSEL_1 + REFON;	// Enabale Reference = 2.0V

	//
	// !!Purpose is not sure!!
    SD24BCTL1 &= ~SD24GRP0SC; 

	//
	// SD24 Basic Setting.
#if MCLK_DEF == 16
    SD24BCTL0 = SD24SSEL__SMCLK  			// Clock is SMCLK
            | SD24PDIV_4    				// Divide by 16 => ADC clock: 1.048576MHz 
            | SD24DIV0
            | SD24REFS;     				// Use internal reference
#endif
#if MCLK_DEF == 8
    SD24BCTL0 = SD24SSEL__SMCLK  			// Clock is SMCLK
            | SD24PDIV_3   					// Divide by 8 => ADC clock: 1.048576MHz
            | SD24REFS;    					// Use internal reference 
#endif
	//
	// Configure Voltage Channel (CH0) Analog Front-End
    SD24BINCTL0 = VOLTAGE_GAIN; 		// Set gain for channel 0 (V) 
	
    SD24BCCTL0 = SD24DF_1| 					// Data Format: 2's complement
	  			//SD24ALGN|					// Data Alignment: Left-aligned MSB of Filter output.
				SD24SCS_4;                	// Set oversampling ratio to 256 (default)
    SD24BOSR0 = 256 - 1;      				// Set oversampling ratio to 256 (default) 
    //SD24BPRE0 = 0;							// Clear Digital filter preload value.

	//
	// Configure Current Channel (CH1) Analog Front-End
    SD24BINCTL1 = CURRENT_Shunt_GAIN;             // Set gain for channel 1 (I) 
	
    SD24BCCTL1 = SD24DF_1|					// Data Format: 2's complement
	 			//SD24ALGN|					// Data Alignment: Left-aligned MSB of Filter output.
				SD24SCS_4;             		// Set oversampling ratio to 256 (default) 
    SD24BOSR1 = 256 -1;                     // Set oversampling ratio to 256 (default) 
    
	//
	// Group Setting
    SD24BCTL1 |= SD24GRP0SC;                // Group 0 Start Conversion.

	SD24BIE = SD24IE1;                      // Enable channel 1 interrupt
	__delay_cycles(0x3600);                 // Delay for init	
	//
	// SD24B Data Bypass Through DMA
#ifdef SD24_with_DMA
	extern int16_t dma_adc_buffer[6];

	SD24BCTL1_H = SD24DMA1_H;

	DMACTL0 = DMA0TSEL__SD24IFG;
	DMA0SA = (unsigned short) &SD24BMEML0;
	DMA0DA = (unsigned short) dma_adc_buffer;
	DMA0SZ = 6;
	DMA0CTL = DMASWDW + DMASRCINCR_3 + DMADSTINCR_3 + DMADT_5 + DMAEN + DMAIE; // word, inc dest, repeated block, Enable, Enable IE
#else
    //SET_SD16IE_VOLTAGE;
#endif
    /*SD16PRE_VOLTAGE = DEFAULT_V_PRESCALE_FACTOR;
    #if defined(SD16CONF1_FUDGE)
    SD16CONF1 = SD16CONF1_FUDGE;
    #endif

    #if !defined(ESP_SUPPORT)  &&  defined(SINGLE_PHASE)
    chan1.metrology.current.in_phase_correction[0].sd16_preloaded_offset = 0;
        #if defined(NEUTRAL_MONITOR_SUPPORT)
    chan1.metrology.neutral.in_phase_correction[0].sd16_preloaded_offset = 0;
        #endif
    #endif*/
    //
    // \note
    // Please note, the oversampling ratio should be the same
    // for all channels. Default is 256.
    //
}

void Init_SD24_ADC (void)
{

	SD24BCTL0 = SD24REFS| 								// Select Internal Reference
	  			SD24SSEL_1|								// Select SMCK as Clcok Source
				SD24PDIV2;								// Divide 16 as fm 
											
	SD24BCCTL0 = SD24SNGL | SD24SCS_4;                 // Left-aligned, group 0
	SD24BCCTL1 = SD24SNGL | SD24SCS_4;                 // Left-aligned, group 0
	
	SD24BINCTL1 = SD24GAIN_2;               // ch0 Gain = ;
	
	
	SD24BIE = SD24IE1;                      // Enable channel 1 interrupt
	__delay_cycles(0x3600);                 // Delay for init
	//SD24BCTL1 |= SD24GRP0SC;                // Set bit to start conversion
	
}
//*****************************************************************************
//	Initialize Debug UART
//	
//*****************************************************************************
void Init_Debug_UART ()
{
	
	/********eUART1 Init********/
	P1SEL |= BIT4 | BIT5;                   // Set P1.4, P1.5 to non-IO
	P1DIR |= BIT4 | BIT5;                   // Enable UCA0RXD, UCA0TXD
	
	UCA1CTLW0 |= UCSWRST;                   // **Put state machine in reset**
	UCA1CTLW0 |= UCSSEL_2;                  // SMCLK
	UCA1BRW_L = 8;                          // 1MHz 115200 (see User's Guide) 1M / 115200 = 8.68
	UCA1BRW_H = 0;                          // 1MHz 115200
	UCA1MCTLW = (11<<4) | UCBRF_11 | UCOS16;          // Modln UCBRSx=0, UCBRFx=0x13,
	//UCA1MCTLW = 0xB700;                     // Modulation UCBRSx=0x53, UCBRFx=0
	UCA1CTLW0 &= ~UCSWRST;                  // **Initialize USCI state machine**
	UCA1IE |= UCRXIE;                       // Enable USCI_A0 RX interrupt
	

}


//*****************************************************************************
//	Initialize RS485 DE/RE Control Timer
//	-TX Off Time is measured from Logical Analyzer.
//*****************************************************************************
#define Rs485_Tx_OffTime 	90			// uSec 90uSec @115200 (10 / 115200 = 86.8us = 90uSec)
#define txEmptyFactor 		2			// Before Transmission completed, there are 2 byte have to be send out.
#define TX_OFFTIMER_LOAD 	(Rs485_Tx_OffTime*MCLK_DEF*txEmptyFactor)	// The value load to timer.
void Init_Timer_For_RS485 (void)
{
    // Setup TA1
    TA1CCTL0 = CCIE;                        // CCR0 interrupt enabled
    TA1CCR0 = TX_OFFTIMER_LOAD;
}

//*****************************************************************************
//	Initialize RS485 UART
//	
//*****************************************************************************
void Init_RS485_UART (void)
{
  	//
  	// Setup Pins 
	P2SEL |= BIT2 | BIT3;                   // Set P2.2, P2.3 for RS485  eUSCI_A2 = RXD/TXD
	P2DIR |= BIT2 | BIT3;                   // Enable UCA2RXD, UCA2TXD
	P1DIR |= BIT7;                          // Enable RS485 DE
	P2DIR |= BIT0;                          // Enable RS485 /RE
	
	//
	// Set to /RE
	P1OUT &= ~BIT7;				
	P2OUT &= ~BIT0;
	//
	// 
	UCA2CTLW0 |= UCSWRST;                   // **Put state machine in reset**
	UCA2CTLW0 |= UCSSEL_2;                  // SMCLK
	UCA2BRW_L = 8;                          // 1MHz 115200 (see User's Guide) 1M / 115200 = 8.68
	UCA2BRW_H = 0;                          // 1MHz 115200
	UCA2MCTLW = (11<<4) | UCBRF_11 | UCOS16;          // Modln UCBRSx=0, UCBRFx=0x13,
	//UCA2MCTLW = 0xB700;                     // Modln UCBRSx=0xB7, UCBRFx=0, 0.68 see User's Guide P992
											// over sampling
	UCA2CTLW0 &= ~UCSWRST;                  // **Initialize USCI state machine**
	UCA2IE = UCRXIE;                       // Enable USCI_A0 RX interrupt
	
	//
	// Enable Timer for DE/RE Control
	Init_Timer_For_RS485 ();

}

//*****************************************************************************
//	Initial System Clock
//	
//*****************************************************************************
void Init_System_Clock (void)
{
	// 
    // Stop WatchDog 
    WDTCTL = (WDTCTL & 0xFF) | WDTPW | WDTHOLD; // WDTPW means WDT Password.
  
	
	//
	// Clock Destination Assignment
    UCSCTL4 |= SELA__REFOCLK;                    	// Set ACLK = 32.768kHz
    UCSCTL4 |= SELS__DCOCLK;               			// Set SMCLK = DCOCLCK
	
    UCSCTL3 |= SELREF_2;                   			// Set DCO FLL reference = 
	
    UCSCTL6 |= XCAP_3;                     			// Internal load cap
	//LFXT_Start(XT1DRIVE_1);		

	
#if MCLK_DEF > 8
    SetVCore(3);      // Set VCore to High level 
#endif			
	
    //Init_FLL_Settle(MCLK_DEF*8388608/8/1000, MCLK_DEF*32768*32/32768);	
	Init_FLL_Settle((unsigned int)(MCLK_DEF*1000*0.95), 
					(unsigned int) (MCLK_DEF*32768*32*0.95/32768));		// 0.95 is a clock compensation Factor.

    // Worst-case settling time for the DCO when the DCO range bits have been
    // changed is n x 32 x 32 x f_MCLK / f_FLL_reference. See UCS chapter in 5xx
    // UG for optimization.
    // 32 x 32 x 16 MHz / 32,768 Hz = 250000 = MCLK cycles for DCO to settle
    __delay_cycles(500000);

    // Loop until XT1 & DCO fault flag is cleared
    /*do
    {
        UCSCTL7 &= ~( DCOFFG);
        // Clear XT2,XT1,DCO fault flags
        SFRIFG1 &= ~OFIFG;                 // Clear fault flags
    } while (SFRIFG1 & OFIFG);             // Test oscillator fault flag
	*/
}






//
void SystemInit(void)
{
  
	//
  	// Setup Clock Source and Disturbution
	Init_System_Clock ();
#if Debug_UCA
	PJDIR |= BIT0 | BIT1 | BIT3;           // ACLK, MCLK, SMCLK set out to pins
	PJSEL |= BIT0 | BIT1 | BIT3;           // PJ.0,1,3 for debugging purposes.
#endif

	
	//
	// INIT RS-485 UART
	Init_RS485_UART ();
	
	//
	// INIT Debug UART 
	Init_Debug_UART ();
	
	//
	// INIT SD24 ADC
	Init_SD24_Analog_Front_End();
	
	//Init_SD24_ADC ();

	/********REF Init********/
	while (REFCTL0 & REFGENBUSY) ;          // If ref generator busy, WAIT
	REFCTL0 |= REFON | REFVSEL_2; 			// REF enable. REF be controlled via the REFCTL register
											// Reference voltage : 2.5V
	__delay_cycles(75);                     // Delay (~75us) for Ref to settle

	


	/********ADC10_A Init********/
	P1SEL |= BIT2;                          // Set P1.2 to non-IO
	ADC10CTL0 = ADC10SHT_2 | ADC10ON;       // S&H=16 ADC clks, Enable ADC10
	ADC10CTL1 |= ADC10SHP;                  // ADCCLK = MODOSC; sampling timer
	ADC10CTL2 |= ADC10RES;                  // 10-bit conversion results
	ADC10IE |= ADC10IE0;                    // Enable ADC conv cmplete interrupt
	//ADC10MCTL0 |= ADC10INCH_0 | ADC10SREF_1;// A0 ADC input select; Vref=2.5V
        ADC10MCTL0 |= ADC10INCH_0 | ADC10SREF_0;// A0 ADC input select; Vref=AVCC 3.3V
	
	
	//
	// Protocol Timer
	Init_Timer_For_BRDProtocol ();
	
	 Init_RTC ();
	
	//ADC10CTL0 |= ADC10ENC | ADC10SC;
	//__bis_SR_register(GIE);
  
    
}
