/*
 * high_radix_router.cc
 *
 *  Created on: 2017年9月27日
 *      Author: Vincent
 */

#include "high_radix_router.h"

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


    //Step 3. SA && VCA (this two stages are parallel in
    //Step 3.1 SA input port switch allocation
    for(int i = 0; i < PortNum; i++) { // for each input port
        int vcid = intuniform(0,VC-1);
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
                        InputBuffer[i][current_vcid][k] = InputBuffer[i][current_vcid][k+1]
                    }
                    InputBuffer[i][current_vcid][BufferDepth-1] = nullptr;
                    break; // break input port SA, cause only one vc take the physical channel
                }
            }
        }
    }

    //Step 3.2 Virtual channel allocation














    //******************************old
    //Step 3. Virtual Channel Allocation
    //每一个输出端口的每一个virtual channel要对每个输入端口的每个virtual channel进行仲裁，只有一个胜利
    //外循环，对每个output virtual channel进行循环
    for(int i = 0; i < PortNum; i++) { // each output port
        for(int j = 0; j < VC; j++) { // each virtual channel
            //内循环，对每个input virtual channel进行判断
            //注意：加了OutputBuffer后，tail flit数据送到outputBuffer后一定要释放VAOutputVCState，输出虚通道状态寄存器要释放
            if(VAOutputVCState[i][j] >= 0) continue; //若输出端口vc已被分配，则跳过下面循环
            bool flag = false;
            int port_vc = VAOutputVCStatePre[i][j] + 1;
            int count = 0, total = PortNum * VC;
            for(; count < total && flag == false; count++, port_vc++) {
                int m = (port_vc / VC) % PortNum;
                int n = port_vc % VC;
                if(RCInputVCState[m][n] == i * VC + j) {//该输入虚通道里面的数据一定是head flit, 否则上面if语句会跳过
                    if(VAInputVCState[m][n] == true) {
                        EV << "Error: Step 3. VC Allocation >> ROUTER: " << getIndex()<<"("<<swpid2swlid(getIndex()) << "), VAInputVCState != false" << endl;
                    }
                    VAInputVCState[m][n] = true; //该虚通道仲裁胜利
                    VAOutputVCStatePre[i][j] = m * VC + n;
                    VAOutputVCState[i][j] = m * VC + n;
                    flag = true;
                    if (Verbose >= VERBOSE_DEBUG_MESSAGES) {
                        EV<<"Step 3. VC Allocation >> ROUTER: "<<getIndex()<<"("<<swpid2swlid(getIndex())<<"), WIN INPORT: "<<m<<
                                ", WIN INPORT VCID: "<<n<<", OUTPORT: "<<i<<
                                ", OUTPORT VCID: "<<j<<", Win Msg: { "<<InputBuffer[m][n][0]<<" }\n";
                    }
                }
            }
        }
    }

    //Step 4. Switch Arbitration
    //在VAInputVCState中为true的输入vc中做选择，而且其对应的输出端口的vc的buffer必须为空，能够容纳它
    //4.1. 输入端口仲裁
    for(int i = 0; i < PortNum; i++) {
        int last_vcid = SAInputWinVcid[i] + 1;
        bool flag = false;
        for(int j = 0; j < VC && flag == false; j++, last_vcid++) {
            int cur_vcid = last_vcid % VC; //Round Robin
            //必须保证输入虚通道在虚通道仲裁中胜利（即输出虚通道为此输入虚通道保留）
            //此外还需要保证输入buffer有数据，可能head flit传输胜利，但是body flit还没过来
            if(VAInputVCState[i][cur_vcid] == true && InputBuffer[i][cur_vcid][0] != nullptr) { //之前写成InputBuffer[i][cur_vcid] != nullptr，查了3天，泪崩，需要判断输入buffer是否有数据，可能堵在前一跳路由器
                int port = RCInputVCState[i][cur_vcid] / VC;
                int out_vcid = RCInputVCState[i][cur_vcid] % VC;
                if(OutputBuffer[port][out_vcid][OutBufferDepth-1] == nullptr) { //判断输出buffer有空间
                    SAInputWinVcid[i] = cur_vcid;
                    flag = true;
                    if (Verbose >= VERBOSE_DEBUG_MESSAGES) {
                        EV<<"Step 4.1 Switch Allocation, Input Port Stage >> ROUTER: "<<getIndex()<<"("<<swpid2swlid(getIndex())<<"), INPORT: "<<i<<
                                ", WIN VCID: "<<cur_vcid<<
                                ", Win Msg: { "<<InputBuffer[i][cur_vcid][0]<<" }\n";
                    }
                }
            }
        }
        if(flag == false) {
            SAInputWinVcid[i] = -1; //没有合适的虚通道
        }
    }
    //4.2. 输出端口仲裁
    for(int i = 0; i < PortNum; i++) {
        //内循环，对每一个输入端口判断是否请求该输出端口
        int last_inport = SAOutputWin[i] + 1; //Round Robin
        bool flag = false;
        for(int j = 0; j < PortNum && flag == false; j++, last_inport++) {
            int cur_inport = last_inport % PortNum;
            //SAInputWinVcid[cur_inport] != -1保证输入有数据，输出有buffer
            if(SAInputWinVcid[cur_inport] != -1) {
                int inport_vcid = SAInputWinVcid[cur_inport];
                int out_port = RCInputVCState[cur_inport][inport_vcid] / VC;
                if(out_port == i) {
                    SAOutputWin[i] = cur_inport;
                    flag = true;
                    if (Verbose >= VERBOSE_DEBUG_MESSAGES) {
                        EV<<"Step 4.2 Switch Allocation, Output Port Stage >> ROUTER: "<<getIndex()<<"("<<swpid2swlid(getIndex())<<"), OUTPORT: "<<i<<
                                ", WIN INPORT: "<<cur_inport<<" }\n";
                    }
                }

            }
        }
        if(flag == false) {
            SAOutputWin[i] = -1; //仲裁失败
        }
    }

    //Step 5. Switch Traversal
    //交叉开关传输，传输完数据后必须把相关的状态reset，传输完的标志是把输入buffer中的tail flit传输到输出buffer中
    //pkt从输入buffer到输出buffer需要改变pkt中的vcid，改为下一跳路由器输入buffer的vcid（和这一跳路由器输出的vcid相同）
    for(int i = 0; i < PortNum; i++) {
        if(SAOutputWin[i] == -1) continue;
        int inport = SAOutputWin[i];
        int inport_vcid = SAInputWinVcid[inport];
        int output_vcid = RCInputVCState[inport][inport_vcid] % VC;
        DataPkt* current_pkt = InputBuffer[inport][inport_vcid][0];


        //有可能body flit还没传输过来，导致current_pkt == nullptr
        //可能有bug
        if(current_pkt == nullptr) {
            EV << "Error in Step 5. Switch Traversal in Router " << getIndex() << ", current_pkt is null" << endl;
            continue;
        }
        //修改VCID
        current_pkt->setVc_id(output_vcid);

        //对输入缓存进行shift
        for(int j = 0; j < BufferDepth - 1; j++){
            InputBuffer[inport][inport_vcid][j] = InputBuffer[inport][inport_vcid][j+1];
        }
        InputBuffer[inport][inport_vcid][BufferDepth-1] = nullptr;
        //将数据放到输出buffer
        if(OutputBuffer[i][output_vcid][OutBufferDepth-1] != nullptr) {
            EV << "Error in Step 5. Switch Traversal in Router, OutputBuffer != nullptr " << getIndex() << ", current_pkt is null" << endl;

        }
        OutputBuffer[i][output_vcid][OutBufferDepth-1] = current_pkt;

        //每转发input buffer里面的一个flit，就产生一个流控信号，通知上游router，进行increment credit操作
        //先将bufferInfoMsg放入Queue中，由于和Data Pkt共用一个信道，容易发生阻塞，需要队列保存数据
        int from_port = getNextRouterPort(inport);
        BufferInfoMsg* bufferInfoMsg = new BufferInfoMsg("bufferInfoMsg");
        bufferInfoMsg->setFrom_port(from_port);
        bufferInfoMsg->setVcid(inport_vcid);
        bufTxQueue[inport].insert(bufferInfoMsg);



        //判断是否为tail flit，如果是，则重置寄存器状态
        if(current_pkt->getIsTail() == true) {
            RCInputVCState[inport][inport_vcid] = -1;
            VAInputVCState[inport][inport_vcid] = false;
            if(Verbose >= VERBOSE_DETAIL_DEBUG_MESSAGES) {
                EV << "In Router: " << getIndex() << "(" << swpid2swlid(getIndex()) << "), VCA Input: Inport: "
                        << inport << ", vcid: " << inport_vcid << ", is released" << endl;
            }
        }

        if (Verbose >= VERBOSE_DEBUG_MESSAGES) {
            EV<<"Step 5. Switch Traversal >> ROUTER: "<<getIndex()<<"("<<swpid2swlid(getIndex())<<"), OUTPORT: "<<i<<
            ", OUTPORT VCID: "<<output_vcid<<", INPORT: "<<inport<<
            ", INPORT VCID: "<<inport_vcid<<", InputBuffer: { "<< current_pkt << " }" << '\n';
        }



    }

    //结束部分模块的计时
    t_end_r = clock();
    if (simTime().dbl() > RecordStartTime) {
        if(t_end_r - t_start_r > t_max_r) {
            t_max_r = t_end_r - t_start_r;
        }
        t_router += t_end_r - t_start_r;
    }


    //Step 6. Forward Data Message
    //发送数据，需要检查信道是否空闲以及
    for(int i = 0; i < PortNum; i++) {
        if(channelAvailTime(i) <= simTime()) {
            bool flag = false;
            for(int j = 0; j < VC && flag == false; j++) {
                if(OutputBuffer[i][j][0] != nullptr && BufferConnectCredit[i][j] != 0) {
                    flag = true;
                    DataPkt* forward_msg = OutputBuffer[i][j][0];

                    //对流控信息寄存器进行操作
                    OutputBuffer[i][j][0] = nullptr;
                    if(!connectToProcessor(i)) { //与路由器相连
                        BufferConnectCredit[i][j]--;
                    }

                    //判断是否为tail
                    //注意，outputBuffer的入口位置有空闲的时候，就需要重置寄存器状态，这样就可以进行下一轮的虚通道仲裁
//                            if(forward_msg->getIsTail() == true) {
//                                VAOutputVCState[i][j] = -1;
//                                if(Verbose >= VERBOSE_DETAIL_DEBUG_MESSAGES) {
//                                    EV << "In Router: " << getIndex() << "(" << swpid2swlid(getIndex()) << "), VCA Output: Output: "
//                                            << i << ", vcid: " << j << ", is released" << endl;
//                                }
//                            }

                    if (Verbose >= VERBOSE_DEBUG_MESSAGES) {
                        EV<<"Step 6. Forward Message >> ROUTER: "<<getIndex()<<"("<<swpid2swlid(getIndex())<<"), OUTPORT: "<<i<<
                        ", OUTPORT VCID: "<<j<<", Forward Pkt: { " <<forward_msg<<" }" <<'\n';
                    }

                    //发送pkt数据
                    forwardMessage(forward_msg, i);
                }
            }
        }
    }

    //Step 7. Forward bufferInfoMsg Message
    for(int i = 0; i < PortNum; i++) {
        if(bufTxQueue[i].isEmpty() || channelAvailTime(i) > simTime()) continue;
        BufferInfoMsg* bufferInfoMsg = (BufferInfoMsg*) bufTxQueue[i].front();
        bufTxQueue[i].pop(); //注意，先pop再发送
        //发送bufferInfoMsg
        forwardBufferInfoMsg(bufferInfoMsg, i);

    }

    //Step 8. Shift OutputBuffer
    for(int i = 0; i < PortNum; i++) {
        for(int j = 0; j < VC; j++) {
            for(int k = 0; k < OutBufferDepth; k++) {
                if(OutputBuffer[i][j][k] != nullptr && k > 0 && OutputBuffer[i][j][k-1] == nullptr) {
                    if (k == OutBufferDepth - 1 && OutputBuffer[i][j][k]->getIsTail() == true) {
                        VAOutputVCState[i][j] = -1;
                        if(Verbose >= VERBOSE_DETAIL_DEBUG_MESSAGES) {
                            EV << "In Router: " << getIndex() << "(" << swpid2swlid(getIndex()) << "), VCA Output: Output: "
                                    << i << ", vcid: " << j << ", is released" << endl;
                        }
                    }
                    OutputBuffer[i][j][k-1] = OutputBuffer[i][j][k];
                    OutputBuffer[i][j][k] = nullptr;
                }
            }
        }
    }
}
