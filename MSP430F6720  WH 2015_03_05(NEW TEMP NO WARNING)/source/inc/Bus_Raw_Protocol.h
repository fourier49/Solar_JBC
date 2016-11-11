

#ifndef __BUS_RAW_PROTOCOL_H__
#define __BUS_RAW_PROTOCOL_H__

//#include "Tasks/Bus_Task.h"
//#include "Tasks/PVInfoFrame.h"
#include "PVInfoFrame.h"


//*****************************************************************************
//
// Bus Raw Data Protocol Parameters
//
//*****************************************************************************
#define MAX_REQ_NumPV_Value		20
#define MAX_NumDev_JOIN			10

#define MAX_Bus_Channel 		1
#define MAX_NumBus_Member 		256

#define MAX_NumBus_Candidate 	3

#define MAX_TxPack_Buff_Size	30


#define svChannelSCAN_Period	5000	// mSec
#define svJB_JoinCheck_Period	50 	// mSec
#define svJB_JoinAccTime		(svJB_JoinCheck_Period*MAX_NumDev_JOIN*3)	// mSec
#define svPeriodicData_TxPeriod	5000	//
#define svPeriodicLINK_Period	svPeriodicData_TxPeriod*10	//
#define svPeriodicTx_TimeoutTime 1000	// mSec


#define TX_TASK_GateWay_LockTIme_Normal	2	// mSec
#define TX_TASK_GateWay_LockTIme_Scan	50	// mSec


#define BRD_Dev_Default_ID 0xfe
//*****************************************************************************
//
// Bus Raw Data Protocol Packet
//
//*****************************************************************************
typedef struct Bus_RawData_Packet
{
	unsigned char HeaderCode;
	unsigned short PackLens;
	unsigned char CTL_Field;	
	unsigned char BusID;
	unsigned char CMD_Type;
	unsigned char *Payload;
	unsigned char CheckSum;
	unsigned char TailCode;
} Bus_Raw_Packet;

#define BRD_Header	0x4a
#define BRD_Tail	0x3b




//*****************************************************************************
//
// Bus Rx/Tx Task Massage Queue  
//
//*****************************************************************************
#define MAX_BRP_Payload_Size 128
/*
typedef struct QBusMessage
{
	unsigned char CH;
	unsigned char BusID;
	unsigned char Type;
	unsigned char PayLoad[MAX_BRP_Payload_Size];
} QBusMsg;


typedef struct BusMsgPack
{
	QBusMsg *QFIFO;
	unsigned char FIFOIdx;
	xQueueHandle QHandle;
}BusQPack;
*/

//*****************************************************************************
//
// Bus Raw Data Protocol Packet - Number of Byte
//
//*****************************************************************************
#define BRD_Head_Byte		1
#define BRD_Lens_Byte		2
#define BRD_CTL_Byte		1
#define BRD_BusID_Byte		1
#define BRD_Type_Byte		1
#define BRD_ChSUM_Byte		1
#define BRD_Tail_Byte		1


//*****************************************************************************
//
// Bus Raw Data Protocol Packet - Index Macro
//
//*****************************************************************************
#define BRD_Lens_Index 			(BRD_Head_Byte)
#define BRD_CTL_Index 			(BRD_Head_Byte+BRD_Lens_Byte)
#define BRD_BusID_Index 	 	(BRD_CTL_Index+BRD_CTL_Byte)
#define BRD_Type_Index 			(BRD_BusID_Index+BRD_BusID_Byte)
#define BRD_PayL_Index 			(BRD_Type_Index+BRD_Type_Byte)
#define BRD_ChSUM_Index(LENS) 	(BRD_Head_Byte+BRD_Lens_Byte+LENS)
#define BRD_Tail_Index(LENS) 	(BRD_ChSUM_Index (LENS) + BRD_ChSUM_Byte)
#define BRD_PayLoad_lens(LENS) 	(LENS - (BRD_Type_Byte+BRD_BusID_Byte+BRD_CTL_Byte))
#define BRD_WholePacket_lens(LENS) 	(LENS + (BRD_Head_Byte+BRD_Lens_Byte+BRD_ChSUM_Byte+BRD_Tail_Byte))


//*****************************************************************************
//
// Bus Member Table
//
//*****************************************************************************
typedef struct Bus_DeviceState_Table
{
	//unsigned char DevMAC[6];
	unsigned char State;
	unsigned char BusID;
	PV_InfoFrame JB_INFO;
	PV_ValueFrame PvValue;
}bBusDevState;
//#define DevIsRegisted 		0xaa
#define DevIsJoin2AM 		0xaa
#define DevIsReqJoin 		0x1c
#define DevIsJoin2Server	0xc4
#define DevReady2Server		0xA5
#define DevReqINFO		0xA8
#define ColmEmpty	 	0x00
//*****************************************************************************
//
// Bus Raw Data Protocol - Control Field Table
//
//*****************************************************************************
#define clBroadcast		(1<<0)	
#define clFromMaster	        (1<<1)	
#define clToMaster		(0<<1)	
#define clScan			(1<<2)	

//*****************************************************************************
//
// Bus Raw Data Protocol - Command Type Table
//
//*****************************************************************************
#define ctReset         (0x02)
#define ctPV_Val	(0x03)	
#define ctPV_Info	(0x04)
#define ctCH_SCAN	(0x05)
#define ctJOIN_Req	(0x06)
#define ctAssignID	(0x07)
#define ctAsign_Ack	(0x08)
#define ctACK_Resp	(0x09)

//
// Internal Type
#define ctJB_Periodic   (0xfe)
#define ctJB_JOIN	(0xff)

//*****************************************************************************
//
// Bus Raw Data Protocol - Command Payload Size
//
//*****************************************************************************
#define ctDevMAC_Lens 	(6)
#define ctAssment_Lens 	(1)
#define ctAssID_Lens (ctDevMAC_Lens+ctAssment_Lens)



//*****************************************************************************
//
// Bus Raw Data Protocol - Assign ID Frame Definition
//
//*****************************************************************************
#define abBusIDisValid 	0x0
#define abWaitNextCyc 	0x1
#define abReqDenied	0x2
//*****************************************************************************
//
// Bus Raw Data Protocol Decoding Engine State
//
//*****************************************************************************
typedef enum
{
	csCMD_Accepted = 0,					
	csCMD_START_WRONG,			
	csCMD_ID_MisMatch,			
	csCheckSum_WRONG,			
	csCMD_Undefined,				
	csCMD_NotReady,				
	csCMD_EOC_WRONG,				
	csCMD_InValid				
} BUS_Protcol_Decode_State;

typedef enum
{
	CL_Sate_NotReady = 0,	
	CL_State_Invalid, 	
	CL_Sate_WrStart				
} Bus_Decode_State;



//*****************************************************************************
//
// Function State
//
//*****************************************************************************
typedef enum
{
	QueueOut = 0,
	NoQueueOut,
	SendFail,
	PackNotReady,
	PackInvalid
}stPackProcess;
//*****************************************************************************
//
// API Function prototypes
//
//*****************************************************************************

stPackProcess BusRaw_DeviceDecode_Engine (unsigned char *Packet_Buffer, unsigned char Pack_End_Indx);

void Init_Bus_Loacal_Device_State_Struct (void);
void TEST(unsigned char *Packet_Buffer);
void Check_Registed_TimeOut (void);
void Update_PV_Values (void);
#endif //  __BUS_RAW_PROTOCOL_H__

