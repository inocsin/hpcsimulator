/*
 * router.h
 *
 *  Created on: 2017��4��26��
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
 * 1.Switch AllocatorĿǰҲ�ǲ�����ѭ��ʽ���ɷ�Ľ���
 *
 *
 * 2.��ͨ���Ĺ��ܣ�һ��Input channel��2����ͨ��������һ���ٲ�ʤ����������ס�ˣ�����һ��Ӧ�ÿ���ͨ�С������ٲõ�λ��package������������һ��ͨ������Ҫ�Ľ�
 * ����˿��ٲã�����˿����ݵ�������packageΪ��λ������ᷢ�����ݻ�����������ͨ�����ٲò���Ҫ��packageΪ��λ����ÿ����ͨ������һ���Ĵ���
 * �����������ͨ�����package������˿����ĸ���Ȼ����ͨ���ٲ�����Ѳ�����Է�ֹһ����ͨ������ʱ��������ͨ������
 *
 * 3.��ͨ�������⣬��ͨ�������Ƿ�������˿�����һ�£�������Ŷ�Ӧ����˿ڵ�����
 *
 *
 * Feature:
 * 1. Credit-based flow control
 * 2. ���ݰ���Head Flit��Body Flit���Ǿ���VCID����·�ͻ���ķ�����FlitΪ��λ
 * 3. ������Դbuffer��channel����flitΪ��λ���з���
 * 4. ĳ��packet����סĳ��virtual channel�󣨿��������ü���node��������vc�Կ��Ա�����
 * 5. ��ͨ��������packetΪ��λ��һ��packetռ��һ��vc��ͬһpacket�Ĳ�ͬflitռ��һ���̶���vc������ע���buffer������flitΪ��λ���֣��������ŵ�������flitΪ��λ
 * 6. ÿ������˿���Ҫ�Ĵ�����¼�����������¸�·����������˿ڵ�vc״̬����¼����buffer��lane�Ƿ���У��ж��ٿ���flit buffer
 * 7. ����˿ڵ�virtual channel��Ҫ���¼���״̬��free��waiting���ȴ������䣩��active
 * 8. һ��virtual channelֻ�ܱ�1��packet��ռ��
 * 9. ·�ɼ��㣬head flit������ͨ��fifoͷ���ż���
 * 10. crossbar���벻��ϣ������ϣ�����vc������˿ھ��������������������ϣ�����˿ں�����˿���Ҫͬʱ�ٲã�
 * 11. round robin��Ѳ
 * 12. ����˿�vc buffer���ΪBufferDepth������˿�vc buffer���Ϊ1
 * 13. ���ͷ���������Ϣ��ͨ������������
 * 14. ����˿ڶ��������������˿ڵ�����vc�����ٲã�������vcΪ������λ��
 *
 * ����·�����ٲõ����̣�
 *
 * 1. Routing Logic��·���㷨��������˿ں�vc��routing function���ص���output vc��Ҳ�ɷ��ض���������ٲû��ø��ӣ�
 * 2. Virtual Channel Allocation�� P������˿ڵ�V��Virtual Channel������VC Arbiter��һ��PV�����ٲ���������ΪP*V��������ͬһ����˿ڵ�ͬһvc�Ĳ�ͬinput vc�����ٲã�ѡ��һ��ʤ����input
 *  vc����PV��input vc��
 * 3. Switch Arbitration�� �����׶Σ�����˿�V��1�ٲã��ڶ��׶�����˿�P��1�ٲã����ڵ�һ�׶�ʤ����vc���������ٲ÷�ʽ���˷Ѵ�����Դ
 * 4. ���濪�ش���
 * ע�⣺Head Flit�У�RC��VA��SA��ST����Body Flitֻ�У�SA��ST
 * ***********************************************
 */


// ��Router���н�ģ
class Router : public cSimpleModule
{
  private:
    cMessage* selfMsgAlloc; //message�ٲö�ʱ�ź�

    //ÿ��Port��buffer״̬
    int BufferConnectCredit[PortNum][VC]; //����·�����˿ڵ�buffer��credit�������л����С

    //Step 1. Input Buffer
    //Input Buffer, Output Buffer, VC State
    DataPkt* InputBuffer[PortNum][VC][BufferDepth]; //����˿�virtual channel��buffer,�������յ���Flit��Ϣ
    DataPkt* OutputBuffer[PortNum][VC][OutBufferDepth]; //����˿ڵ�virtual channel��buffer�����ΪOutBufferDepth�����������ʾrouter path-through latency



    //Step 2. Routing Logic
    int RCInputVCState[PortNum][VC]; //-1����û�з�����������Ӧ��vcû�����ݣ������ʾ�������output vc��ţ�port_number * vc + vc_id
    //Խ�絽�����ݷ���IDС���Ǳߣ��涨��,0��ʾbuffer�е�һ����ȥ������

    //Step 3. Virtual Channel Allocation
    int VAOutputVCState[PortNum][VC]; //-1����output vc���ã������ʾ�������input vc��ţ�port_number * vc + vc_id������˿�vc����ס����һ��������˿ڶ�Ӧ��vc�ͱ���ס
    int VAOutputVCStatePre[PortNum][VC]; //��¼��һ���ٲ�ʤ��������˿���ͨ����ţ�����Round Robin�ٲ÷�
    bool VAInputVCState[PortNum][VC]; //false����VCAʧ�ܣ�true����VCA�ɹ��������output vc��RCInputVCState��


    //Step 4. Switch Arbitration
    int SAInputWinVcid[PortNum]; //����ʤ����vcid,�ٲ�ʧ�ܱ���-1������Ҫ����
    int SAOutputWin[PortNum]; //����ʤ����input port���ٲ�ʧ�ܱ���-1������Ҫ����

    //bufferInfoMsg Queue
    cQueue bufTxQueue[PortNum]; //����bufferInfoMsg���ݶ���

    double RouterPower;
    double flitReceived; //���ڼ���toggle rate

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

//Define_Module(Router);

#endif /* ROUTER_H_ */
