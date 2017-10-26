/*
 * processor.h
 *
 *  Created on: 2017年4月26日
 *      Author: Vincent
 */

#ifndef PROCESSOR_H_
#define PROCESSOR_H_

#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include <algorithm>
#include "data_pkt_m.h"
#include "buffer_info_m.h"
#include "topoconfig.h"

using namespace omnetpp;


enum TRAFFIC {
    UNI = 0,
    HOTSPOT = 1,
    TRANSPOSE = 2,
    COMPLEMENT = 3,
    BITREVERSAL = 4
};

// Processor
class Processor : public cSimpleModule
{
  private:
    cMessage *selfMsgGenMsg;//产生flit定时，package产生按泊松分布或均匀分布
    cMessage *selfMsgSendMsg;//发送flit定时，每周期都检查buffer，再发送
    cMessage *selfMsgCalcBufferOccupy;//发送计算buffer occupation的定时信号


    bool dropFlag; //判断本轮是否drop过
    int BufferConnectCredit[VC]; //连接Processor端口的Router的buffer的credit
    cQueue txQueue; //发送数据队列
    enum TRAFFIC traffic = TRAFFIC(Traffic);
    int hotspotIndex;  //hotspot对应的热点

    //buffer scalar
    double inputBufferOccupancy; // input buffer占有率
    double inputBufferEmptyTimes; // buffer空闲的时段
    double inputBufferFullTimes;
    double channelUnavailTimes;

    //network info variable
    long numFlitSent;
    long numPackageSent;
    long numFlitReceived;
    long numPackageReceived;
    long numPktDropped; //丢弃的数据包
    long flitByHop; //用于计算链路利用率, flit * Hop
    long hopCountTotal;
    int hopCountCount;
    double flitDelayTimeTotal;
    int flitDelayTimeCount;
    double packetDelayTimeTotal;
    int packetDelayTimeCount;
    double creditMsgDelayTimeTotal;
    int creditMsgDelayTimeCount;

  public:
    Processor();
    virtual ~Processor();
  protected:

    virtual void initialize() override;

    // The finish() function is called by OMNeT++ at the end of the simulation:
    virtual void finish() override; //需要把processor buffer中的pkt给析构掉

    //handle message
    virtual void handleMessage(cMessage *msg) override;
    virtual void handleSendMsg();
    virtual void handleGenMsg();
    virtual void handleBufferInfoMsg(cMessage *msg);
    virtual void handleDataPkt(cMessage *msg);

    //util function
    virtual DataPkt* generateMessage(bool isHead, bool isTail, int flitCount, int flidID, int vcid);
    virtual void forwardMessage(DataPkt *msg);
    virtual void checkGenMsg(DataPkt* datapkt); //检测生成的数据包是否有问题
    virtual simtime_t channelAvailTime();
    virtual int generateBestVCID();
    virtual void calcInputBufferOccupancy();
//    virtual double ParetoON();
//    virtual double ParetoOFF();
    virtual double Poisson();
    virtual double Uniform();

    //纯虚函数，由子类实现
    virtual int ppid2plid(int ppid) = 0;
    virtual int plid2ppid(int plid) = 0;
    virtual int getNextRouterPortP() = 0; //计算与processor相连的router的端口

    //traffic
    int uni();
    int hotspot();
    int transpose();
    int complement();
    int bitreversal();

};

//Cannot allocate abstract class
//Define_Module(Processor);

#endif /* PROCESSOR_H_ */
