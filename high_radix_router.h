/*
 * high_radix_router.h
 *
 *  Created on: 2017年9月27日
 *      Author: Vincent
 */

#ifndef HIGH_RADIX_ROUTER_H_
#define HIGH_RADIX_ROUTER_H_

#include "ft_router.h"

using namespace omnetpp;
/*
 * 17/10/9 排查了两天的性能降低问题，主要表现为，采用high-radix-router后，网路的吞吐率急剧下降，且到达某个时间点后，
 * 部分节点不再接收和发送新的数据。排查发现寄存器的释放时间出现问题。原先的路由器输入仲裁和输出仲裁没有解耦，当数据通过
 * crossbar后，RCInputVCState，VAOutputVCState，VAInputVCState等寄存器被释放，在inputBuffer队头的数据可以重新进行新一轮
 * 仲裁。采用high-radix-router后，寄存器释放机制必须修改。修改后，逻辑如下：
 * 1.flit在input buffer的头时，RCInputVCState保存输出端口信息，tail flit通过后，更新RCInputVCState
 * 2.RCCrossPointVCState保存CrossPointBuffer中buffer头的输出vc，tail flit通过后，重置寄存器
 *
 * high-radix-router allocation register并不会用到所有的寄存器，使用到的寄存器如下:
 * RCInputVCState, VAOutputVCState, VAOutputVCStatePre, RCCrossPointVCState
 *
 *
 */

class HighRadixRouter : public FtRouter
{
  protected:

    DataPkt* CrosspointBuffer[PortNum][PortNum][VC][CrossPointBufferDepth];
    int CrosspointBufferCredit[PortNum][PortNum][VC]; // credit-based flow control

    // allocation
    int RCCrossPointVCState[PortNum][PortNum][VC]; // CrossPoint中buffer头的vc allocation结果,值为[0,VC-1]
//    int VACrossPointOutputVCState[PortNum][VC];
//    bool VAInputVCState[PortNum][VC];

  protected:
    //handle message functions, different in routers
    virtual void initialize() override;
    virtual void handleAllocMessage(cMessage *msg) override;  // RC, VCA, SA, ST in this function
    virtual void step3VCAllocation() override;
    virtual void step4_5_SA_ST() override;  // Step 4,5 SA, ST


};

Define_Module(HighRadixRouter);

#endif /* HIGH_RADIX_ROUTER_H_ */
