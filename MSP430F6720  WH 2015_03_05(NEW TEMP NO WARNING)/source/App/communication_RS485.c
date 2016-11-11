#include "configuration.h"
#include "Bus_Raw_Protocol.h"
#include "source/App/communication_RS485.h"

/******** eUART ********/
static uint8_t CMD_Decode_Buffer[MAX_CMD_LINE_SIZE] = {0};
static uint32_t CMD_Decode_Index = 0;
volatile uint8_t Decode_Flag = 0;  


//*****************************************************************************
//	UART TX/RX Ring Buffer and Index
//*****************************************************************************
//
// Rs485 RX Ring Buffer
#define UART_RX_BUFFER_SIZE 152
static uint8_t RX_Tank[UART_RX_BUFFER_SIZE];                   //Recevice software Buffer
static volatile uint16_t RX_Tank_Read_Index = 0;                         //RX Read point
static volatile uint16_t RX_Tank_Write_Index = 0;                        //RX write point


#define ADVANCE_RX_BUFFER_INDEX(Index) \
                                (Index) = ((Index) + 1) % UART_RX_BUFFER_SIZE

#define RX_BUFFER_EMPTY         (IsBufferEmpty(&RX_Tank_Read_Index,   \
                                               &RX_Tank_Write_Index))

//
//	Rs485 TX Ring Buffer
#define UART_TX_BUFFER_SIZE 152
static uint8_t TX_Tank[UART_TX_BUFFER_SIZE];                   //Recevice software Buffer
static volatile uint16_t TX_Tank_Read_Index = 0;               //Tx Read point
static volatile uint16_t TX_Tank_Write_Index = 0;              //Tx write point


#define ADVANCE_TX_BUFFER_INDEX(Index) \
                                (Index) = ((Index) + 1) % UART_TX_BUFFER_SIZE

#define TX_BUFFER_EMPTY         (IsBufferEmpty(&TX_Tank_Read_Index,   \
                                               &TX_Tank_Write_Index))

//*****************************************************************************
//	RS-485 Tranceiver DE/RE Control for Half Duplex Transmission
//*****************************************************************************
#define Enable_Rs485Tx_Tranceiver()	\
do{									\
  		P2OUT |= BIT0;				\
		P1OUT |= BIT7;				\
}while(0)							\

								  
#define Enable_Rs485Rx_Tranceiver()	\
do{									\
  		P1OUT &= ~BIT7;				\
		P2OUT &= ~BIT0;				\
}while(0)							\

//*****************************************************************************
//	Activate Tx Tranceiver Offtime Counting Timer
// - TASSEL_2 = SMCLK
// - MC_1 = Up mode
// - TACLR = clear TAR
//*****************************************************************************	
#define Activate_TX_OffTimer()		\
do{									\
  	TA1CTL = TASSEL_2|				\
			 MC_1|					\
			 TACLR;					\
}while(0)

#define Stop_TX_OffTimer()			\
do{									\
  	TA1CTL = TASSEL_2|				\
			 MC_0|					\
			 TACLR;					\
}while(0)

//*****************************************************************************
//	UART TX/RX Buffer Checker Functions
//*****************************************************************************								  
static uint8_t
IsBufferEmpty(volatile uint16_t *pulRead,
              volatile uint16_t *pulWrite)
{
    uint16_t ulWrite;
    uint16_t ulRead;

    ulWrite = *pulWrite;
    ulRead = *pulRead;

    return((ulWrite  == ulRead) ? true : false);
}

uint8_t RS485_0_Getc_NonBlock(uint8_t *Char_Data)
{
	//
	// Check if any char in the rx buffer
	if ( RX_BUFFER_EMPTY)
          return false;

    //
    // Read a character from the buffer.
    //
    *Char_Data = RX_Tank[RX_Tank_Read_Index];
	
	ADVANCE_RX_BUFFER_INDEX( RX_Tank_Read_Index);
    /*RX_Tank_Read_Index = (RX_Tank_Read_Index + 1);
    if ( RX_Tank_Read_Index>= UART_RX_BUFFER_SIZE)
      RX_Tank_Write_Index = 0;*/

    return true;
}

//*****************************************************************************
//	Timer1 A0 interrupt service routine
// 	- Automatic DE/RE Control, One-Shot timer.
//*****************************************************************************		
#pragma vector=TIMER1_A0_VECTOR
__interrupt void TIMER1_A0_ISR(void)
{
 	//
  	// Stop The Timer 
  	Stop_TX_OffTimer();	// Stop Timer
	
	//
	// Close Tx
	Enable_Rs485Rx_Tranceiver();

}

