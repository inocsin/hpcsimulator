/*
 * router.cc
 *
 *  Created on: 2016年7月30日
 *      Author: Vincent
 *
 *  Function:
 *      路由器：输入缓存，虚通道，虚通道仲裁(Round Robin)，交叉开关仲裁(Round Robin)，交叉开关传输
 *      数据包：Flit形式，Flit长度可调(根据Head Flit决定)
 */

#include "router.h"

Router::Router(){
    selfMsgAlloc=nullptr;
}

Router::~Router(){
    cancelAndDelete(selfMsgAlloc);
}

void Router::initialize()
{

    //对Buffer进行初始化
    for(int i = 0; i < PortNum; i++) {
        SAInputWinVcid[i] = -1;
        SAOutputWin[i] = -1;
        for(int j = 0; j < VC; j++) {
            for(int k = 0; k < OutBufferDepth; k++) {
                OutputBuffer[i][j][k] = nullptr;
            }

            RCInputVCState[i][j] = -1;
            BufferConnectCredit[i][j] = BufferDepth;
            VAOutputVCState[i][j] = -1;
            VAOutputVCStatePre[i][j] = -1;
            VAInputVCState[i][j] = false;
            //InputVCFlitCount[i][j] = 0;
            for(int k = 0; k < BufferDepth; k++){
                InputBuffer[i][j][k] = nullptr;
            }
        }
    }

    //对selfMsg进行初始化
    selfMsgAlloc = new cMessage("selfMsgAlloc");
    scheduleAt(Sim_Start_Time, selfMsgAlloc);

    RouterPower = 0;
    flitReceived = 0;
    t_start_h = t_end_h = t_max_h = t_start_r = t_end_r = t_max_r = 0;
    t_handleMessage = 0;
    t_router = 0;
    t_totalTime = clock();

    inputBufferOccupancy = 0.0;
    inputBufferEmptyTimes = 0.0;
    inputBufferFullTimes = 0.0;
    channelUnavailTimes = 0.0;
    bufferChannelUnavialTimes = 0.0;

}

void Router::handleMessage(cMessage *msg)
{
    t_start_h = clock();

    if (msg->isSelfMessage()) {
        //****************仲裁定时****************************

        if(msg == selfMsgAlloc){//自消息为仲裁定时消息

            handleAllocMessage(msg);

        } // end of selfMsgAlloc

    } // end of selfMsg
    else{ //非自消息，即收到其他路由器的消息
        //*************************收到其他端口buffer更新消息************************
        if(strcmp("bufferInfoMsg", msg->getName()) == 0){

            handleBufferInfoMessage(msg);

        }else{
            //**********************收到其他端口的DataPkt数据消息*******************

            handleInputDataPkt(msg);

        }//end of DataPkt

    } // end of Not self msg

    t_end_h = clock();

    if (simTime().dbl() > RecordStartTime) {
        if(t_end_h - t_start_h > t_max_h) {
            t_max_h = t_end_h - t_start_h;
        }
        t_handleMessage += t_end_h - t_start_h;
    }

}

void Router::handleAllocMessage(cMessage *msg)
{
    scheduleAt(simTime()+CLK_CYCLE, selfMsgAlloc);

    // calculate adjacent routers' input buffer occupancy
    calcInputBufferOccupancy();

    //Step 2. Routing Logic
    //计算每个packet的输出端口及输出vcid
    step2RoutingLogic();

    //开始对部分模块进行计时
    t_start_r = clock();

    //Step 3. Virtual Channel Allocation
    step3VCAllocation();

    // Step 4. Switch Arbitration && Step 5. Switch Traversal
    step4_5_SA_ST();


    //结束部分模块的计时
    t_end_r = clock();
    if (simTime().dbl() > RecordStartTime) {
        if(t_end_r - t_start_r > t_max_r) {
            t_max_r = t_end_r - t_start_r;
        }
        t_router += t_end_r - t_start_r;
    }

    //Notes:
    //由于DataMsg和BufferMsg占用同一个信道，BufferInfoMsg的大小为0Byte，DataMsg大小>0Byte，因此，如果先发送
    //DataMsg，则信道被占用，channelAvailTime()>simTime()，导致BufferInfoMsg无法发送。相反，如果先发送
    //0Byte大小的BufferInfoMsg，数据的TransmissionTime = 0 / Bandwidth = 0, 因此channelAvailTime()<=simTime(),
    //不会占用信道，DataMsg也可以正常发送。因此必须保证BufferInfoMsg在DataMsg之前发送

    //Step 6. Forward bufferInfoMsg Message
    step6ForwardBufferInfoMsg();

    //Step 7. Forward Data Message & Shift OutputBuffer
    step7ForwardDataMsg();

}

