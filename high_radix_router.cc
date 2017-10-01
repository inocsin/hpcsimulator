/*
 * high_radix_router.cc
 *
 *  Created on: 2017��9��27��
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
    //����ÿ��packet������˿ڼ����vcid
    step2RoutingLogic();

    //��ʼ�Բ���ģ����м�ʱ
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
    //ÿһ������˿ڵ�ÿһ��virtual channelҪ��ÿ������˿ڵ�ÿ��virtual channel�����ٲã�ֻ��һ��ʤ��
    //��ѭ������ÿ��output virtual channel����ѭ��
    for(int i = 0; i < PortNum; i++) { // each output port
        for(int j = 0; j < VC; j++) { // each virtual channel
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
}
