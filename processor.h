/*
 * processor.h
 *
 *  Created on: 2017��4��26��
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
    cMessage *selfMsgGenMsg;//����flit��ʱ��package���������ɷֲ�����ȷֲ�
    cMessage *selfMsgSendMsg;//����flit��ʱ��ÿ���ڶ����buffer���ٷ���

    long numFlitSent;
    long numPackageSent;
    long numFlitReceived;
    long numPackageReceived;
    long numPktDropped; //���������ݰ�
    long flitByHop; //���ڼ�����·������, flit * Hop
    bool dropFlag; //�жϱ����Ƿ�drop��

    int BufferConnectCredit[VC]; //����Processor�˿ڵ�Router��buffer��credit
    cQueue txQueue; //�������ݶ���

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
    virtual void checkGenMsg(DataPkt* datapkt); //������ɵ����ݰ��Ƿ�������
    virtual simtime_t channelAvailTime();
    virtual int generateBestVCID();
//    virtual double ParetoON();
//    virtual double ParetoOFF();
    virtual double Poisson();
    virtual double Uniform();
    // The finish() function is called by OMNeT++ at the end of the simulation:
    virtual void finish() override; //��Ҫ��processor buffer�е�pkt��������

    //���麯����������ʵ��
    virtual int ppid2plid(int ppid) = 0;
    virtual int plid2ppid(int plid) = 0;
    virtual int getNextRouterPortP() = 0; //������processor������router�Ķ˿�
};


//Cannot allocate abstract class
//Define_Module(Processor);


#endif /* PROCESSOR_H_ */