void Router::step2RoutingLogic()
{
    for(int i = 0; i < PortNum; i++) {
        for(int j = 0; j < VC; j++) {
            DataPkt* current_pkt = InputBuffer[i][j][0];
            //InputBuffer队头的Pkt还没有经过Routing Computing，-1代表还没RC过
            if(current_pkt != nullptr && current_pkt->getIsHead() == true && RCInputVCState[i][j] == -1) { //数据包有可能阻塞在队头
                RCInputVCState[i][j] = getPortAndVCID(current_pkt);
                if (Verbose >= VERBOSE_DEBUG_MESSAGES) {
                    EV<<"Step 2. Routing Computation >> ROUTER: "<<getIndex()<<"("<<swpid2swlid(getIndex())<<"), INPORT: "<<i<<
                            ", INPORT VCID: "<<j<<", Routing Result: OUTPORT: "<<RCInputVCState[i][j]/VC<<
                            ", OUTPUT VCID: "<<RCInputVCState[i][j]%VC<<", Msg: { "<<InputBuffer[i][j][0]<<" }\n";
                }
            }
        }
    }
}

void Router::step3VCAllocation()
{
    //每一个输出端口的每一个virtual channel要对每个输入端口的每个virtual channel进行仲裁，只有一个胜利
    //外循环，对每个output virtual channel进行循环
    for(int i = 0; i < PortNum; i++) {
        for(int j = 0; j < VC; j++) {
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
}

void Router::step4_5_SA_ST() {
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
            //SAInputWinVcid[cur_inport] != -1 保证输入有数据，输出有buffer, 上一步仲裁结果
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
            VAOutputVCState[i][output_vcid] = -1;
            if(Verbose >= VERBOSE_DETAIL_DEBUG_MESSAGES) {
                EV << "In Router: " << getIndex() << "(" << swpid2swlid(getIndex()) << "), VCA Input: Inport: "
                        << inport << ", vcid: " << inport_vcid << ", is released" << endl;
                EV << "In Router: " << getIndex() << "(" << swpid2swlid(getIndex()) << "), VCA Output: Output: "
                        << i << ", vcid: " << output_vcid << ", is released" << endl;
            }
        }

        if (Verbose >= VERBOSE_DEBUG_MESSAGES) {
            EV<<"Step 5. Switch Traversal >> ROUTER: "<<getIndex()<<"("<<swpid2swlid(getIndex())<<"), OUTPORT: "<<i<<
            ", OUTPORT VCID: "<<output_vcid<<", INPORT: "<<inport<<
            ", INPORT VCID: "<<inport_vcid<<", InputBuffer: { "<< current_pkt << " }" << '\n';
        }

    }
}

void Router::step6ForwardBufferInfoMsg()
{
    for(int i = 0; i < PortNum; i++) {
        if(channelAvailTime(i) > simTime() && simTime().dbl() > RecordStartTime) {
            bufferChannelUnavialTimes += 1;
        }
        if(bufTxQueue[i].isEmpty() || channelAvailTime(i) > simTime()) continue;
        BufferInfoMsg* bufferInfoMsg = (BufferInfoMsg*) bufTxQueue[i].front();
        bufTxQueue[i].pop(); //注意，先pop再发送
        //发送bufferInfoMsg
        forwardBufferInfoMsg(bufferInfoMsg, i);
    }
}

