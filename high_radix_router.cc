/*
 * high_radix_router.cc
 *
 *  Created on: 2017年9月27日
 *      Author: Vincent
 */

#include "high_radix_router.h"
#include <assert.h>

void HighRadixRouter::initialize()
{
    FtRouter::initialize();

    for(int i = 0; i < PortNum; i++) {
        for(int j = 0; j < PortNum; j++) {
            for(int k = 0; k < VC; k++) {
                CrosspointBufferCredit[i][j][k] = CrossPointBufferDepth;
                RCCrossPointVCState[i][j][k] = -1;
                for(int l = 0; l < CrossPointBufferDepth; l++) {
                    CrosspointBuffer[i][j][k][l] = nullptr;
                }
            }
        }
    }


}

void HighRadixRouter::handleAllocMessage(cMessage *msg)
{
    //debug
    if (getIndex() == 7 && simTime().dbl() > 1.000000014871) {
        int index = getIndex();
        double rec = RecordStartTime;
        double sim = simTime().dbl();
//        double avil = channelAvailTime().dbl();
        int a = 0;

    }

    //High radix router
    //Kim J, Dally W J, Towles B, et al. Microarchitecture of a high radix router[C]
    //Computer Architecture, 2005. ISCA'05. Proceedings. 32nd International Symposium on. IEEE, 2005: 420-431.

    scheduleAt(simTime()+CLK_CYCLE, selfMsgAlloc);

    //Step 2. Routing Logic
    //计算每个packet的输出端口及输出vcid
    step2RoutingLogic();

    //开始对部分模块进行计时
    t_start_r = clock();

    //Step 3,4,5 SA && VCA (this two stages are parallel in high-radix router)
    //Step 3.1 VCA
    step3VCAllocation();

    //Step 4,5 SA and ST
    step4_5_SA_ST();


    //结束部分模块的计时
    t_end_r = clock();
    if (simTime().dbl() > RecordStartTime) {
        if(t_end_r - t_start_r > t_max_r) {
            t_max_r = t_end_r - t_start_r;
        }
        t_router += t_end_r - t_start_r;
    }

    //Step 6. Forward Data Message
    step6ForwardDataMsg();

    //Step 7. Forward bufferInfoMsg Message
    step7ForwardBufferInfoMsg();

}


void HighRadixRouter::step3VCAllocation()
{
    // Step 3.1 output vc computation
    for(int i = 0; i < PortNum; i++) { // input port
        for(int j = 0; j < PortNum; j++) { // output port
            for(int k = 0; k < VC; k++) { // vc
                DataPkt* current_pkt = CrosspointBuffer[i][j][k][0];
                if(current_pkt != nullptr && current_pkt->getIsHead() == true && RCCrossPointVCState[i][j][k] == -1) {
                    int port_vc = getPortAndVCID(current_pkt);
                    RCCrossPointVCState[i][j][k] = port_vc % VC;
                    assert(j == (port_vc / VC));
                    if (Verbose >= VERBOSE_DEBUG_MESSAGES) {
                        EV<<"Step 3.1 VCAllocation >> ROUTER: "<<getIndex()<<"("<<swpid2swlid(getIndex())<<"), INPORT: "<<i<<
                                ", OUTPUT PORT: "<<j<<", VC: "<<k<<
                                ", ALLOCATION VC: "<< port_vc % VC <<", Msg: { "<<current_pkt<<" }\n";
                    }
                }
            }
        }
    }

    // Step 3.2 output vc allocation
    //每一个输出端口的每一个virtual channel要对每个输入端口的每个virtual channel进行仲裁，只有一个胜利
    //外循环，对每个output virtual channel进行循环
    for(int i = 0; i < PortNum; i++) { // output port
        int outport = i;
        for(int j = 0; j < VC; j++) {
            int out_vc = j;
            //内循环，对每个input virtual channel进行判断
            //注意：加了OutputBuffer后，tail flit数据送到outputBuffer后一定要释放VAOutputVCState，输出虚通道状态寄存器要释放
            if(VAOutputVCState[i][j] >= 0) continue; //若输出端口vc已被分配，则跳过下面循环
            bool flag = false;
            int port_vc = VAOutputVCStatePre[i][j] + 1;
            int count = 0, total = PortNum * VC;
            for(; count < total && flag == false; count++, port_vc++) {
                int inport = (port_vc / VC) % PortNum;
                int in_vc = port_vc % VC;
                if(RCCrossPointVCState[inport][outport][in_vc] == out_vc) { //上面已经保证仲裁的输出端口为outport
                    VAOutputVCStatePre[i][j] = inport * VC + in_vc;
                    VAOutputVCState[i][j] = inport * VC + in_vc;
                    flag = true;
                    if (Verbose >= VERBOSE_DEBUG_MESSAGES) {
                        EV<<"Step 3.2 VC Allocation >> ROUTER: "<<getIndex()<<"("<<swpid2swlid(getIndex())<<"), WIN INPORT: "<<inport<<
                                ", WIN INPORT VCID: "<<in_vc<<", OUTPORT: "<<i<<
                                ", OUTPORT VCID: "<<j<<"\n";
                    }
                }
            }
        }
    }
}

