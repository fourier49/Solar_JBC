#include <string.h>
#include <stdio.h>
/* Application include files. */	
//#include "Bus_Protocol/RS485_API.h"
#include "Bus_Raw_Protocol.h"
#include "configuration.h"

#include "source/App/DCEmeter.h"
#include "source/App/DebugUART.h"
#include "source/App/communication_RS485.h"
#include "source/App/ProtocolTimer.h"
//#include "Uart_Com.h"

#define WRONG_WDTPW   (0x5B00u)

//*****************************************************************************
//
// Read MAC file
//
//*****************************************************************************
#define EnvStartAdd 0x1800
#define ContiRetCount	10

typedef struct ENVCONFIG
{
	uint8_t Mac[6];
	uint8_t SeriesNumber[12];


}ENVCONFIG;
////////////////////////GOBAL VALUE//////////////
ENVCONFIG G_ENVCONFIG;
/////////////////////////////////////////////////
int Get_EvnString(void);
int SaveMac2Evn(char*Start);
int SaveSNEvn(char*Start);
uint8_t* hex_decode(char *in, size_t len, uint8_t *out);

//*****************************************************************************
//	Macro
//*****************************************************************************
#define Flush_Packet_Buffer(Buff, Index)	\
	do										\
	{										\
		while (Index--)						\
		{									\
			Buff[Index] = 0x0;				\
		}									\
	}while(0)

	  
#define BRP_Pack_Out(Buf,Lens)	Burst_Rs485_Tx(Buf,Lens) 
	  
	  
#define DbStrPrint(Str)	 DB_Burst_Rs485_Tx( Str , sizeof(Str))
	  
/*	  
void DbStrPrint (unsigned char *Str)
{
	DB_Burst_Rs485_Tx( Str , sizeof(Str));
	DB_Burst_Rs485_Tx("\r\n", 2); 
}*/

//*****************************************************************************
//	Gernerate Check Sum for Packet
//  
//*****************************************************************************		
int16_t Generate_CheckSum (unsigned char *Pack, unsigned char Start_Index,unsigned char Lens)
{
	unsigned char i;
	unsigned long Check_Sum = 0;
	
	for (i=Start_Index; i< (Lens+Start_Index); i++)
		Check_Sum += Pack[i];
	
	//
	// Ignaor Saturation 
	Check_Sum = (Check_Sum&0xff);
	
	return Check_Sum;
}

//#define BRD_Debug
//#define Display_BRD_INFO
//#define BRD_Debug_TX_DUMP
//*****************************************************************************
//	Bus Raw Data Decoding Engine
//	
//*****************************************************************************
//static unsigned char JB_Local_BusID = 0xfe;
//static unsigned char JB_Local_MAC[6] = {};

extern uint32_t  urand(void);

static bBusDevState JB_LocalDev;


