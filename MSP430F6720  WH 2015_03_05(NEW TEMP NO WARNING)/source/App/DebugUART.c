#include "configuration.h"
#include <stdio.h>
#include "source/App/DebugUART.h"
//*****************************************************************************
//	UART TX/RX Ring Buffer and Index
//*****************************************************************************
//
// Rs485 RX Ring Buffer
#define DB_UART_RX_BUFFER_SIZE 10
//static uint8_t RX_Tank[DB_UART_RX_BUFFER_SIZE];                   //Recevice software Buffer
static volatile uint16_t RX_Tank_Read_Index = 0;                         //RX Read point
static volatile uint16_t RX_Tank_Write_Index = 0;                        //RX write point


#define ADVANCE_RX_BUFFER_INDEX(Index) \
                                (Index) = ((Index) + 1) % DB_UART_RX_BUFFER_SIZE

#define RX_BUFFER_EMPTY         (IsBufferEmpty(&RX_Tank_Read_Index,   \
                                               &RX_Tank_Write_Index))

//
//	Rs485 TX Ring Buffer
#define DB_UART_TX_BUFFER_SIZE 64
static uint8_t TX_Tank[DB_UART_TX_BUFFER_SIZE];                   //Recevice software Buffer
static volatile uint16_t TX_Tank_Read_Index = 0;               //Tx Read point
static volatile uint16_t TX_Tank_Write_Index = 0;              //Tx write point


#define ADVANCE_TX_BUFFER_INDEX(Index) \
                                (Index) = ((Index) + 1) % DB_UART_TX_BUFFER_SIZE

#define TX_BUFFER_EMPTY         (IsBufferEmpty(&TX_Tank_Read_Index,   \
                                               &TX_Tank_Write_Index))


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


// USCI_A2 interrupt service routine
#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void)
{
    switch (__even_in_range(UCA1IV, 4))
    {
        case USCI_NONE: break;              // No interrupt
        case USCI_UART_UCRXIFG:             // RXIFG
         
			//RX_Tank[RX_Tank_Write_Index] = UCA1RXBUF;
			
			ADVANCE_RX_BUFFER_INDEX( RX_Tank_Write_Index);
			/*RX_Tank_Write_Index = (RX_Tank_Write_Index + 1);
			if ( RX_Tank_Write_Index>= DB_UART_RX_BUFFER_SIZE)
			  RX_Tank_Write_Index = 0;*/

            break;
        case USCI_UART_UCTXIFG: 
			
		  	//
		  	// Load Char to Hw Tx Buffer.
		  	UCA1TXBUF = TX_Tank[TX_Tank_Read_Index];
			
			//
			// Move Index of Tx Ring Buffer.
			ADVANCE_TX_BUFFER_INDEX( TX_Tank_Read_Index);
			/*TX_Tank_Read_Index++;
			if (TX_Tank_Read_Index >= DB_UART_TX_BUFFER_SIZE)
				TX_Tank_Read_Index = 0;	 */
			//
			// Check  Empty
			if (TX_BUFFER_EMPTY)
			{
			  	//
			  	// Close TX Interrupt
			  	UCA1IE &= ~UCTXIE;  
			}
		  
		break;      // TXIFG
		//
		// Following is I2C Only Events.
        case USCI_UART_UCSTTIFG: break;     // TTIFG
        case USCI_UART_UCTXCPTIFG: break;   // TXCPTIFG
        default: break;
    }
}



void DB_tx_data(uint8_t *str,uint8_t len)       //Transmit data function
{
	for(int i=0; i < len; i++)
	{
		while (!(UCA1IFG & UCTXIFG)) ;           // USCI_A0 TX buffer ready?
	  	UCA2TXBUF = str[i];

	}
}


//*****************************************************************************
//	Burst Rs-485 Transmission
//	-Buffered(Non-Block).
//	-Automatic RE/DE Control structure.
//*****************************************************************************	
void DB_Burst_Rs485_Tx (unsigned char *Buff, unsigned int Size)
{
  	uint16_t i;
	uint16_t Base;
	
	//
	//	When Buffer is Empty and Tx is not busy,
	// Excute Burst Transimission to reduce a tx cycle.
	if (TX_BUFFER_EMPTY && (UCA1IFG & UCTXIFG))
	{
		//
	  	// Send out First Char 
	  	UCA1TXBUF = Buff[0];
		Base = 1;
	}
	
	//
	// Load Char to Tx Buffer
	for (i=Base; i<Size; i++)
	{
		 TX_Tank[TX_Tank_Write_Index] = Buff[i];
		 //
		 // Increase Index
		 ADVANCE_TX_BUFFER_INDEX( TX_Tank_Write_Index);
		 /*TX_Tank_Write_Index++;
		 if (TX_Tank_Write_Index >= DB_UART_TX_BUFFER_SIZE)
		   TX_Tank_Write_Index = 0;	 */
	}
	
	//
	// Enable Tx Empty Interrupt
	//UCA1IE |= UCTXIE;  
}

