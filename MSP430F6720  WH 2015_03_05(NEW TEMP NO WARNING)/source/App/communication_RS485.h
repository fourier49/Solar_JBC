

#ifndef __COMMUNICATION_RS485_H__
#define __COMMUNICATION_RS485_H__


//*****************************************************************************
//
// API Functions Prototype
//
//*****************************************************************************
void Burst_Rs485_Tx (unsigned char *Buff, unsigned int Size);
void Enhance_Rs485_Tx (unsigned char *Buff, unsigned int Size);
void tx_data(uint8_t *str,uint8_t len);
uint8_t RS485_0_Getc_NonBlock(uint8_t *Char_Data);
#endif //  __COMMUNICATION_RS485_H__
