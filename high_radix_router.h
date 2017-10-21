/*
 * high_radix_router.h
 *
 *  Created on: 2017��9��27��
 *      Author: Vincent
 */

#ifndef HIGH_RADIX_ROUTER_H_
#define HIGH_RADIX_ROUTER_H_

#include "ft_router.h"

using namespace omnetpp;
/*
 * 17/10/9 �Ų�����������ܽ������⣬��Ҫ����Ϊ������high-radix-router����·�������ʼ����½����ҵ���ĳ��ʱ����
 * ���ֽڵ㲻�ٽ��պͷ����µ����ݡ��Ų鷢�ּĴ������ͷ�ʱ��������⡣ԭ�ȵ�·���������ٲú�����ٲ�û�н��������ͨ��
 * crossbar��RCInputVCState��VAOutputVCState��VAInputVCState�ȼĴ������ͷţ���inputBuffer��ͷ�����ݿ������½�����һ��
 * �ٲá�����high-radix-router�󣬼Ĵ����ͷŻ��Ʊ����޸ġ��޸ĺ��߼����£�
 * 1.flit��input buffer��ͷʱ��RCInputVCState��������˿���Ϣ��tail flitͨ���󣬸���RCInputVCState
 * 2.RCCrossPointVCState����CrossPointBuffer��bufferͷ�����vc��tail flitͨ�������üĴ���
 *
 * high-radix-router allocation register�������õ����еļĴ�����ʹ�õ��ļĴ�������:
 * RCInputVCState, VAOutputVCState, VAOutputVCStatePre, RCCrossPointVCState
 *
 * RCCrossPointVCState������VCA�׶Σ����������output vc����������ռ�и�output vc�����ܶ��inport vc���pkt�������䵽��
 * ͬһ�����outport vc��
 * VACrossPointVCState����crosspoint��vc��vca�ٲõĽ����true��ʾ��vc�ɹ����output vc
 *
 * High radix router
 * Kim J, Dally W J, Towles B, et al. Microarchitecture of a high radix router[C]
 * Computer Architecture, 2005. ISCA'05. Proceedings. 32nd International Symposium on. IEEE, 2005: 420-431.
 *
 *
 */

class HighRadixRouter : public FtRouter
{
  protected:

    DataPkt* CrosspointBuffer[PortNum][PortNum][VC][CrossPointBufferDepth];
    int CrosspointBufferCredit[PortNum][PortNum][VC]; // credit-based flow control

    // allocation
    int RCCrossPointVCState[PortNum][PortNum][VC]; // CrossPoint��bufferͷ��vc allocation���,������output vc�����ֵΪ[0,VC-1]
    bool VACrossPointVCState[PortNum][PortNum][VC]; // true�����vc�ٲ�ʤ��
//    int VACrossPointOutputVCState[PortNum][VC];
//    bool VAInputVCState[PortNum][VC];

  protected:
    //handle message functions, different in routers
    virtual void initialize() override;
//    virtual void handleAllocMessage(cMessage *msg) override;  // RC, VCA, SA, ST in this function
    virtual void step3VCAllocation() override;
    virtual void step4_5_SA_ST() override;  // Step 4,5 SA, ST


};

Define_Module(HighRadixRouter);

#endif /* HIGH_RADIX_ROUTER_H_ */
