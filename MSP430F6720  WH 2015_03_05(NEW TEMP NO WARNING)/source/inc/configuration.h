#include <msp430f6720.h>

/*Integer types*/
typedef signed char                             int8_t;
typedef int                                     int16_t;
typedef long int                                int32_t;
typedef long long int                  		int64_t;

typedef unsigned char                           uint8_t;
typedef unsigned int                            uint16_t;
typedef unsigned long int                       uint32_t;
typedef unsigned long long int                  uint64_t;

#ifndef true
 #define true 1
#endif

#ifndef false
 #define false 0
#endif
//*****************************************************************************
//
// Parameter
//
//*****************************************************************************

/******** SD24B ********/
#define Kv 0x6305                           // 1.15 * 221 * 100 (Fractional Numbers of two)
#define Ki 0x599C                           // 1.15 * 200 * 100 (Fractional Numbers of two)
#define Divisor 8388607                   // 2^29 - 2^6 =  536870848
#define Sample_Count 1024                   // SD24 Sample Count is 4096

/******** eUART ********/
#define MAX_CMD_LINE_SIZE 256
#define Tail_Byte 0x3B
//*****************************************************************************
//
// Variable
//
//*****************************************************************************

/******** SD24B ********/
extern uint32_t sample_V,sample_I;          // SD24B Sample
extern uint64_t accSample_V,accSample_I;    // accumlate ADC Sample
extern uint16_t V_rms,I_rms;                // V and I of RMS value
extern uint32_t P_rms;                      // P of RMS value
extern uint32_t res1;                       // temp for RES1
extern uint64_t res2;                       // temp for RES2
extern uint16_t sd24bmemL;                  // temp for SD24BMEML
extern uint32_t sd24bmemH;                  // temp for SD24BMEMH

/******** ADC10_A ********/
//extern uint16_t sample_Temp;                // ADC10 Sample
//extern int32_t adcresult_Temp;              // ADC10 Sample convert
//extern int16_t result_Temp;                 // Temperature value

/******** eUART ********/

extern volatile uint8_t Decode_Flag;                            //Decode_Flag = 1 when read Tail_Byte

/******** Struct ********/
extern struct JB_PowerInfo pvPowerInfo;

/******** RESETTIMER ********/
extern uint16_t RESETTIMER;                // RESETTIMER
//*****************************************************************************
//
// function
//
//*****************************************************************************

void SystemInit(void);
void Cal_DcEmeter_RMS_Value (void);

/******** SD24B ********/
void calAccSample_I(uint32_t sample_I);     // accumulate all Sample for I
void calAccSample_V(uint32_t sample_V);     // accumulate all Sample for V
void calVRMS();                             // calculate V RMS
void calIRMS();                             // calculate I RMS
void calPACT();                             // calculate P ACT

/******** ADC10_A ********/
void calTemp(uint16_t sample_Temp);         // calculate Temp
int32_t Temp_table(int32_t R);


//void tx_data(uint8_t str);                  // Transmit data function
void fill_PV_PowerInfo();                   // fill PV_PowerInfo value
void Decode_Proccess();                     // Decode Array Packet

void Init_Bus_Loacal_Device_State_Struct (void); 
//*****************************************************************************
//
// Structure Define
//
//*****************************************************************************
#pragma pack(push)                          // Save alignment state
#pragma pack(1)                             // alignment to 1 byte

struct JB_PowerInfo                 // PV_PowerInfo
{
  uint8_t  DiodeTemp;
  uint16_t Voltage;
  uint16_t Current;
  uint32_t Power;
  uint32_t AlertState;
};
#define Size_of_JB_PowerInfo sizeof(struct JB_PowerInfo )

struct JB_Value                     // PV_Value
{
  uint8_t  HeaderCode;
  uint16_t PacketLength;
  uint8_t  ControlField;
  uint8_t  BusID;
  uint8_t  CommandType;
  struct JB_PowerInfo Payload;
  uint8_t  CheckSum;
  uint8_t  TailCode;
};

#define Size_of_JB_Value sizeof(struct JB_Value)

#pragma pack(pop)                           // restore alignment state



