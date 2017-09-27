/*
 * high_radix_router.h
 *
 *  Created on: 2017��9��27��
 *      Author: Vincent
 */

#ifndef HIGH_RADIX_ROUTER_H_
#define HIGH_RADIX_ROUTER_H_

#include "router.h"

using namespace omnetpp;

class HighRadixRouter : public Router
{
  private:
//    cMessage* selfMsgAlloc; //message�ٲö�ʱ�ź�
//
//    //ÿ��Port��buffer״̬
//    int BufferConnectCredit[PortNum][VC]; //����·�����˿ڵ�buffer��credit�������л����С
//
//    //Step 1. Input Buffer
//    //Input Buffer, Output Buffer, VC State
//    DataPkt* InputBuffer[PortNum][VC][BufferDepth]; //����˿�virtual channel��buffer,�������յ���Flit��Ϣ
//    DataPkt* OutputBuffer[PortNum][VC][OutBufferDepth]; //����˿ڵ�virtual channel��buffer�����ΪOutBufferDepth�����������ʾrouter path-through latency
//
//
//
//    //Step 2. Routing Logic
//    int RCInputVCState[PortNum][VC]; //-1����û�з�����������Ӧ��vcû�����ݣ������ʾ�������output vc��ţ�port_number * vc + vc_id
//    //Խ�絽�����ݷ���IDС���Ǳߣ��涨��,0��ʾbuffer�е�һ����ȥ������
//
//    //Step 3. Virtual Channel Allocation
//    int VAOutputVCState[PortNum][VC]; //-1����output vc���ã������ʾ�������input vc��ţ�port_number * vc + vc_id������˿�vc����ס����һ��������˿ڶ�Ӧ��vc�ͱ���ס
//    int VAOutputVCStatePre[PortNum][VC]; //��¼��һ���ٲ�ʤ��������˿���ͨ����ţ�����Round Robin�ٲ÷�
//    bool VAInputVCState[PortNum][VC]; //false����VCAʧ�ܣ�true����VCA�ɹ��������output vc��RCInputVCState��
//
//
//    //Step 4. Switch Arbitration
//    int SAInputWinVcid[PortNum]; //����ʤ����vcid,�ٲ�ʧ�ܱ���-1������Ҫ����
//    int SAOutputWin[PortNum]; //����ʤ����input port���ٲ�ʧ�ܱ���-1������Ҫ����
//
//    //bufferInfoMsg Queue
//    cQueue bufTxQueue[PortNum]; //����bufferInfoMsg���ݶ���
//
//    double RouterPower;
//    double flitReceived; //���ڼ���toggle rate
//
//    //time
//    clock_t t_start_h, t_end_h, t_max_h, t_start_r, t_end_r, t_max_r, t_handleMessage, t_router, t_totalTime;

    DataPkt* CrosspointBuffer[PortNum][PortNum][VC]


  public:
    HighRadixRouter();
    virtual ~HighRadixRouter();
  protected:
    virtual void forwardMessage(DataPkt *msg, int out_port_id);
    virtual void forwardBufferInfoMsg(BufferInfoMsg *msg, int out_port_id);
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual int getPortAndVCID(DataPkt* msg);
    virtual int getNextRouterAvailVCID(int port_num); //������һ���ڵ���Ӧ�˿ڿ��õ�virtual channel
    virtual simtime_t channelAvailTime(int port_num);
    virtual double getRouterPower();//����·��������
    // The finish() function is called by OMNeT++ at the end of the simulation:
    virtual void finish() override; //��Ҫ��router buffer�е�pkt����

    //���麯�������ݾ����������ʵ��
    virtual int ppid2plid(int ppid) = 0;
    virtual int plid2ppid(int plid) = 0;
    virtual int swpid2swlid(int swpid) = 0;
    virtual int swlid2swpid(int swlid) = 0;
    virtual int calRoutePort(DataPkt* msg) = 0;
    virtual int getNextRouterPort(int current_out_port) = 0; //������һ��������router�Ķ˿�
    virtual bool connectToProcessor(int port_num) = 0;

};

#endif /* HIGH_RADIX_ROUTER_H_ */