void Router::step7ForwardDataMsg()
{
    //发送数据，需要检查信道是否空闲以及
    for(int i = 0; i < PortNum; i++) {
        if(channelAvailTime(i) > simTime() && simTime().dbl() > RecordStartTime) {
            channelUnavailTimes += 1;
        }
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

    // shift OutputBuffer
    for(int i = 0; i < PortNum; i++) {
        for(int j = 0; j < VC; j++) {
            for(int k = 0; k < OutBufferDepth; k++) {
                if(OutputBuffer[i][j][k] != nullptr && k > 0 && OutputBuffer[i][j][k-1] == nullptr) {
                    OutputBuffer[i][j][k-1] = OutputBuffer[i][j][k];
                    OutputBuffer[i][j][k] = nullptr;
                }
            }
        }
    }
}

void Router::handleBufferInfoMessage(cMessage *msg)
{
    //收到的消息为buffer状态消息，更新BufferConnectCredit[PortNum][VC]
    BufferInfoMsg* bufferInfoMsg = check_and_cast<BufferInfoMsg*>(msg);
    int from_port = bufferInfoMsg->getFrom_port();
    int vcid = bufferInfoMsg->getVcid();
    BufferConnectCredit[from_port][vcid]++;

    if (Verbose >= VERBOSE_DEBUG_MESSAGES) {
        EV<<"Receiving bufferInfoMsg >> ROUTER: "<<getIndex()<<"("<<swpid2swlid(getIndex())<<"), INPORT: "<<from_port<<
            ", Received MSG: { "<<bufferInfoMsg<<" }\n";
        EV<<"BufferConnectCredit["<<from_port<<"]["<<vcid<<"]="<<BufferConnectCredit[from_port][vcid]<<"\n";
        EV<<"Msg transmition time: "<<simTime().dbl() - bufferInfoMsg->getTransmit_start() << "\n";
    }
    delete bufferInfoMsg;
}

void Router::handleInputDataPkt(cMessage *msg)
{
    DataPkt *datapkt = check_and_cast<DataPkt*>(msg);
    if (simTime().dbl() > RecordStartTime) {
        flitReceived += 1;
    }

    //Step 1. Input Buffer
    //决定放到哪个input port的virtual channel
    int input_port = datapkt->getFrom_router_port();
    int vc_id = datapkt->getVc_id();

    //由于有流控机制的存在，上一跳路由器向该路由器发送FatTreePkt时，buffer一定有容量来保存数据
    for(int i = 0; i < BufferDepth; i++){
        if(InputBuffer[input_port][vc_id][i] == nullptr){
            InputBuffer[input_port][vc_id][i] = datapkt;
            break;
        }
    }
    if (Verbose >= VERBOSE_DEBUG_MESSAGES) {
        EV<<"Step 1. Input Buffer >> ROUTER: "<<getIndex()<<"("<<swpid2swlid(getIndex())<<"), INPORT: "<<input_port<<
            ", VCID: "<<vc_id<<", Received MSG: { "<<datapkt<<" }\n";
        EV<<"Msg transmition time: "<<simTime().dbl() - datapkt->getTransmit_start() << "\n";
    }
}

//调用...计算路由端口
void Router::forwardMessage(DataPkt *msg, int out_port_id)
{

    // Increment hop count.
    msg->setHopCount(msg->getHopCount()+1);

    //int k=calRoutePort(msg);//计算发出msg的端口号
    int k = out_port_id;
    char str1[20]="port_";
    char str2[20];
    sprintf(str2, "%d", k);
    strcat(str1,str2);
    strcat(str1,"$o");
    //EV<<"k="<<k<<" str1="<<str1<<" str2="<<str2<<"\n";
    msg->setFrom_router_port(getNextRouterPort(k));//设置接受该msg的Router的port端口号
    msg->setTransmit_start(simTime().dbl());
    send(msg,str1);
    int cur_swpid=getIndex();//当前路由器的id
    int cur_swlid=swpid2swlid(cur_swpid);
    if (Verbose >= VERBOSE_DETAIL_DEBUG_MESSAGES) {
        EV << "Forwarding message { " << msg << " } from router "<<cur_swpid<<"("<<cur_swlid<<")"<< " through port "<<k<<"\n";
    }


}

void Router::forwardBufferInfoMsg(BufferInfoMsg *msg, int out_port_id){

    int k = out_port_id;
    char str1[20]="port_";
    char str2[20];
    sprintf(str2, "%d", k);
    strcat(str1,str2);
    strcat(str1,"$o");
    //EV<<"k="<<k<<" str1="<<str1<<" str2="<<str2<<"\n";
    msg->setFrom_port(getNextRouterPort(k));//设置接受该msg的Router的port端口号
    msg->setTransmit_start(simTime().dbl());
    send(msg,str1);
    int cur_swpid=getIndex();//当前路由器的id
    int cur_swlid=swpid2swlid(cur_swpid);
    if (Verbose >= VERBOSE_DEBUG_MESSAGES) {
        EV << "Step 7 Forwarding BufferInfoMsg { " << msg << " } from router "<<cur_swpid<<"("<<cur_swlid<<")"<< " through port "<<k<<"\n";
    }

}

int Router::getPortAndVCID(DataPkt* msg) {
    int port = calRoutePort(msg);
    int best_vcid = 0;
    int max_avail = 0;
    int vc_id = intuniform(0,VC-1); //随机分配一个通道，以此vc开始循环判断是否有空的vc，增加随机分布，防止每次都从0开始
    for(int i= 0; i < VC; i++){
        int vcid_tmp = (vc_id + i)%VC;
        if(BufferConnectCredit[port][vcid_tmp] != 0){
            if(BufferConnectCredit[port][vcid_tmp] > max_avail) {
                max_avail = BufferConnectCredit[port][vcid_tmp];
                best_vcid = vcid_tmp;
            }
        }
    }
    //return value port * VC + vcid
    return port * VC + best_vcid;
}


simtime_t Router::channelAvailTime(int port_num) {
    int k = port_num;
    char str1[20] = "port_";
    char str2[20];
    sprintf(str2, "%d", k);
    strcat(str1, str2);
    strcat(str1, "$o");

    cChannel* txChannel = gate(str1)->getTransmissionChannel();
    simtime_t txFinishTime = txChannel->getTransmissionFinishTime();
    return txFinishTime;
}

double Router::getRouterPower() {

    double timeCount = simTime().dbl() - Sim_Start_Time;
    double clockCount = timeCount / CLK_CYCLE; //时钟周期数
    double TR = flitReceived / (PortNum * clockCount);
    //recordScalar("TR", );
    //instances
    int XBAR_insts = PortNum * PortNum * FlitWidth;
    int SWVC_insts = 9 * ((pow(PortNum,2) * VC * VC) + pow(PortNum,2) + (PortNum * VC) - PortNum);
    int INBUF_insts = 180 * PortNum * VC + 2 * PortNum * VC * BufferDepth * FlitWidth + 2* PortNum *
            PortNum * VC * BufferDepth + 3 * PortNum * VC * BufferDepth + 5 * PortNum * PortNum
            * BufferDepth + PortNum * PortNum + PortNum * FlitWidth + 15 * PortNum;
    int OUTBUF_insts = 25 * PortNum + 80 * PortNum * VC;
    int CLKCTRL_insts = 0.02 * (SWVC_insts + INBUF_insts + OUTBUF_insts);

    //leakage power
    double XBAR_leakage_power = MUX2_leak_nW * XBAR_insts;
    double SWVC_leakage_power = ((6*NOR_leak_nW + 2*INV_leak_nW + DFF_leak_nW)/9)* SWVC_insts;
    double INBUF_leakage_power = ((AOI_leak_nW + DFF_leak_nW)/2) * INBUF_insts;
    double OUTBUF_leakage_power = ((AOI_leak_nW + DFF_leak_nW)/2) * OUTBUF_insts;
    double CLKCTRL_leakage_power = ((AOI_leak_nW + INV_leak_nW)/2) * CLKCTRL_insts;

    //internal power
    double XBAR_internal_power = MUX2_int_J * TR * XBAR_insts;
    double SWVC_internal_power = (6*NOR_int_J + 2*INV_int_J + DFF_int_J) * TR * SWVC_insts;
    double INBUF_internal_power = (AOI_int_J + DFF_int_J) * .5 * (INBUF_insts * TR + .05 * INBUF_insts);
    double OUTBUF_internal_power = (AOI_int_J + DFF_int_J) * .5 * (OUTBUF_insts * TR + .05 * OUTBUF_insts);
    double CLKCTRL_internal_power = (AOI_int_J + INV_int_J) * CLKCTRL_insts * TR;

    //switching power
    double XBAR_switching_power = 0.5 * 1.4 * MUX2_load_pF * VDD * VDD * TR * FREQ;
    double SWVC_switching_power = 0.5 *1.4 * (NOR_load_pF + INV_load_pF + DFF_load_pF) * VDD *  VDD * FREQ * SWVC_insts * TR;
    double INBUF_switching_power = 0.5 *1.4 * VDD * VDD * FREQ * .5 * (INBUF_insts * TR * AOI_load_pF + .05 * INBUF_insts * DFF_load_pF);
    double OUTBUF_switching_power = 0.5 *1.4 * VDD * VDD * FREQ * .5 * (OUTBUF_insts * TR * AOI_load_pF + .05 * OUTBUF_insts * DFF_load_pF);
    double CLKCTRL_switching_power = .5 * 1.4 *(INV_load_pF + AOI_load_pF) * VDD * VDD * FREQ * CLKCTRL_insts * TR;

    double p_leakage = 1e-6 * (XBAR_leakage_power + SWVC_leakage_power + INBUF_leakage_power + OUTBUF_leakage_power + CLKCTRL_leakage_power);
    double p_internal = XBAR_internal_power + SWVC_internal_power + INBUF_internal_power + OUTBUF_internal_power + CLKCTRL_internal_power;
    double p_switching = 1e-9 * (XBAR_switching_power + SWVC_switching_power + INBUF_switching_power + OUTBUF_switching_power + CLKCTRL_switching_power);

    double p_total = p_leakage + p_internal + p_switching;

    return p_total;


}

void Router::calcInputBufferOccupancy()
{
    if(simTime().dbl() > RecordStartTime) {
        for(int i = 0; i < PortNum; i++) {
            if(!connectToProcessor(i)) {
                for(int j = 0; j < VC; j++) {
                    inputBufferOccupancy += 1.0 * BufferDepth - BufferConnectCredit[i][j];
                    if(BufferConnectCredit[i][j] == 0) {
                        ++inputBufferFullTimes;
                    } else if(BufferConnectCredit[i][j] == BufferDepth) {
                        ++inputBufferEmptyTimes;
                    }
                }
            }
        }
    }

}

void Router::finish()
{
    // This function is called by OMNeT++ at the end of the simulation.

    double routerPower = getRouterPower();
    EV <<"Router power: " << routerPower <<endl;
    recordScalar("routerPower", routerPower);
    if(getIndex() == 0) {
        t_totalTime = clock() - t_totalTime;
        recordScalar("realTotalTime", t_totalTime * 1.0 / CLOCKS_PER_SEC); // 单位是s
    }
    recordScalar("realTotalHandleMessageTime", t_handleMessage * 1.0 / CLOCKS_PER_SEC);
    recordScalar("realRouterTime", t_router * 1.0 / CLOCKS_PER_SEC);
    recordScalar("realMaxHandleMessagetime", t_max_h * 1.0 / CLOCKS_PER_SEC);
    recordScalar("realMaxRouterTime", t_max_r * 1.0 / CLOCKS_PER_SEC);


    double timeCount = (simTime().dbl() - RecordStartTime) / (CLK_CYCLE);
    int port_num = 0;
    for(int i = 0; i < PortNum; i++) {
        if(!connectToProcessor(i)) {
            ++port_num;
        }
    }
    double totalInputBufferOccupancy = 1.0 * port_num * VC * BufferDepth * timeCount;
    inputBufferOccupancy = inputBufferOccupancy / totalInputBufferOccupancy;
    recordScalar("inputBufferOccupancy", inputBufferOccupancy);
    recordScalar("inputBufferFullTimes", inputBufferFullTimes / (timeCount * VC * port_num));
    recordScalar("inputBufferEmptyTimes", inputBufferEmptyTimes / (timeCount * VC * port_num));
    recordScalar("channelUnavailTimes", channelUnavailTimes / (timeCount * PortNum));
    recordScalar("bufferChannelUnavialTimes", bufferChannelUnavialTimes / (timeCount * PortNum));

}