stPackProcess BusRaw_DeviceDecode_Engine (unsigned char *Packet_Buffer, unsigned char Pack_End_Indx)
{
  	//unsigned char DebStr[6];
  
	unsigned int i;
	
	uint16_t RandomDelay;
	unsigned char *pMAC;
	unsigned char *Payload;
	int16_t Temp = 0;
	
	unsigned int OutIndex = 0;
	
	unsigned char Decode_State = PackInvalid;
	unsigned char Header_Index = 0;
	
	Bus_Raw_Packet BusRxt_Pack;
    
#ifdef Display_BRD_INFO
	printf("Dump Packet : ");
	for (i=0;i<Pack_End_Indx; i++)
		printf("%x ", Packet_Buffer[i]);
	printf("\n");
#endif	
	
#ifdef BRD_Debug
	printf("Pack End Index : %d\n", Pack_End_Indx);
#endif		//
	// Check Command line is valid.
	if ( Pack_End_Indx == 0)
		return PackInvalid;
	
	while (1)	
	{
		//
		// Search Header Byte 
		for (i=Header_Index; i<Pack_End_Indx; i++)
		{
			if (Packet_Buffer[i] == BRD_Header)
			{
				Header_Index = i;
                
#ifdef BRD_Debug
            	printf("Header %x\n", Packet_Buffer[i]);
#endif
				break;
			}
		}
		
		//
		// Check if Start Byte is Not Finded
		if (Pack_End_Indx == i)
		{
			//
			// Check If CMD is Not Ready
			if (Decode_State == CL_Sate_NotReady)
				return PackNotReady;

			//
			// Decode_State == CL_Sate_WrStart or CL_State_Invalid
			// Trush in the buffer , clear buffer
			Flush_Packet_Buffer(Packet_Buffer, Pack_End_Indx);
			return PackInvalid;
		}
		
		//
		// Get Packet Length
		BusRxt_Pack.PackLens = (unsigned short) ((Packet_Buffer[Header_Index+1]<<8)|Packet_Buffer[Header_Index+2]);
#ifdef BRD_Debug
		printf("Pack Lens : %d\n", BusRxt_Pack.PackLens);	
#endif
		//
		// Check Tail Code.
		if ((BRD_Tail_Index(BusRxt_Pack.PackLens)+Header_Index) == (Pack_End_Indx-1))
		{
			//
			// Detect a Valid Packet.
			break;
		}
		else if ((BRD_Tail_Index(BusRxt_Pack.PackLens)+Header_Index) > (Pack_End_Indx-1))
		{
			//
			// Assume CMD is Not Ready but keep going for Searching
			Decode_State = CL_Sate_NotReady;
		}
		else 
		{
			//
			// Start Index is Wrong
			Decode_State = CL_Sate_WrStart;
		}

		//
		// Going to Search Another Header Code from CMD Line Buffer
		Header_Index++;
	}
#ifdef BRD_Debug
	printf("Start Extract\n");
#endif

	
	//
	// Extract Packet
	BusRxt_Pack.CTL_Field = Packet_Buffer[Header_Index+BRD_CTL_Index];	
	BusRxt_Pack.BusID = Packet_Buffer[Header_Index+BRD_BusID_Index];	
	BusRxt_Pack.CMD_Type = Packet_Buffer[Header_Index+BRD_Type_Index];	
	BusRxt_Pack.CheckSum = Packet_Buffer[Header_Index+BRD_ChSUM_Index(BusRxt_Pack.PackLens)];
#ifdef BRD_Debug
	printf("Pack Control Field : %x\n", BusRxt_Pack.CTL_Field);	
	printf("Pack Bus ID : %x\n", BusRxt_Pack.BusID);	
	printf("Pack CMD Type : %x\n", BusRxt_Pack.CMD_Type);	
	printf("Pack CheckSum : %x\n", BusRxt_Pack.CheckSum);	
#endif
	
	//
	// Extract Control Field - Determine Target Device
	if ( !(BusRxt_Pack.CTL_Field & clFromMaster))
	{
		//
		// Source is not Master ,Ignore Packet
#ifdef BRD_Debug
		printf("Source is not Master ,Ignore Packet\n");
#endif 		
		//
		// Flush Command Buffer 
		Flush_Packet_Buffer(Packet_Buffer, Pack_End_Indx);
		return PackInvalid;
	}
	
	//
	// Check Bus ID is Matched.
	if ( BusRxt_Pack.BusID != JB_LocalDev.BusID)	
	{

		if ( !(BusRxt_Pack.CTL_Field & clBroadcast))
		{
			//DbStrPrint("Fail ID\n\r");
			//
			// Check Packet is Scan Event
			if ( !(BusRxt_Pack.CTL_Field & clScan))
			{	//
				// Undefined Packet, Ignore Packet

#ifdef BRD_Debug
				printf("Undefined BroadPacket, Ignore Packet\n");
#endif 				
				//
				// Flush Command Buffer 
				Flush_Packet_Buffer(Packet_Buffer, Pack_End_Indx);
				return PackInvalid;	
			}
			
			if (BusRxt_Pack.CMD_Type != ctCH_SCAN)
			{	//

				// Command Type is Not Correct, Ignore Packet
#ifdef BRD_Debug
				printf("Command Type is Not Correct, Ignore Packet\n");
#endif 				
				//
				// Flush Command Buffer 
				Flush_Packet_Buffer(Packet_Buffer, Pack_End_Indx);
				return PackInvalid;						
			}

		}
	}
	
	
	//
	// Generate Check-Sum 
	Temp = Generate_CheckSum ( Packet_Buffer, (Header_Index+BRD_CTL_Index), BusRxt_Pack.PackLens);
	
#ifdef BRD_Debug
	printf("Check Sum Base: %x\n", Header_Index+BRD_CTL_Index);
	printf("Generate Check Sum : %x\n", Temp);
#endif  
	//
	// Check Check-Sum
    if (Temp != BusRxt_Pack.CheckSum)
	{
#ifdef BRD_Debug
	printf("Check Sum MissMatch: Source->%x != Local->%d\n", BusRxt_Pack.CheckSum , Temp);
#endif 
		Flush_Packet_Buffer(Packet_Buffer, Pack_End_Indx);
		return PackInvalid;
	}
	
	//
	// Extract Packet Type
	switch (BusRxt_Pack.CMD_Type)
	{
                //
                //Reset JB to ColmEmpty
                case ctReset:
                        //Reset_ClearRegistFlag_Counter();
                        //JB_LocalDev.State = ColmEmpty;
                        WDTCTL = WRONG_WDTPW + WDTHOLD; //RESET
                        return NoQueueOut;
		//
		// PV Value Response
		case ctPV_Val:
		  
		  	Reset_ClearRegistFlag_Counter();
#ifdef Display_BRD_INFO
			printf("Need to Send PV Value\n");
#endif
			//
			OutIndex = 0;
	
			BusRxt_Pack.CTL_Field = (unsigned char)clToMaster;	
			BusRxt_Pack.BusID = JB_LocalDev.BusID;	
			BusRxt_Pack.CMD_Type = (unsigned char)ctPV_Val;	
			
			//
			// Packet Lens
			BusRxt_Pack.PackLens = (BRD_CTL_Byte+BRD_BusID_Byte+BRD_Type_Byte)+(sizeof(PV_ValueFrame)-1);
			

			//printf("Packe Lens : %d, frameSize %d\n", BusRxt_Pack.PackLens, sizeof(PV_ValueFrame)-1);
			
			Packet_Buffer[OutIndex++] = BRD_Header; 
			Packet_Buffer[OutIndex++] = (unsigned char)(((BusRxt_Pack.PackLens)<<8)&0xff); 
			Packet_Buffer[OutIndex++] = (unsigned char)((BusRxt_Pack.PackLens)&0xff); 
			
			Packet_Buffer[OutIndex++] = BusRxt_Pack.CTL_Field; 
			Packet_Buffer[OutIndex++] = BusRxt_Pack.BusID; 
			Packet_Buffer[OutIndex++] = BusRxt_Pack.CMD_Type;
			
			//
			// Load Payload
                        //Update_PV_Values (pvPowerInfo);
			Payload = (unsigned char *)&(JB_LocalDev.PvValue.DiodeTemp);
			
			//printf("\n\nShow Value Frame Size : %d\n\n", (sizeof(PV_ValueFrame)- 1));
			
				/*JB_LocalDev.PvValue.DiodeTemp = 0x64;
                                  JB_LocalDev.PvValue.Voltage = 0x1987;
                                  JB_LocalDev.PvValue.Current = 0x0538;
                                  JB_LocalDev.PvValue.Power = 0x0001550b;
                                   JB_LocalDev.PvValue.AlertState = 0x0;*/
			for (i=0; i<(sizeof(PV_ValueFrame)- 1); i++)
			{
				Packet_Buffer[OutIndex++] = Payload[i];
			}
			
			Packet_Buffer[OutIndex++] = Generate_CheckSum ( Packet_Buffer, BRD_CTL_Index, BusRxt_Pack.PackLens);
			Packet_Buffer[OutIndex++] = BRD_Tail;		
		break;
		
		//
		// PV Info Response
		case ctPV_Info:
		  	Reset_ClearRegistFlag_Counter();
#ifdef Display_BRD_INFO
			printf("Need to Send PV INFO\n");
#endif
			DbStrPrint( "Get INFO");
			//
			OutIndex = 0;
	
			BusRxt_Pack.CTL_Field = (unsigned char)clToMaster;	
			BusRxt_Pack.BusID = JB_LocalDev.BusID;	
			BusRxt_Pack.CMD_Type = (unsigned char)ctPV_Info;	
			
			//
			// Packet Lens
			BusRxt_Pack.PackLens = (BRD_CTL_Byte+BRD_BusID_Byte+BRD_Type_Byte)+(sizeof(PV_InfoFrame)- 1);
			

			printf("Packe Lens : %d, frameSize %d\n", BusRxt_Pack.PackLens, sizeof(PV_InfoFrame)-1);
			
			Packet_Buffer[OutIndex++] = BRD_Header; 
			Packet_Buffer[OutIndex++] = (unsigned char)(((BusRxt_Pack.PackLens)<<8)&0xff); 
			Packet_Buffer[OutIndex++] = (unsigned char)((BusRxt_Pack.PackLens)&0xff); 
			
			Packet_Buffer[OutIndex++] = BusRxt_Pack.CTL_Field; 
			Packet_Buffer[OutIndex++] = BusRxt_Pack.BusID; 
			Packet_Buffer[OutIndex++] = BusRxt_Pack.CMD_Type;
			
			//
			// Load Payload
			Payload = (unsigned char *)&(JB_LocalDev.JB_INFO.DeviceMAC[0]);
			for (i=0; i<(sizeof(PV_InfoFrame)- 1); i++)
			{
				Packet_Buffer[OutIndex++] = Payload[i];
			}
			
			Packet_Buffer[OutIndex++] = Generate_CheckSum ( Packet_Buffer, BRD_CTL_Index, BusRxt_Pack.PackLens);
			Packet_Buffer[OutIndex++] = BRD_Tail;	

		break;
		
		
		//
		// ID Assignment Acknowldgement.
		case ctAsign_Ack:
		  

			
			//
			// Load Device MAC
			//for (i=0; i<6; i++)
			//	RxQPack->QFIFO[RxQPack->FIFOIdx].PayLoad[i] = Packet_Buffer[(Header_Index+BRD_PayL_Index)+i];
	
		return NoQueueOut;
		
		case ctCH_SCAN:
#ifdef Display_BRD_INFO
			printf("Get Channel Scan \n");
#endif
                        RESETTIMER = Packet_Buffer[(Header_Index+BRD_PayL_Index)]*256+Packet_Buffer[(Header_Index+BRD_PayL_Index+1)];
			//RESETTIMER = 60;
                        //
			// Check Local JOining State
			if (JB_LocalDev.State != DevIsJoin2AM)
			{
			  	Reset_ClearRegistFlag_Counter();
				OutIndex = 0;
				
				DbStrPrint("SC");
				
				//
				// Perform Random Delay to Reply Scan 1ms~2000mSec.
				RandomDelay = (uint16_t ) urand()%2000;

				//
				// Start Blockig Timer
				Start_BRD_Timer ( RandomDelay);
				//RandomDelay
				
				//
				// Block until Lock is release.
				while ( Get_BRD_TimerLock_State() != 1);
				  

				
				BusRxt_Pack.CTL_Field = (unsigned char)clToMaster;	
				BusRxt_Pack.BusID = JB_LocalDev.BusID;	
				BusRxt_Pack.CMD_Type = (unsigned char)ctJOIN_Req;	
				
				
				Packet_Buffer[OutIndex++] = BRD_Header; 
				//
				// Packet Lens
				BusRxt_Pack.PackLens = BRD_CTL_Byte+BRD_BusID_Byte+BRD_Type_Byte+6;
				Packet_Buffer[OutIndex++] = (unsigned char)(((BusRxt_Pack.PackLens)<<8)&0xff); 
				Packet_Buffer[OutIndex++] = (unsigned char)((BusRxt_Pack.PackLens)&0xff); 
				
				Packet_Buffer[OutIndex++] = BusRxt_Pack.CTL_Field; 
				Packet_Buffer[OutIndex++] = BusRxt_Pack.BusID; 
				Packet_Buffer[OutIndex++] = BusRxt_Pack.CMD_Type;
				
				//
				// Load Payload - JB MAC
				for (i=0; i<6; i++)
					Packet_Buffer[OutIndex++] = JB_LocalDev.JB_INFO.DeviceMAC[i];
				
				Packet_Buffer[OutIndex++] = Generate_CheckSum ( Packet_Buffer, BRD_CTL_Index, BusRxt_Pack.PackLens);
				Packet_Buffer[OutIndex++] = BRD_Tail;

			
				JB_LocalDev.State = DevIsReqJoin;
			}
			else 
				return NoQueueOut;
			
			
		break;
		
		case ctAssignID:
#ifdef Display_BRD_INFO
			printf("Get Assign ID\n");
#endif			
			//DbStrPrint("AID");
			//
		  	// Match MAC
		  	for (i=0; i<6; i++)
		  	{
				if (Packet_Buffer[(Header_Index+BRD_PayL_Index+1)+i] != JB_LocalDev.JB_INFO.DeviceMAC[i])
				  break;
			}
		  	if (i != 6)	// MAC is Not Match
		    	return NoQueueOut;
			
			//
			// Update Local Bus ID and State, if assignment is valid.
			//printf("DB State : %x\n", Packet_Buffer[Header_Index+BRD_PayL_Index]);
			
			if ( Packet_Buffer[Header_Index+BRD_PayL_Index] == (unsigned char) abBusIDisValid)
			{
				pMAC = (unsigned char *)&(Packet_Buffer[Header_Index+BRD_PayL_Index+1]);	
#ifdef BRD_Debug
				printf("Get BusID : %d\n", BusRxt_Pack.BusID);
#endif		
				//
				// Matching Device MAC 
				for (i=0; i<6; i++)
				{
					if ( pMAC[i] != JB_LocalDev.JB_INFO.DeviceMAC[i])
						break;
				}
				if ( i == 6)
				{	//
					// Matching
					JB_LocalDev.BusID = BusRxt_Pack.BusID;
					//JB_LocalDev.State = DevIsJoin2AM;
                                        JB_LocalDev.State = DevIsReqJoin;
					
#ifdef Display_BRD_INFO
					printf("Get BusID : %d\n", JB_LocalDev.BusID);
#endif				
					DbStrPrint("Get ID");
					/*DebStr[0] = BusRxt_Pack.BusID + 0x30;
					DebStr[1] = 0x0;
					
					DB_Burst_Rs485_Tx( DebStr, 1);
					DbStrPrint(DebStr);*/
					
					
					//
					// Send Ack
					OutIndex = 0;
					
					BusRxt_Pack.CTL_Field = (unsigned char)clToMaster;	
					BusRxt_Pack.BusID = JB_LocalDev.BusID;	
					BusRxt_Pack.CMD_Type = (unsigned char)ctAsign_Ack;	
					
					
					Packet_Buffer[OutIndex++] = BRD_Header; 
					//
					// Packet Lens
					BusRxt_Pack.PackLens = (BRD_CTL_Byte+BRD_BusID_Byte+BRD_Type_Byte)+6;
					Packet_Buffer[OutIndex++] = (unsigned char)(((BusRxt_Pack.PackLens)<<8)&0xff); 
					Packet_Buffer[OutIndex++] = (unsigned char)((BusRxt_Pack.PackLens)&0xff); 
					
					Packet_Buffer[OutIndex++] = BusRxt_Pack.CTL_Field; 
					Packet_Buffer[OutIndex++] = BusRxt_Pack.BusID; 
					Packet_Buffer[OutIndex++] = BusRxt_Pack.CMD_Type;
					
					//
					// Load Payload - JB MAC
					for (i=0; i<6; i++)
						Packet_Buffer[OutIndex++] = JB_LocalDev.JB_INFO.DeviceMAC[i];
					
					Packet_Buffer[OutIndex++] = Generate_CheckSum ( Packet_Buffer, BRD_CTL_Index, BusRxt_Pack.PackLens);
					Packet_Buffer[OutIndex++] = BRD_Tail;
	


#ifdef Display_BRD_INFO
					printf("Junction Box iS Joining to Array Manager\n");
#endif
				}			
			}

		break;
			
		case ctACK_Resp:
#ifdef Display_BRD_INFO
			printf("Get ACK Response\n");	
#endif
			JB_LocalDev.State = DevIsJoin2AM;	
#ifdef Display_BRD_INFO
			printf("Junction Box iS Joining to Array Manager\n");
#endif	
			
		return NoQueueOut;

		
		default:
#ifdef Display_BRD_INFO
			printf("Warnning : Undefined CMD Type\n");
#endif			
			//
			// Flush Command Buffer 
			Flush_Packet_Buffer(Packet_Buffer, Pack_End_Indx);
		return PackInvalid;
		
	}
				

        BRP_Pack_Out( Packet_Buffer,OutIndex);

	//DB_Burst_Rs485_Tx( Packet_Buffer, OutIndex);
	
#ifdef BRD_Debug_TX_DUMP
	printf("\n: Dump TX Pack :\n");
	for (i=0; i<OutIndex; i++)
		printf("%x ", Packet_Buffer[i]);
	printf("\n\n");
	printf("Output Lens : %d\n",OutIndex);
#endif	
	//
	// Flush Packet Buffer 
	if ( OutIndex> Pack_End_Indx)
		Flush_Packet_Buffer(Packet_Buffer, OutIndex);
	else
		Flush_Packet_Buffer(Packet_Buffer, Pack_End_Indx);
	


	return QueueOut;
}
#define DB_JB_infoValue
void Update_PV_Values (void)
{
	/*JB_LocalDev.PvValue.DiodeTemp = dt.DiodeTemp;
	JB_LocalDev.PvValue.Voltage = dt.Voltage;
	JB_LocalDev.PvValue.Current = dt.Current;
	JB_LocalDev.PvValue.Power = dt.Power;
	JB_LocalDev.PvValue.AlertState = dt.AlertState;*/
  	Emeter *pDcEmeter;
  	
	pDcEmeter = Get_DcEmeter();
	if ( (pDcEmeter->SampleDoneFlag) == 0x01)
	{
	  if(pDcEmeter->Diode_Temp<0)
          {
            JB_LocalDev.PvValue.DiodeTemp = (-(pDcEmeter->Diode_Temp))+150;   ////-40 - 150
          } 
          else
          {    
          JB_LocalDev.PvValue.DiodeTemp = (pDcEmeter->Diode_Temp);
          }
	  JB_LocalDev.PvValue.Voltage = (pDcEmeter->DigiVrms)/10; 		// Truncate to xx.xx Vol
	  JB_LocalDev.PvValue.Current = (pDcEmeter->DigiIrms)/10;	  	// Trncate to xx.xx A
	  JB_LocalDev.PvValue.Power = (pDcEmeter->DigiavgPower)/10;
	  JB_LocalDev.PvValue.AlertState = 0;
	  
	  pDcEmeter->SampleDoneFlag = 0;
	}
        
#ifdef DB_JB_infoValue
        //unsigned 
        char buf [100];
        //char i;
        int Convert_Temp;
        //
        // Display Voltage to Rs232
        sprintf(buf,"Voltage : %u mV\n\r",JB_LocalDev.PvValue.Voltage * 10);
        for(int i = 0;buf[i]!='\0';i++){
          while (!(UCA1IFG & UCTXIFG)) ;  // USCI_A0 TX buffer ready?
          UCA1TXBUF = buf[i];             // TX -> RXed character
        }
        
        //
        // Display Current to Rs232    
        sprintf(buf,"Current : %u mA\n\r",(int)pDcEmeter->DigiIrms / 10);
        for(int i = 0;buf[i]!='\0';i++){
          while (!(UCA1IFG & UCTXIFG)) ;  // USCI_A0 TX buffer ready?
          UCA1TXBUF = buf[i];             // TX -> RXed character
        }
        
        //
        // Display Power to Rs232
        if(pDcEmeter->DigiavgPower > 65536)
        {
          unsigned int h_temp = pDcEmeter->DigiavgPower / 100;
          unsigned int l_temp = pDcEmeter->DigiavgPower % 100;
          sprintf(buf,"Power : %u%u mW\n\r",h_temp,l_temp);
        }
        else
          sprintf(buf,"Power : %u mW\n\r",(int)pDcEmeter->DigiavgPower);
        
        for(int i = 0;buf[i]!='\0';i++){
          while (!(UCA1IFG & UCTXIFG)) ;  // USCI_A0 TX buffer ready?
          UCA1TXBUF = buf[i];             // TX -> RXed character
        }
        
        //
        // Display DiodeTemp to Rs232
        Convert_Temp = JB_LocalDev.PvValue.DiodeTemp;
        sprintf(buf,"DiodeTemp : %d �J\n\r-----------------------\n\r",Convert_Temp-50);
        for(int i = 0;buf[i]!='\0';i++){
          while (!(UCA1IFG & UCTXIFG)) ;  // USCI_A0 TX buffer ready?
          UCA1TXBUF = buf[i];             // TX -> RXed character
        }
        
        //
        //Print_Debug_Mode
        sprintf(buf,"JB_LocalDev.State : %x\n\rRESETTIMER : %d\n\r-----------------------\n\r",JB_LocalDev.State,RESETTIMER);
        for(int i = 0;buf[i]!='\0';i++){
          while (!(UCA1IFG & UCTXIFG)) ;  // USCI_A0 TX buffer ready?
          UCA1TXBUF = buf[i];             // TX -> RXed character
        }
        
        //
        //Print_ADC_Debug_Mode
        sprintf(buf,"A/D : %d\n\r-----------------------\n\r",ADC10MEM0);
        for(int i = 0;buf[i]!='\0';i++){
          while (!(UCA1IFG & UCTXIFG)) ;  // USCI_A0 TX buffer ready?
          UCA1TXBUF = buf[i];             // TX -> RXed character
        }
       
        
#endif
        
}
#define JB_SerialNum 	"JB1345-785"
#define JB_FirVersion	"JB TDK v0.4b"
#define JB_HardVersion	"JB Single v0.5b"
#define JB_DeviceSpec	"Rush Plumber"
#define JB_MANU_Date	"20121221"	
extern ENVCONFIG G_ENVCONFIG;
void Init_Bus_Loacal_Device_State_Struct (void)
{
	unsigned int i;
	//
	// Load MAC From ROM
	/*JB_LocalDev.JB_INFO.DeviceMAC[0] = 0xdc;	
	JB_LocalDev.JB_INFO.DeviceMAC[1] = 0xa1;	
	JB_LocalDev.JB_INFO.DeviceMAC[2] = 0xa2;	
	JB_LocalDev.JB_INFO.DeviceMAC[3] = 0xa3;	
	JB_LocalDev.JB_INFO.DeviceMAC[4] = 0xcc;	
	JB_LocalDev.JB_INFO.DeviceMAC[5] = 0x29;*/
        //
	// Load MAC From INFO FLASH @1800~@1805
        Get_EvnString();
        JB_LocalDev.JB_INFO.DeviceMAC[0] = G_ENVCONFIG.Mac[0];	
	JB_LocalDev.JB_INFO.DeviceMAC[1] = G_ENVCONFIG.Mac[1];	
	JB_LocalDev.JB_INFO.DeviceMAC[2] = G_ENVCONFIG.Mac[2];	
	JB_LocalDev.JB_INFO.DeviceMAC[3] = G_ENVCONFIG.Mac[3];	
	JB_LocalDev.JB_INFO.DeviceMAC[4] = G_ENVCONFIG.Mac[4];	
	JB_LocalDev.JB_INFO.DeviceMAC[5] = G_ENVCONFIG.Mac[5];
        //
	// Load MAC From INFO FLASH @1800~@1805 OLD
        /*unsigned char  *mac_ptr = (unsigned char *)0x1800;
        JB_LocalDev.JB_INFO.DeviceMAC[0] = *mac_ptr;
        mac_ptr = (unsigned char *)0x1801;	
	JB_LocalDev.JB_INFO.DeviceMAC[1] = *mac_ptr;
        mac_ptr = (unsigned char *)0x1802;
	JB_LocalDev.JB_INFO.DeviceMAC[2] = *mac_ptr;	
        mac_ptr = (unsigned char *)0x1803;
	JB_LocalDev.JB_INFO.DeviceMAC[3] = *mac_ptr;
        mac_ptr = (unsigned char *)0x1804;	
	JB_LocalDev.JB_INFO.DeviceMAC[4] = *mac_ptr;
        mac_ptr = (unsigned char *)0x1805;
	JB_LocalDev.JB_INFO.DeviceMAC[5] = *mac_ptr;*/
	
	
	//
	// Lonad Infor From ROM
        
    	for (i=0; i<22; i++)
	{   
         if ( G_ENVCONFIG.SeriesNumber[i] == '\0')
		{
			JB_LocalDev.JB_INFO.SerialNum[i] = '\0';
			break;
		} 
        JB_LocalDev.JB_INFO.SerialNum[i]=G_ENVCONFIG.SeriesNumber[i];	
        
        }
        JB_LocalDev.JB_INFO.SerialNum[23] = '\0';
        
#if 0       
	for (i=0; i<22; i++)
	{
		if ( JB_SerialNum[i] == '\0')
		{
			JB_LocalDev.JB_INFO.SerialNum[i] = '\0';
			break;
		}
		JB_LocalDev.JB_INFO.SerialNum[i] = 	JB_SerialNum[i];
	}
	JB_LocalDev.JB_INFO.SerialNum[23] = '\0';
#endif	
	//
	// Firmware Version.
	for (i=0; i<22; i++)
	{
		if ( JB_FirVersion[i] == '\0')
		{
			JB_LocalDev.JB_INFO.FirmVer[i] = '\0';
			break;
		}
		JB_LocalDev.JB_INFO.FirmVer[i] = JB_FirVersion[i];
	}
	JB_LocalDev.JB_INFO.FirmVer[23] = '\0';	
	
	//
	// Load Hardware Version
	for (i=0; i<22; i++)
	{
		if ( JB_HardVersion[i] == '\0')
		{
			JB_LocalDev.JB_INFO.HardVer[i] = '\0';
			break;
		}
		JB_LocalDev.JB_INFO.HardVer[i] = JB_HardVersion[i];
	}
	JB_LocalDev.JB_INFO.HardVer[23] = '\0';	
	
	//
	// Load Device Specification.
	for (i=0; i<22; i++)
	{
		if ( JB_FirVersion[i] == '\0')
		{
			JB_LocalDev.JB_INFO.Dev[i] = '\0';
			break;
		}
		JB_LocalDev.JB_INFO.Dev[i] = JB_DeviceSpec[i];
	}
	JB_LocalDev.JB_INFO.Dev[23] = '\0';		
	
	//
	// Load Production Date
	for (i=0; i<8; i++)
	{
		JB_LocalDev.JB_INFO.ManuDate[i] = JB_MANU_Date[i];
	}


/*	JB_LocalDev.JB_INFO.FirmVer[24];		
	JB_LocalDev.JB_INFO.HardVer[24];		// Hardware Version.
	JB_LocalDev.JB_INFO.Dev[24];			// 	
	JB_LocalDev.JB_INFO.ManuDate[8];	*/
	//
	// Reset State and Bus ID
	JB_LocalDev.BusID = BRD_Dev_Default_ID;
	JB_LocalDev.State = ColmEmpty;	
	
	
		
	//Update_PV_Values ();
}


