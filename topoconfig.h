#ifndef TOPOCONFIG_H_TEMPLATE_
#define TOPOCONFIG_H_TEMPLATE_
//***********topology parameters***********
#define PortNum 4
#define ProcessorNum 16
//#define PortNum 16
//#define ProcessorNum 16
#define LinkNum 96
#define PacketSize 16
#define FlitSize 4
#define FlitLength 4
#define VC 3
#define BufferDepth 32 * FlitLength
#define ProcessorBufferDepth 32 * FlitLength
#define CrossPointBufferDepth 8
#define FREQ 3500000000.0
#define OutBufferDepth 3
#define RecordStartTime 1.0000006
//*************unchangable variable***************
#define CLK_CYCLE 1/FREQ
#define Sim_Start_Time 1
#define TimeScale 0.1
//*************injection mode***************
#define UNIFORM
#define LAMBDA 7
#define INJECTION_RATE 1.0
//*************debug infomation***************
#define Verbose 1
#define VERBOSE_DEBUG_MESSAGES 1
#define VERBOSE_DETAIL_DEBUG_MESSAGES 2
//************power infomation***************
#define LVT 1
#define NVT 2
#define HVT 3
#define VDD 1.0
#define PARM(x) PARM_ ## x
#define PARM_TECH_POINT 45
#define PARM_TRANSISTOR_TYPE NVT
#define FlitWidth FlitSize * 8
//***********end of parameter definition*****
#endif /* TOPOCONFIG_H_TEMPLATE_ */
