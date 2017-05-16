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

/**
 * This model is exciting enough so that we can collect some statistics.
 * We'll record in output vectors the hop count of every message upon arrival.
 * Output vectors are written into the omnetpp.vec file and can be visualized
 * with the Plove program.
 *
 * We also collect basic statistics (min, max, mean, std.dev.) and histogram
 * about the hop count which we'll print out at the end of the simulation.
 */


// Processor
class Processor : public cSimpleModule
{
  private:
    cMessage *selfMsgGenMsg;//产生flit定时，package产生按泊松分布或均匀分布
    cMessage *selfMsgSendMsg;//发送flit定时，每周期都检查buffer，再发送

    long numFlitSent;
    long numPackageSent;
    long numFlitReceived;
    long numPackageReceived;
    long numPktDropped; //丢弃的数据包
    long flitByHop; //用于计算链路利用率, flit * Hop
    bool dropFlag; //判断本轮是否drop过

    int BufferConnectCredit[VC]; //连接Processor端口的Router的buffer的credit
    cQueue txQueue; //发送数据队列

    //cLongHistogram hopCountStats;
    cOutVector hopCountVector;
    cOutVector flitDelayTime;
    cOutVector packageDelayTime;

  public:
    Processor();
    virtual ~Processor();
  protected:
    virtual DataPkt* generateMessage(bool isHead, bool isTail, int flitCount, int flidID, int vcid);
    virtual void forwardMessage(DataPkt *msg);
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void checkGenMsg(DataPkt* datapkt); //检测生成的数据包是否有问题
    virtual simtime_t channelAvailTime();
    virtual int generateBestVCID();
//    virtual double ParetoON();
//    virtual double ParetoOFF();
    virtual double Poisson();
    virtual double Uniform();
    // The finish() function is called by OMNeT++ at the end of the simulation:
    virtual void finish() override; //需要把processor buffer中的pkt给析构掉

    //纯虚函数，由子类实现
    virtual int ppid2plid(int ppid) = 0;
    virtual int plid2ppid(int plid) = 0;
    virtual int getNextRouterPortP() = 0; //计算与processor相连的router的端口
};


//Cannot allocate abstract class
//Define_Module(Processor);


#endif /* PROCESSOR_H_ */
