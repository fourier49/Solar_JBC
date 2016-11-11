
#ifndef _Uart_Com_H_
#define _Uart_Com_H_


// Uart Function
extern int rs232_init(int com,int baudrate);

extern int recv_data(char *data);

extern int send_data(char data);




// Send Command to Board 
extern void write_cmd(char cmd,int data);
extern void read_cmd(char cmd,unsigned int *data);
extern void* Get_com1_handler (void);

#endif



