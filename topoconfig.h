/*
 * topoconfig.h
 *
 *  Created on: 2017年4月25日
 *      Author: Vincent
 */

#ifndef TOPOCONFIG_H_
#define TOPOCONFIG_H_


/*
//tianhe_router
//网络拓扑参数
#define PortNum 24
#define ProcessorNum 24

//#define LinkNum 48 //双向链路乘以2，用于功耗仿真
//曙光5000相关参数，16端口，3虚通道，MTU 1KB，Buffer 4MTU

#define PacketSize 128 //Byte为单位
#define FlitSize 4 //Byte为单位
#define FlitLength 32 // PacketSize / FlitSize， FlitLength必须大于等于2，否则processor产生的数据包会发生错误

#define VC 3 //virtual channel
#define BufferDepth 4 * FlitLength //virtual channel buffer depth, 大于等于2，可存放Flit的数量
#define ProcessorBufferDepth 4 * FlitLength // processor txQueue 的大小

//时钟相关参数
#define FREQ 3500000000.0  //用于功耗仿真，单位hz

*/

#define PortNum 24
#define ProcessorNum 24
#define PacketSize 128
#define FlitSize 4
#define FlitLength 32
#define VC 3
#define BufferDepth 4 * FlitLength
#define ProcessorBufferDepth 4 * FlitLength
#define FREQ 3500000000.0
#define OutBufferDepth 29

//******************以下手工修改，以上自动生成*******************8

#define CLK_CYCLE 1/FREQ //时钟周期
#define Sim_Start_Time 1 //1s 开始仿真
#define TimeScale 0.1 //不改，用于泊松分布和自相似分布，如lambda=10，表示1s内10个flit，得到的时间间隔除以TimeScale再round取整

//Spatial Distribution
#define UNIFORM //空间均匀分布

//Time Distribution
//#define SELF_SIMILARITY //自相似分布
//#define POISSON_DIST //采用泊松分布

//自相似和泊松分布的取值范围小于10，单位时间为1s，但TimeScale会进行换算，最后乘以时钟周期
//自相似和泊松分布的参数

//自相似分布Pareto参数
//#define ALPHA_ON 4
//#define ALPHA_OFF 2

//Poisson分布参数
#define LAMBDA 7 //泊松分布中用于产生时间间隔的指数分布的lambda，表示单位时间内(1s)到达的帧数，其倒数为时间间隔的平均值
//Uniform分布参数
#define INJECTION_RATE 1.8 //注入率，范围在0.1 - 1之间

//调试信息
#define Verbose 1
#define VERBOSE_DEBUG_MESSAGES 1
#define VERBOSE_DETAIL_DEBUG_MESSAGES 2



//功耗相关参数
//晶体管工艺参数
#define LVT 1
#define NVT 2
#define HVT 3

//#define TR 0.2//toggle rate
#define VDD 1.0
#define PARM(x) PARM_ ## x
#define PARM_TECH_POINT 45
#define PARM_TRANSISTOR_TYPE NVT
#define FlitWidth FlitSize * 8



#endif /* TOPOCONFIG_H_ */