void HighRadixRouter::step4_5_SA_ST()
{
    //Step 4.1 SA input port switch allocation and traversal
    for(int i = 0; i < PortNum; i++) { // for each input port
        int vcid = intuniform(0,VC-1); // random allocation start point
        for(int j = 0; j < VC; j++) { // input port SA
            int current_vcid = (vcid + j) % VC;
            DataPkt* current_pkt = InputBuffer[i][current_vcid][0];
            if(current_pkt != nullptr) {
                assert(RCInputVCState[i][current_vcid] != -1);
                //RCInputVCState保存了outport和out vcid，但是这里out vcid不用，step3VCAllocation里面的out vcid才有用
                int outport = RCInputVCState[i][current_vcid] / VC;
                if(CrosspointBufferCredit[i][outport][current_vcid] > 0) {
                    int credit = CrosspointBufferCredit[i][outport][current_vcid];
                    --CrosspointBufferCredit[i][outport][current_vcid];
                    assert(CrosspointBuffer[i][outport][current_vcid][CrossPointBufferDepth-credit] == nullptr);
                    CrosspointBuffer[i][outport][current_vcid][CrossPointBufferDepth-credit] = current_pkt;
                    for(int k = 0; k < BufferDepth-1; k++) {
                        InputBuffer[i][current_vcid][k] = InputBuffer[i][current_vcid][k+1];
                    }
                    InputBuffer[i][current_vcid][BufferDepth-1] = nullptr;

                    if (Verbose >= VERBOSE_DEBUG_MESSAGES) {
                        EV<<"Step 4.1 Switch Allocation, Input Port Stage >> ROUTER: "<<getIndex()<<"("<<swpid2swlid(getIndex())<<"), INPORT: "<<i<<
                                ", WIN VCID: "<<current_vcid<< ", OUTPORT: " << outport <<
                                ", Win Msg: { " << current_pkt << " }\n";
                    }

                    // 释放寄存器
                    if(current_pkt->getIsTail()) {
                        RCInputVCState[i][current_vcid] = -1;
                    }

                    //每转发input buffer里面的一个flit，就产生一个流控信号，通知上游router，进行increment credit操作
                    //先将bufferInfoMsg放入Queue中，由于和Data Pkt共用一个信道，容易发生阻塞，需要队列保存数据
                    int from_port = getNextRouterPort(i);
                    BufferInfoMsg* bufferInfoMsg = new BufferInfoMsg("bufferInfoMsg");
                    bufferInfoMsg->setFrom_port(from_port);
                    bufferInfoMsg->setVcid(current_vcid);
                    bufTxQueue[i].insert(bufferInfoMsg);
                    break; // break input port SA, cause only one vc take the physical channel

                }
            }
        }
    }

    //Step 4.2 SA output port switch allocation and traversal
    for(int i = 0; i < PortNum; i++) { // for each output port
        int outport = i;
        int port_vcid_start = intuniform(0, PortNum * VC -1);  //randomly pick up a vc channel in random port
        for(int j = 0; j < PortNum * VC; j++) {
            int port_vcid = j + port_vcid_start;
            int inport = (port_vcid / VC) % PortNum;
            int in_vc = port_vcid % VC;
            if(CrosspointBuffer[inport][i][in_vc][0] != nullptr &&
               RCCrossPointVCState[inport][i][in_vc] != -1) {
                // this out_vc is allocated due to RCInputVCState != -1
                int out_vc = RCCrossPointVCState[inport][i][in_vc];
                assert(VAOutputVCState[outport][out_vc] == inport * VC + in_vc);
                if(OutputBuffer[i][out_vc][OutBufferDepth-1] == nullptr) { // buffer available
                    DataPkt* current_pkt = CrosspointBuffer[inport][outport][in_vc][0];
                    OutputBuffer[i][out_vc][OutBufferDepth-1] = current_pkt;
                    for(int k = 0; k < CrossPointBufferDepth-1; k++) {
                        CrosspointBuffer[inport][outport][in_vc][k] = CrosspointBuffer[inport][outport][in_vc][k+1];
                    }
                    CrosspointBuffer[inport][outport][in_vc][CrossPointBufferDepth-1] = nullptr;
                    CrosspointBufferCredit[inport][outport][in_vc]++;

                    current_pkt->setVc_id(out_vc);
                    // if is head tail, release aloacation states
                    // there is bug
                    if(current_pkt->getIsTail()) {
                        RCCrossPointVCState[inport][outport][in_vc] = -1;
                        VAOutputVCState[outport][out_vc] = -1;
                    }
                    if (Verbose >= VERBOSE_DEBUG_MESSAGES) {
                        EV<<"Step 4.2 Switch Allocation, Output Port Stage >> ROUTER: "<<getIndex()<<"("<<swpid2swlid(getIndex())<<"), OUTPORT: "<<i<<
                                ", WIN INPORT: "<<inport<<", WIN INPORT VC: " << in_vc<< ", OUT VC: " << out_vc << " }\n";
                    }
                    break; // break this outport alloacation and traversal
                }
            }

        }

    }  // end of step 4.2
}
