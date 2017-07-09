/*
 * router.cc
 *
 *  Created on: 2016��7��30��
 *      Author: Vincent
 *
 *  Function:
 *      ·���������뻺�棬��ͨ������ͨ���ٲ�(Round Robin)�����濪���ٲ�(Round Robin)�����濪�ش���
 *      ���ݰ���Flit��ʽ��Flit���ȿɵ�(����Head Flit����)
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


    //��Buffer���г�ʼ��
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


    //��selfMsg���г�ʼ��
    selfMsgAlloc = new cMessage("selfMsgAlloc");
    scheduleAt(Sim_Start_Time, selfMsgAlloc);

    RouterPower = 0;
    flitReceived = 0;
    t_start_h = t_end_h = t_max_h = t_start_r = t_end_r = t_max_r = 0;
    t_handleMessage = 0;
    t_router = 0;
    t_totalTime = clock();

}

void Router::handleMessage(cMessage *msg)
{
    t_start_h = clock();

    if (msg->isSelfMessage()) {
        //****************�ٲö�ʱ****************************

        if(msg == selfMsgAlloc){//����ϢΪ�ٲö�ʱ��Ϣ

            scheduleAt(simTime()+CLK_CYCLE, selfMsgAlloc);

            //Step 2. Routing Logic
            //����ÿ��packet������˿ڼ����vcid
            for(int i = 0; i < PortNum; i++) {
                for(int j = 0; j < VC; j++) {
                    DataPkt* current_pkt = InputBuffer[i][j][0];
                    //InputBuffer��ͷ��Pkt��û�о���Routing Computing��-1����ûRC��
                    if(current_pkt != nullptr && current_pkt->getIsHead() == true && RCInputVCState[i][j] == -1) { //���ݰ��п��������ڶ�ͷ
                        RCInputVCState[i][j] = getPortAndVCID(current_pkt);
                        if (Verbose >= VERBOSE_DEBUG_MESSAGES) {
                            EV<<"Step 2. Routing Computation >> ROUTER: "<<getIndex()<<"("<<swpid2swlid(getIndex())<<"), INPORT: "<<i<<
                                    ", INPORT VCID: "<<j<<", Routing Result: OUTPORT: "<<RCInputVCState[i][j]/VC<<
                                    ", OUTPUT VCID: "<<RCInputVCState[i][j]%VC<<", Msg: { "<<InputBuffer[i][j][0]<<" }\n";
                        }
                    }
                }
            }

            //��ʼ�Բ���ģ����м�ʱ
            t_start_r = clock();

            //Step 3. Virtual Channel Allocation
            //ÿһ������˿ڵ�ÿһ��virtual channelҪ��ÿ������˿ڵ�ÿ��virtual channel�����ٲã�ֻ��һ��ʤ��
            //��ѭ������ÿ��output virtual channel����ѭ��
            for(int i = 0; i < PortNum; i++) {
                for(int j = 0; j < VC; j++) {
                    //��ѭ������ÿ��input virtual channel�����ж�
                    //ע�⣺����OutputBuffer��tail flit�����͵�outputBuffer��һ��Ҫ�ͷ�VAOutputVCState�������ͨ��״̬�Ĵ���Ҫ�ͷ�
                    if(VAOutputVCState[i][j] >= 0) continue; //������˿�vc�ѱ����䣬����������ѭ��
                    bool flag = false;
                    int port_vc = VAOutputVCStatePre[i][j] + 1;
                    int count = 0, total = PortNum * VC;
                    for(; count < total && flag == false; count++, port_vc++) {
                        int m = (port_vc / VC) % PortNum;
                        int n = port_vc % VC;
                        if(RCInputVCState[m][n] == i * VC + j) {//��������ͨ�����������һ����head flit, ��������if��������
                            if(VAInputVCState[m][n] == true) {
                                EV << "Error: Step 3. VC Allocation >> ROUTER: " << getIndex()<<"("<<swpid2swlid(getIndex()) << "), VAInputVCState != false" << endl;
                            }
                            VAInputVCState[m][n] = true; //����ͨ���ٲ�ʤ��
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
            //��VAInputVCState��Ϊtrue������vc����ѡ�񣬶������Ӧ������˿ڵ�vc��buffer����Ϊ�գ��ܹ�������
            //4.1. ����˿��ٲ�
            for(int i = 0; i < PortNum; i++) {
                int last_vcid = SAInputWinVcid[i] + 1;
                bool flag = false;
                for(int j = 0; j < VC && flag == false; j++, last_vcid++) {
                    int cur_vcid = last_vcid % VC; //Round Robin
                    //���뱣֤������ͨ������ͨ���ٲ���ʤ�����������ͨ��Ϊ��������ͨ��������
                    //���⻹��Ҫ��֤����buffer�����ݣ�����head flit����ʤ��������body flit��û����
                    if(VAInputVCState[i][cur_vcid] == true && InputBuffer[i][cur_vcid][0] != nullptr) { //֮ǰд��InputBuffer[i][cur_vcid] != nullptr������3�죬�������Ҫ�ж�����buffer�Ƿ������ݣ����ܶ���ǰһ��·����
                        int port = RCInputVCState[i][cur_vcid] / VC;
                        int out_vcid = RCInputVCState[i][cur_vcid] % VC;
                        if(OutputBuffer[port][out_vcid][OutBufferDepth-1] == nullptr) { //�ж����buffer�пռ�
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
                    SAInputWinVcid[i] = -1; //û�к��ʵ���ͨ��
                }
            }
            //4.2. ����˿��ٲ�
            for(int i = 0; i < PortNum; i++) {
                //��ѭ������ÿһ������˿��ж��Ƿ����������˿�
                int last_inport = SAOutputWin[i] + 1; //Round Robin
                bool flag = false;
                for(int j = 0; j < PortNum && flag == false; j++, last_inport++) {
                    int cur_inport = last_inport % PortNum;
                    //SAInputWinVcid[cur_inport] != -1��֤���������ݣ������buffer
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
                    SAOutputWin[i] = -1; //�ٲ�ʧ��
                }
            }

            //Step 5. Switch Traversal
            //���濪�ش��䣬���������ݺ�������ص�״̬reset��������ı�־�ǰ�����buffer�е�tail flit���䵽���buffer��
            //pkt������buffer�����buffer��Ҫ�ı�pkt�е�vcid����Ϊ��һ��·��������buffer��vcid������һ��·���������vcid��ͬ��
            for(int i = 0; i < PortNum; i++) {
                if(SAOutputWin[i] == -1) continue;
                int inport = SAOutputWin[i];
                int inport_vcid = SAInputWinVcid[inport];
                int output_vcid = RCInputVCState[inport][inport_vcid] % VC;
                DataPkt* current_pkt = InputBuffer[inport][inport_vcid][0];


                //�п���body flit��û�������������current_pkt == nullptr
                //������bug
                if(current_pkt == nullptr) {
                    EV << "Error in Step 5. Switch Traversal in Router " << getIndex() << ", current_pkt is null" << endl;
                    continue;
                }
                //�޸�VCID
                current_pkt->setVc_id(output_vcid);

                //�����뻺�����shift
                for(int j = 0; j < BufferDepth - 1; j++){
                    InputBuffer[inport][inport_vcid][j] = InputBuffer[inport][inport_vcid][j+1];
                }
                InputBuffer[inport][inport_vcid][BufferDepth-1] = nullptr;
                //�����ݷŵ����buffer
                if(OutputBuffer[i][output_vcid][OutBufferDepth-1] != nullptr) {
                    EV << "Error in Step 5. Switch Traversal in Router, OutputBuffer != nullptr " << getIndex() << ", current_pkt is null" << endl;

                }
                OutputBuffer[i][output_vcid][OutBufferDepth-1] = current_pkt;

                //ÿת��input buffer�����һ��flit���Ͳ���һ�������źţ�֪ͨ����router������increment credit����
                //�Ƚ�bufferInfoMsg����Queue�У����ں�Data Pkt����һ���ŵ������׷�����������Ҫ���б�������
                int from_port = getNextRouterPort(inport);
                BufferInfoMsg* bufferInfoMsg = new BufferInfoMsg("bufferInfoMsg");
                bufferInfoMsg->setFrom_port(from_port);
                bufferInfoMsg->setVcid(inport_vcid);
                bufTxQueue[inport].insert(bufferInfoMsg);



                //�ж��Ƿ�Ϊtail flit������ǣ������üĴ���״̬
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

            //��������ģ��ļ�ʱ
            t_end_r = clock();
            if (simTime().dbl() > RecordStartTime) {
                if(t_end_r - t_start_r > t_max_r) {
                    t_max_r = t_end_r - t_start_r;
                }
                t_router += t_end_r - t_start_r;
            }


            //Step 6. Forward Data Message
            //�������ݣ���Ҫ����ŵ��Ƿ�����Լ�
            for(int i = 0; i < PortNum; i++) {
                if(channelAvailTime(i) <= simTime()) {
                    bool flag = false;
                    for(int j = 0; j < VC && flag == false; j++) {
                        if(OutputBuffer[i][j][0] != nullptr && BufferConnectCredit[i][j] != 0) {
                            flag = true;
                            DataPkt* forward_msg = OutputBuffer[i][j][0];

                            //��������Ϣ�Ĵ������в���
                            OutputBuffer[i][j][0] = nullptr;
                            if(!connectToProcessor(i)) { //��·��������
                                BufferConnectCredit[i][j]--;
                            }

                            //�ж��Ƿ�Ϊtail
                            //ע�⣬outputBuffer�����λ���п��е�ʱ�򣬾���Ҫ���üĴ���״̬�������Ϳ��Խ�����һ�ֵ���ͨ���ٲ�
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

                            //����pkt����
                            forwardMessage(forward_msg, i);
                        }
                    }
                }
            }

            //Step 7. Forward bufferInfoMsg Message
            for(int i = 0; i < PortNum; i++) {
                if(bufTxQueue[i].isEmpty() || channelAvailTime(i) > simTime()) continue;
                BufferInfoMsg* bufferInfoMsg = (BufferInfoMsg*) bufTxQueue[i].front();
                bufTxQueue[i].pop(); //ע�⣬��pop�ٷ���
                //����bufferInfoMsg
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

        } // end of selfMsgAlloc

    } // end of selfMsg
    else{ //������Ϣ�����յ�����·��������Ϣ
        //*************************�յ������˿�buffer������Ϣ************************
        if(strcmp("bufferInfoMsg", msg->getName()) == 0){
            //�յ�����ϢΪbuffer״̬��Ϣ������BufferConnectCredit[PortNum][VC]
            BufferInfoMsg* bufferInfoMsg = check_and_cast<BufferInfoMsg*>(msg);
            int from_port = bufferInfoMsg->getFrom_port();
            int vcid = bufferInfoMsg->getVcid();
            BufferConnectCredit[from_port][vcid]++;

            if (Verbose >= VERBOSE_DEBUG_MESSAGES) {
                EV<<"Receiving bufferInfoMsg >> ROUTER: "<<getIndex()<<"("<<swpid2swlid(getIndex())<<"), INPORT: "<<from_port<<
                    ", Received MSG: { "<<bufferInfoMsg<<" }\n";
                EV<<"BufferConnectCredit["<<from_port<<"]["<<vcid<<"]="<<BufferConnectCredit[from_port][vcid]<<"\n";
            }
            delete bufferInfoMsg;


        }else{
            //**********************�յ������˿ڵ�DataPkt������Ϣ*******************

            DataPkt *datapkt = check_and_cast<DataPkt*>(msg);
            if (simTime().dbl() > RecordStartTime) {
                flitReceived += 1;
            }

            //Step 1. Input Buffer
            //�����ŵ��ĸ�input port��virtual channel
            int input_port = datapkt->getFrom_router_port();
            int vc_id = datapkt->getVc_id();

            //���������ػ��ƵĴ��ڣ���һ��·�������·��������FatTreePktʱ��bufferһ������������������
            for(int i = 0; i < BufferDepth; i++){
                if(InputBuffer[input_port][vc_id][i] == nullptr){
                    InputBuffer[input_port][vc_id][i] = datapkt;
                    break;
                }
            }
            if (Verbose >= VERBOSE_DEBUG_MESSAGES) {
                EV<<"Step 1. Input Buffer >> ROUTER: "<<getIndex()<<"("<<swpid2swlid(getIndex())<<"), INPORT: "<<input_port<<
                    ", VCID: "<<vc_id<<", Received MSG: { "<<datapkt<<" }\n";
            }

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


//����...����·�ɶ˿�
void Router::forwardMessage(DataPkt *msg, int out_port_id)
{

    // Increment hop count.
    msg->setHopCount(msg->getHopCount()+1);

    //int k=calRoutePort(msg);//���㷢��msg�Ķ˿ں�
    int k = out_port_id;
    char str1[20]="port_";
    char str2[20];
    sprintf(str2, "%d", k);
    strcat(str1,str2);
    strcat(str1,"$o");
    //EV<<"k="<<k<<" str1="<<str1<<" str2="<<str2<<"\n";
    msg->setFrom_router_port(getNextRouterPort(k));//���ý��ܸ�msg��Router��port�˿ں�
    send(msg,str1);
    int cur_swpid=getIndex();//��ǰ·������id
    int cur_swlid=swpid2swlid(cur_swpid);
    if (Verbose >= VERBOSE_DEBUG_MESSAGES) {
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
    msg->setFrom_port(getNextRouterPort(k));//���ý��ܸ�msg��Router��port�˿ں�
    send(msg,str1);
    int cur_swpid=getIndex();//��ǰ·������id
    int cur_swlid=swpid2swlid(cur_swpid);
    if (Verbose >= VERBOSE_DEBUG_MESSAGES) {
        EV << "Forwarding BufferInfoMsg { " << msg << " } from router "<<cur_swpid<<"("<<cur_swlid<<")"<< " through port "<<k<<"\n";
    }

}

int Router::getPortAndVCID(DataPkt* msg) {
    int port = calRoutePort(msg);
    int best_vcid = 0;
    for(int i = 0; i < VC; i++){
        if(BufferConnectCredit[port][i] > BufferConnectCredit[port][best_vcid]){
            best_vcid = i;
        }
    }
    //return value port * VC + vcid
    return port * VC + best_vcid;
}

int Router::getNextRouterAvailVCID(int port_num){
    int vc_id = intuniform(0,VC-1); //�������һ��ͨ�����Դ�vc��ʼѭ���ж��Ƿ��пյ�vc����������ֲ�����ֹÿ�ζ���0��ʼ
    for(int i=0;i<VC;i++){
        int vcid_tmp = (vc_id + i)%VC;
        if(BufferConnectCredit[port_num][vcid_tmp] != 0){
            return vcid_tmp;
        }
    }
    return -1;

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
    double clockCount = timeCount / CLK_CYCLE; //ʱ��������
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

void Router::finish()
{
    // This function is called by OMNeT++ at the end of the simulation.

    double routerPower = getRouterPower();
    EV <<"Router power: " << routerPower <<endl;
    recordScalar("routerPower", routerPower);
    if(getIndex() == 0) {
        t_totalTime = clock() - t_totalTime;
        recordScalar("realTotalTime", t_totalTime * 1.0 / CLOCKS_PER_SEC); // ��λ��s
    }
    recordScalar("realTotalHandleMessageTime", t_handleMessage * 1.0 / CLOCKS_PER_SEC);
    recordScalar("realRouterTime", t_router * 1.0 / CLOCKS_PER_SEC);
    recordScalar("realMaxHandleMessagetime", t_max_h * 1.0 / CLOCKS_PER_SEC);
    recordScalar("realMaxRouterTime", t_max_r * 1.0 / CLOCKS_PER_SEC);
}