//*****************************************************************************
//	Check Registed Timeout
// 	- Put this function to main loop for checking the link between 
//	array manager and junction box.
//*****************************************************************************	
void Check_Registed_TimeOut (void)
{
	if ( GET_ClearRegistFlag() == 1)
	{
		//
	  	// It's too long to get requestion from Array Manager
	  	// Clear the state of Registed to Array Manger.
		//JB_LocalDev.State = 0;
                WDTCTL = WRONG_WDTPW + WDTHOLD; //RESET
	}
}
//*****************************************************************************
//	Read MAC file
//*****************************************************************************	
int Get_EvnString(void)
{
	char* ptr ;
	int stringCount=0;
	char StringBuf[10][50];
	char TempBuf[100];
	int tempbufindex=0;
	int retConut=0;
	char* parserStart;
	int i;

	ptr=(char*)EnvStartAdd;
	memset(StringBuf,0,sizeof(StringBuf));

	while(retConut<ContiRetCount && tempbufindex<40)
	{

		if(*ptr==0x0d && *(ptr+1)==0x0a)
		{
			TempBuf[tempbufindex]=0;
			strcpy(StringBuf[stringCount],TempBuf);
			stringCount+=1;
			tempbufindex=0;
			retConut=0;
			ptr++;
		}
		else
		{
			if(*ptr==0xff)
			{
				retConut++;
			}
			TempBuf[tempbufindex++]=*ptr;

		}

		ptr++;


	}

	for( i=0;i<stringCount;i++)
	{
		if((parserStart=strstr(StringBuf[i],"MAC=")))
		{
			SaveMac2Evn(parserStart+strlen("MAC="));
		}

		if((parserStart=strstr(StringBuf[i],"SN=")))
		{
			SaveSNEvn(parserStart+strlen("SN="));
		}

	}


	return stringCount;
}

