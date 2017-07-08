/*
 * router.h
 *
 *  Created on: 2017年4月26日
 *      Author: Vincent
 */

#ifndef ROUTER_H_
#define ROUTER_H_

#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include <math.h>
#include "data_pkt_m.h"
#include "buffer_info_m.h"
#include "topoconfig.h"
#include "tech_power.h"
#include <time.h>
//#include <windows.h>

using namespace omnetpp;


/*************************************************
 * TODO:
 *
 *
 *
 * 1.Switch Allocator目前也是采用轮循方式，可否改进？
 *
 *
 * 2.虚通道的功能，一个Input channel有2个虚通道，其中一个仲裁胜利，但阻塞住了，另外一个应该可以通行。现在仲裁单位是package，会阻塞另外一个通道，需要改进
 * 输出端口仲裁，输入端口数据的数据以package为单位，否则会发生数据混淆。但是虚通道的仲裁不需要以package为单位，对每个虚通道设置一个寄存器
 * 用来保存该虚通道这个package的输出端口是哪个，然后虚通道仲裁用轮巡，可以防止一个虚通道阻塞时，其他虚通道可用
 *
 * 3.虚通道的问题，虚通道数量是否与输出端口数量一致，用来存放对应输出端口的数据
 *
 *
 * Feature:
 * 1. Credit-based flow control
 * 2. 数据包的Head Flit和Body Flit都是具有VCID，链路和缓存的分配以Flit为单位
 * 3. 互联资源buffer，channel，以flit为单位进行分配
 * 4. 某个packet阻塞住某个virtual channel后（可能阻塞好几个node），其他vc仍可以被利用
 * 5. 虚通道分配以packet为单位，一个packet占用一个vc（同一packet的不同flit占用一个固定的vc），（注意和buffer分配以flit为单位区分），物理信道分配以flit为单位
 * 6. 每个输出端口需要寄存器记录与它相连的下个路由器的输入端口的vc状态，记录输入buffer中lane是否空闲，有多少空闲flit buffer
 * 7. 输入端口的virtual channel需要以下几个状态：free，waiting（等待被分配），active
 * 8. 一个virtual channel只能被1个packet所占有
 * 9. 路由计算，head flit到达虚通道fifo头部才计算
 * 10. crossbar输入不耦合，输出耦合（输入vc对输出端口竞争，如果输入和输出都耦合，输入端口和输出端口需要同时仲裁）
 * 11. round robin轮巡
 * 12. 输入端口vc buffer深度为BufferDepth，输出端口vc buffer深度为1
 * 13. 发送反向流控信息，通过独立传输线
 * 14. 输出端口对所有请求该输出端口的输入vc进行仲裁（以输入vc为基本单位）
 *
 * 整个路由器仲裁的流程：
 *
 * 1. Routing Logic，路由算法决定输出端口和vc，routing function返回单个output vc（也可返回多个，但是仲裁会变得复杂）
 * 2. Virtual Channel Allocation， P个输出端口的V个Virtual Channel都有其VC Arbiter，一共PV个，仲裁器的输入为P*V，对请求同一输出端口的同一vc的不同input vc进行仲裁，选出一个胜利的input
 *  vc（共PV个input vc）
 * 3. Switch Arbitration， 两个阶段，输入端口V：1仲裁，第二阶段输出端口P：1仲裁（基于第一阶段胜利的vc），这种仲裁方式会浪费带宽资源
 * 4. 交叉开关传输
 * 注意：Head Flit有：RC，VA，SA，ST；而Body Flit只有：SA，ST
 * ***********************************************
 */


// 对Router进行建模
class Router : public cSimpleModule
{
  private:
    cMessage* selfMsgAlloc; //message仲裁定时信号

    //每个Port的buffer状态
    int BufferConnectCredit[PortNum][VC]; //连接路由器端口的buffer的credit，即空闲缓存大小

    //Step 1. Input Buffer
    //Input Buffer, Output Buffer, VC State
    DataPkt* InputBuffer[PortNum][VC][BufferDepth]; //输入端口virtual channel的buffer,里面存放收到的Flit信息
    DataPkt* OutputBuffer[PortNum][VC][OutBufferDepth]; //输出端口的virtual channel的buffer，深度为OutBufferDepth，深度用来表示router path-through latency



    //Step 2. Routing Logic
    int RCInputVCState[PortNum][VC]; //-1代表没有分配结果，即对应的vc没有数据，否则表示被分配的output vc标号，port_number * vc + vc_id
    //越早到的数据放在ID小的那边，规定好,0表示buffer中第一个出去的数据

    //Step 3. Virtual Channel Allocation
    int VAOutputVCState[PortNum][VC]; //-1代表output vc闲置，否则表示被分配的input vc标号，port_number * vc + vc_id，输出端口vc被锁住后，下一跳的输入端口对应的vc就被锁住
    int VAOutputVCStatePre[PortNum][VC]; //记录上一次仲裁胜利的输入端口虚通道标号，用于Round Robin仲裁法
    bool VAInputVCState[PortNum][VC]; //false代表VCA失败，true代表VCA成功，请求的output vc在RCInputVCState中


    //Step 4. Switch Arbitration
    int SAInputWinVcid[PortNum]; //保存胜利的vcid,仲裁失败保存-1，不需要重置
    int SAOutputWin[PortNum]; //保存胜利的input port，仲裁失败保存-1，不需要重置

    //bufferInfoMsg Queue
    cQueue bufTxQueue[PortNum]; //发送bufferInfoMsg数据队列

    double RouterPower;
    double flitReceived; //用于计算toggle rate

    //time
    clock_t t_start_h, t_end_h, t_max_h, t_start_r, t_end_r, t_max_r, t_handleMessage, t_router, t_totalTime;


  public:
    Router();
    virtual ~Router();
  protected:
    virtual void forwardMessage(DataPkt *msg, int out_port_id);
    virtual void forwardBufferInfoMsg(BufferInfoMsg *msg, int out_port_id);
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual int getPortAndVCID(DataPkt* msg);
    virtual int getNextRouterAvailVCID(int port_num); //计算下一个节点相应端口可用的virtual channel
    virtual simtime_t channelAvailTime(int port_num);
    virtual double getRouterPower();//计算路由器功耗
    // The finish() function is called by OMNeT++ at the end of the simulation:
    virtual void finish() override; //需要对router buffer中的pkt析构

    //纯虚函数，根据具体的拓扑来实现
    virtual int ppid2plid(int ppid) = 0;
    virtual int plid2ppid(int plid) = 0;
    virtual int swpid2swlid(int swpid) = 0;
    virtual int swlid2swpid(int swlid) = 0;
    virtual int calRoutePort(DataPkt* msg) = 0;
    virtual int getNextRouterPort(int current_out_port) = 0; //计算下一个相连的router的端口
    virtual bool connectToProcessor(int port_num) = 0;

};

//Define_Module(Router);

#endif /* ROUTER_H_ */
