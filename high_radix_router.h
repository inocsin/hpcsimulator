/*
 * high_radix_router.h
 *
 *  Created on: 2017年9月27日
 *      Author: Vincent
 */

#ifndef HIGH_RADIX_ROUTER_H_
#define HIGH_RADIX_ROUTER_H_

#include "fat_tree.h"

using namespace omnetpp;

class HighRadixRouter : public FtRouter
{
  protected:
//    cMessage* selfMsgAlloc; //message仲裁定时信号
//
//    //每个Port的buffer状态
//    int BufferConnectCredit[PortNum][VC]; //连接路由器端口的buffer的credit，即空闲缓存大小
//
//    //Step 1. Input Buffer
//    //Input Buffer, Output Buffer, VC State
//    DataPkt* InputBuffer[PortNum][VC][BufferDepth]; //输入端口virtual channel的buffer,里面存放收到的Flit信息
//    DataPkt* OutputBuffer[PortNum][VC][OutBufferDepth]; //输出端口的virtual channel的buffer，深度为OutBufferDepth，深度用来表示router path-through latency
//
//
//
//    //Step 2. Routing Logic
//    int RCInputVCState[PortNum][VC]; //-1代表没有分配结果，即对应的vc没有数据，否则表示被分配的output vc标号，port_number * vc + vc_id
//    //越早到的数据放在ID小的那边，规定好,0表示buffer中第一个出去的数据
//
//    //Step 3. Virtual Channel Allocation
//    int VAOutputVCState[PortNum][VC]; //-1代表output vc闲置，否则表示被分配的input vc标号，port_number * vc + vc_id，输出端口vc被锁住后，下一跳的输入端口对应的vc就被锁住
//    int VAOutputVCStatePre[PortNum][VC]; //记录上一次仲裁胜利的输入端口虚通道标号，用于Round Robin仲裁法
//    bool VAInputVCState[PortNum][VC]; //false代表VCA失败，true代表VCA成功，请求的output vc在RCInputVCState中
//
//
//    //Step 4. Switch Arbitration
//    int SAInputWinVcid[PortNum]; //保存胜利的vcid,仲裁失败保存-1，不需要重置
//    int SAOutputWin[PortNum]; //保存胜利的input port，仲裁失败保存-1，不需要重置
//
//    //bufferInfoMsg Queue
//    cQueue bufTxQueue[PortNum]; //发送bufferInfoMsg数据队列
//
//    double RouterPower;
//    double flitReceived; //用于计算toggle rate
//
//    //time
//    clock_t t_start_h, t_end_h, t_max_h, t_start_r, t_end_r, t_max_r, t_handleMessage, t_router, t_totalTime;

    DataPkt* CrosspointBuffer[PortNum][PortNum][VC][CrossPointBufferDepth];


  public:
//    HighRadixRouter();
//    virtual ~HighRadixRouter();
  protected:
    //handle message functions, different in routers
    virtual void initialize() override;
//    virtual void handleMessage(cMessage *msg) override; // main handle message
    virtual void handleAllocMessage(cMessage *msg) override;  // RC, VCA, SA, ST in this function
//    virtual void handleBufferInfoMessage(cMessage *msg); // credit-based flow control
//    virtual void handleInputDataPkt(cMessage *msg); // input stage

    //utility functions
//    virtual void forwardMessage(DataPkt *msg, int out_port_id);
//    virtual void forwardBufferInfoMsg(BufferInfoMsg *msg, int out_port_id);
//    virtual int getPortAndVCID(DataPkt* msg);
//    virtual simtime_t channelAvailTime(int port_num);
//    virtual double getRouterPower();//计算路由器功耗
//    // The finish() function is called by OMNeT++ at the end of the simulation:
//    virtual void finish() override; //需要对router buffer中的pkt析构

    //纯虚函数，根据具体的拓扑来实现
//    virtual int ppid2plid(int ppid) = 0;
//    virtual int plid2ppid(int plid) = 0;
//    virtual int swpid2swlid(int swpid) = 0;
//    virtual int swlid2swpid(int swlid) = 0;
//    virtual int calRoutePort(DataPkt* msg) = 0;
//    virtual int getNextRouterPort(int current_out_port) = 0; //计算下一个相连的router的端口
//    virtual bool connectToProcessor(int port_num) = 0;

};

Define_Module(HighRadixRouter);

#endif /* HIGH_RADIX_ROUTER_H_ */
