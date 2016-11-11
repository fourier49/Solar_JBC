

#ifndef __DEBUGUART_H__
#define __DEBUGUART_H__


//*****************************************************************************
//
// API Functions Prototype
//
//*****************************************************************************
void DB_Burst_Rs485_Tx (unsigned char *Buff, unsigned int Size);
void DB_tx_data(uint8_t *str,uint8_t len) ;
#endif //  __DEBUGUART_H__