static uint16_t RX_Tank_Write_Index_Use;
// USCI_A2 interrupt service routine
#pragma vector=USCI_A2_VECTOR
__interrupt void USCI_A2_ISR(void)
{
RX_Tank_Write_Index_Use = RX_Tank_Write_Index;
    switch (__even_in_range(UCA2IV, 4))
    {
        case USCI_NONE: break;              // No interrupt
        case USCI_UART_UCRXIFG:             // RXIFG
            /* while (!(UCA2IFG & UCTXIFG));
              UCA2TXBUF = UCA2RXBUF;
            break;*/
            
			RX_Tank[RX_Tank_Write_Index_Use] = UCA2RXBUF;

			if(RX_Tank[RX_Tank_Write_Index_Use] == Tail_Byte)
			  Decode_Flag = 1;
			
			
			ADVANCE_RX_BUFFER_INDEX( RX_Tank_Write_Index);
			/*RX_Tank_Write_Index = (RX_Tank_Write_Index + 1);
			if ( RX_Tank_Write_Index>= UART_RX_BUFFER_SIZE)
			  RX_Tank_Write_Index = 0;*/

            break;
        case USCI_UART_UCTXIFG: 
			
		  	//
		  	// Load Char to Hw Tx Buffer.
		  	UCA2TXBUF = TX_Tank[TX_Tank_Read_Index];
			
			//
			// Move Index of Tx Ring Buffer.
			ADVANCE_TX_BUFFER_INDEX(TX_Tank_Read_Index);
			/*TX_Tank_Read_Index++;
			if (TX_Tank_Read_Index >= UART_TX_BUFFER_SIZE)
				TX_Tank_Read_Index = 0;	 */
			//
			// Check  Empty
			if (TX_BUFFER_EMPTY)
			{
				//
			  	// Activate Tranceiver controlling Timer
			  	Activate_TX_OffTimer();
			  	//
			  	// Close TX Interrupt
			  	UCA2IE &= ~UCTXIE;  
			}
		  
		break;      // TXIFG
		//
		// Following is I2C Only Events.
        case USCI_UART_UCSTTIFG: break;     // TTIFG
        case USCI_UART_UCTXCPTIFG: break;   // TXCPTIFG
        default: break;
    }
}



void tx_data(uint8_t *str,uint8_t len)       //Transmit data function
{
  	Enable_Rs485Tx_Tranceiver();
	for(int i=0; i < len; i++)
	{
		while (!(UCA2IFG & UCTXIFG)) ;           // USCI_A0 TX buffer ready?
	  	UCA2TXBUF = str[i];

	}
	Activate_TX_OffTimer();
}


//*****************************************************************************
//	Enhanced Rs-485 Transmission
//	-Buffered(Non-Block).
//	-Automatic RE/DE Control structure.
//*****************************************************************************	
void Enhance_Rs485_Tx (unsigned char *Buff, unsigned int Size)
{
  	uint16_t i;
	//
	// Stopping TX Tranceiver Timer
	Stop_TX_OffTimer();	
	
	//
  	// Enable TX Tranciever
	Enable_Rs485Tx_Tranceiver();
	
	//
	// Load Char to Tx Buffer
	for (i=0; i<Size; i++)
	{
		 TX_Tank[TX_Tank_Write_Index] = Buff[i];
		 
		 //
		 // Increase Index
		 ADVANCE_TX_BUFFER_INDEX(TX_Tank_Write_Index);
		 /*TX_Tank_Write_Index++;
		 if (TX_Tank_Write_Index >= UART_TX_BUFFER_SIZE)
		   TX_Tank_Write_Index = 0;	 */
	}
	
	//
	// Enable Tx Empty Interrupt
	UCA2IE |= UCTXIE;  
}

//*****************************************************************************
//	Burst Rs-485 Transmission
//	-Buffered(Non-Block).
//	-Automatic RE/DE Control structure.
//*****************************************************************************	
void Burst_Rs485_Tx (unsigned char *Buff, unsigned int Size)
{	//!!!! Warnning This function is incompelet.
  	uint16_t i;
	uint16_t Base;
	//
	// Stopping TX Tranceiver Timer
	Stop_TX_OffTimer();	
	
        
        
	//
  	// Enable TX Tranciever
	Enable_Rs485Tx_Tranceiver();
        
        //
        //delay 1000us
        __delay_cycles(25000);
        //__delay_cycles(30000);
	
	//
	//	When Buffer is Empty and Tx is not busy,
	// Excute Burst Transimission to reduce a tx cycle.
	if (TX_BUFFER_EMPTY && (UCA2IFG & UCTXIFG))
	{
		//
	  	// Send out First Char 
	  	UCA2TXBUF = Buff[0];
		Base = 1;
	}
	
	//
	// Load Char to Tx Buffer
	for (i=Base; i<Size; i++)
	{
		 TX_Tank[TX_Tank_Write_Index] = Buff[i];
		 
		 //
		 // Increase Index
		 ADVANCE_TX_BUFFER_INDEX(TX_Tank_Write_Index);
		 /*TX_Tank_Write_Index++;
		 if (TX_Tank_Write_Index >= UART_TX_BUFFER_SIZE)
		   TX_Tank_Write_Index = 0;	 */
	}
	
	//
	// Enable Tx Empty Interrupt
	UCA2IE |= UCTXIE;  
}


/////
//	!!Move to Protocol this is RS-485 Port layer
/////


void fill_PV_PowerInfo()                     //fill PV_PowerInfo value
{
  //pvPowerInfo.DiodeTemp = result_Temp + 50;
  pvPowerInfo.Voltage = V_rms;
  pvPowerInfo.Current = I_rms;
  pvPowerInfo.Power = P_rms;
  pvPowerInfo.AlertState = 0x00000000;
}


void Decode_Proccess ()
{
     unsigned char Engine_State;
	//
	// Pop out Command line
	while (RS485_0_Getc_NonBlock(&(CMD_Decode_Buffer[CMD_Decode_Index])))
	{
		if (CMD_Decode_Index > MAX_CMD_LINE_SIZE)
		{
			//
			// Reset Index
			CMD_Decode_Index = 0;
			
		}
		//printf("%x ", CMD_Decode_Buffer[CMD_Decode_Index]);
		CMD_Decode_Index++;
	}
	
	if ( CMD_Decode_Index == 0)
	   return;
	   
	//
	// Decode Command Line
	Engine_State = BusRaw_DeviceDecode_Engine ( CMD_Decode_Buffer, CMD_Decode_Index);

	if (Engine_State != PackNotReady)
	{
		//
		// Reset Index
		CMD_Decode_Index = 0;
	}

}