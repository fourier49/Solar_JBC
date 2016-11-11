

#ifndef __PVINFOFRAME_H__
#define __PVINFOFRAME_H__

//*****************************************************************************
//
// Bus Raw Data Protocol - PV Info Frame
//
//*****************************************************************************
typedef struct PV_Info
{
	unsigned char DeviceMAC[6];		// Junction Box MAC.
	unsigned char SerialNum[24];	// Serial Number.
	unsigned char FirmVer[24];		// Firmware Version.
	unsigned char HardVer[24];		// Hardware Version.
	unsigned char Dev[24];			// Device Specification.	
	unsigned char ManuDate[8];		// Manufacture Date.
	unsigned char BusID;
	//unsigned char Pad;
} PV_InfoFrame;

//*****************************************************************************
//
// Bus Raw Data Protocol - PV Value Frame
//
//*****************************************************************************
typedef struct PV_Value
{
	unsigned char BusID;
	unsigned char DiodeTemp;
	unsigned short Voltage;
	unsigned short Current;
	unsigned long Power;
	unsigned long AlertState;
} PV_ValueFrame;
//*****************************************************************************
//
// API Function prototypes
//
//*****************************************************************************


#endif //  __PVINFOFRAME_H__