int SaveMac2Evn(char*Start)
{
		uint8_t RetHex[1];


		G_ENVCONFIG.Mac[0]=*hex_decode(Start,2,RetHex);
		Start=strstr(Start,":")+1;
		G_ENVCONFIG.Mac[1]=*hex_decode(Start,2,RetHex);
		Start=strstr(Start,":")+1;
		G_ENVCONFIG.Mac[2]=*hex_decode(Start,2,RetHex);
		Start=strstr(Start,":")+1;
		G_ENVCONFIG.Mac[3]=*hex_decode(Start,2,RetHex);
		Start=strstr(Start,":")+1;
		G_ENVCONFIG.Mac[4]=*hex_decode(Start,2,RetHex);
		Start=strstr(Start,":")+1;
		G_ENVCONFIG.Mac[5]=*hex_decode(Start,2,RetHex);

		return 0;
}

int SaveSNEvn(char*Start)
{

                memset(G_ENVCONFIG.SeriesNumber,0,sizeof(G_ENVCONFIG.SeriesNumber));
                
                strcpy((char*)G_ENVCONFIG.SeriesNumber,Start);
                
		return 0;
}

uint8_t* hex_decode(char *in, size_t len, uint8_t *out)
{
    unsigned int i, t, hn, ln;

    for (t = 0,i = 0; i < len; i+=2,++t) {

            hn = in[i] > '9' ? (in[i]|32) - 'a' + 10 : in[i] - '0';
            ln = in[i+1] > '9' ? (in[i+1]|32) - 'a' + 10 : in[i+1] - '0';

            out[t] = (hn << 4 ) | ln;

    }
    return out;

}





