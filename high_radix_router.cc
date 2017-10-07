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
                for(int l = 0; l < CrossPointBufferDepth; l++) {
                    CrosspointBuffer[i][j][k][l] = nullptr;
                }
            }
        }
    }


}

void HighRadixRouter::handleAllocMessage(cMessage *msg)
{
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

    //Step 4.1 SA input port switch allocation and traversal
    for(int i = 0; i < PortNum; i++) { // for each input port
        int vcid = intuniform(0,VC-1); // random allocation start point
        for(int j = 0; j < VC; j++) { // input port SA
            int current_vcid = (vcid + j) % VC;
            DataPkt* current_pkt = InputBuffer[i][current_vcid][0];
            if(current_pkt != nullptr) {
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
        int port_vcid_start = intuniform(0, PortNum * VC -1);  //randomly pick up a vc channel in random port
        for(int j = 0; j < PortNum * VC; j++) {
            int port_vcid = j + port_vcid_start;
            int inport = (port_vcid / VC) % PortNum;
            int in_vc = port_vcid % VC;
            if(CrosspointBuffer[inport][i][in_vc][0] != nullptr &&
               VAInputVCState[inport][in_vc] == true) {
                assert(RCInputVCState[inport][in_vc] != -1);
                int outport = RCInputVCState[inport][in_vc] / VC;
                // this out_vc is allocated due to RCInputVCState != -1
                int out_vc = RCInputVCState[inport][in_vc] % VC;
                assert(outport == i);
                // given VAInputVCState[inport][in_vc] == true
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
                    if(current_pkt->getIsTail()) {
                        RCInputVCState[inport][in_vc] = -1;
                        VAInputVCState[inport][in_vc] = false;
                        VAOutputVCState[outport][out_vc] = -1;
                    }
                    break; // break this outport alloacation and traversal
                }
            }

        }

    }  // end of step 3.3

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
