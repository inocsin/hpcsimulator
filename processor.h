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
    cMessage *selfMsgCalcBufferOccupy;//���ͼ���buffer occupation�Ķ�ʱ�ź�

    long numFlitSent;
    long numPackageSent;
    long numFlitReceived;
    long numPackageReceived;
    long numPktDropped; //���������ݰ�
    long flitByHop; //���ڼ�����·������, flit * Hop
    bool dropFlag; //�жϱ����Ƿ�drop��

    int BufferConnectCredit[VC]; //����Processor�˿ڵ�Router��buffer��credit
    cQueue txQueue; //�������ݶ���
    double inputBufferOccupancy; // input bufferռ����
    double inputBufferEmptyTimes; // buffer���е�ʱ��
    double inputBufferFullTimes;
    double channelUnavailTimes;

//    cOutVector hopCountVector;
//    cOutVector flitDelayTime;
//    cOutVector packageDelayTime;
//    cOutVector creditMsgDelayTime;

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
    virtual void finish() override; //��Ҫ��processor buffer�е�pkt��������

    //handle message
    virtual void handleMessage(cMessage *msg) override;
    virtual void handleSendMsg();
    virtual void handleGenMsg();
    virtual void handleBufferInfoMsg(cMessage *msg);
    virtual void handleDataPkt(cMessage *msg);

    //util function
    virtual DataPkt* generateMessage(bool isHead, bool isTail, int flitCount, int flidID, int vcid);
    virtual void forwardMessage(DataPkt *msg);
    virtual void checkGenMsg(DataPkt* datapkt); //������ɵ����ݰ��Ƿ�������
    virtual simtime_t channelAvailTime();
    virtual int generateBestVCID();
    virtual void calcInputBufferOccupancy();
//    virtual double ParetoON();
//    virtual double ParetoOFF();
    virtual double Poisson();
    virtual double Uniform();

    //���麯����������ʵ��
    virtual int ppid2plid(int ppid) = 0;
    virtual int plid2ppid(int plid) = 0;
    virtual int getNextRouterPortP() = 0; //������processor������router�Ķ˿�
};

//Cannot allocate abstract class
//Define_Module(Processor);

#endif /* PROCESSOR_H_ */
