/*
 * processor.cc
 *
 *  Created on: 2016��7��30��
 *      Author: Vincent
 *
 *  Function:
 *      Flitģʽ��Head Flit��Body Flit
 *      ʱ��ֲ���������(FlitLength������)������(FlitLength����)
 *      �ռ�ֲ�������
 *
 */

#include "processor.h"
#include "math.h"
#include <assert.h>

Processor::Processor(){
    selfMsgGenMsg=nullptr;
    selfMsgSendMsg=nullptr;
    selfMsgCalcBufferOccupy=nullptr;

}

Processor::~Processor(){
    cancelAndDelete(selfMsgGenMsg);
    cancelAndDelete(selfMsgSendMsg);
    cancelAndDelete(selfMsgCalcBufferOccupy);

}

void Processor::initialize()
{
    // Initialize variables
    numFlitSent = 0;
    numPackageSent = 0;
    numFlitReceived = 0;
    numPackageReceived = 0;
    numPktDropped = 0;
    flitByHop = 0;

    hopCountTotal = 0;
    hopCountCount = 0;
    flitDelayTimeTotal = 0;
    flitDelayTimeCount = 0;
    packetDelayTimeTotal = 0;
    packetDelayTimeCount = 0;
    creditMsgDelayTimeTotal = 0;
    creditMsgDelayTimeCount = 0;

    inputBufferOccupancy = 0.0;
    inputBufferEmptyTimes = 0.0;
    inputBufferFullTimes = 0.0;
    channelUnavailTimes = 0.0;

    selfMsgSendMsg = new cMessage("selfMsgSendMsg");//ע��˳���ȷ���buffer�����msg���ٲ����µ�msg������һ��flit��Ҫ2�����ڲŻᷢ��ȥ
    scheduleAt(Sim_Start_Time, selfMsgSendMsg);
    selfMsgGenMsg = new cMessage("selfMsgGenMsg");
    scheduleAt(Sim_Start_Time, selfMsgGenMsg);
    selfMsgCalcBufferOccupy = new cMessage("selfMsgCalcBufferOccupy");
    scheduleAt(Sim_Start_Time, selfMsgCalcBufferOccupy);

    for (int i = 0; i < VC; i++) {
        BufferConnectCredit[i] = BufferDepth; //��ʼ������������router��buffer��Ϊ��
    }

    dropFlag = false;
    hotspotIndex = -1;

}

void Processor::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        if(msg == selfMsgCalcBufferOccupy) {
            calcInputBufferOccupancy();
            scheduleAt(simTime()+CLK_CYCLE, selfMsgCalcBufferOccupy);
        }
        //********************���������ݵ��Զ�ʱ��Ϣ********************
        else if(msg == selfMsgSendMsg) {
            //****************************ת��flit**************************
            handleSendMsg();

        }else if(msg == selfMsgGenMsg){

            handleGenMsg();

        }

    }else{ // end of self msg
        //************************��self message*********************
        //************************�յ�buffer������Ϣ******************
        if(strcmp("bufferInfoMsg", msg->getName()) == 0){

            handleBufferInfoMsg(msg);

        }else{
        //***********************�յ�DataPkt��Ϣ***********************
            handleDataPkt(msg);

        }
    }
}

void Processor::handleSendMsg()
{
    if(!txQueue.isEmpty()){ //���Ͷ���������
        DataPkt* current_forward_msg = (DataPkt*) txQueue.front();
        int vcid = current_forward_msg->getVc_id();
        if(channelAvailTime() > simTime() && simTime().dbl() > RecordStartTime) {
            channelUnavailTimes += 1;
        }
        if(channelAvailTime() <= simTime() && BufferConnectCredit[vcid] != 0) { //���Ͷ˿ڿ��У���һ���ڵ���buffer���ܴ�flit
            txQueue.pop();
            forwardMessage(current_forward_msg);
            BufferConnectCredit[vcid]--; //decrement credit count

            if (simTime().dbl() > RecordStartTime) {
                numFlitSent++;
            }
            if (current_forward_msg->getIsTail() == true) {
                if (simTime().dbl() > RecordStartTime) {
                    numPackageSent++;
                }
            }

            if (Verbose >= VERBOSE_DETAIL_DEBUG_MESSAGES) {
                EV<<"BufferConnectCredit["<<vcid<<"]="<<BufferConnectCredit[vcid]<<"\n";
            }
        }

    }
//    if(channelAvailTime() > simTime()) {
//        scheduleAt(channelAvailTime(), selfMsgSendMsg);
//    } else {
//        scheduleAt(simTime()+CLK_CYCLE, selfMsgSendMsg);
//    }
    scheduleAt(std::max(simTime()+CLK_CYCLE,channelAvailTime()),selfMsgSendMsg);
}

void Processor::handleGenMsg()
{
//    if(getIndex() == 0){ //processor����msg��ģʽ,��Ҫ�Ľ�
    if (true) {

        //**********************����flit*****************************
        if(txQueue.getLength() + FlitLength <= ProcessorBufferDepth){ //Ҫ�����µ�Packet(head flit + body flit)��ͬʱbuffer���пռ����洢
            int bestVCID = generateBestVCID();
            for(int i = 0; i < FlitLength; i++) {
                DataPkt* msg = nullptr;

                if(i == 0) {
                    msg = generateMessage(true, false, FlitLength, i, bestVCID);
                } else if(i == FlitLength - 1) {
                    msg = generateMessage(false, true, FlitLength, i, bestVCID);
                } else {
                    msg = generateMessage(false, false, FlitLength, i, bestVCID);
                }

                txQueue.insert(msg);
                if(Verbose >= VERBOSE_DETAIL_DEBUG_MESSAGES) {
                    checkGenMsg(msg);
                }
                if (Verbose >= VERBOSE_DEBUG_MESSAGES) {
                    EV << "<<<<<<<<<<Processor: "<<getIndex()<<"("<<ppid2plid(getIndex())<<") is generating Flit>>>>>>>>>>\n";
                    EV << msg << endl;
                }
            }

        }else{//Ҫ�����µ�package������buffer�ռ䲻����drop����package
            if (simTime().dbl() > RecordStartTime) {
                numPktDropped++;
            }
            //���drop��һ��packet�ģ�����Ϊ1����������Ķ�ʱʱ��
            //����һ��ʱ��������Ϊ��ʱ��λ�������ǲ��ɻ������Ƶ�ʱ����
            dropFlag = true;
        }

        //**********************������ʱ��Ϣ*****************************
        //package֮���ʱ����Ϊ���ɷֲ�����ȷֲ�
        if(dropFlag == false){

#ifdef POISSON_DIST //���ɷֲ�
            double expTime = Poisson();
            if (Verbose >= VERBOSE_DETAIL_DEBUG_MESSAGES) {
                EV << "Poisson interval: "<<expTime<<"\n";
            }
            scheduleAt(simTime()+expTime,selfMsgGenMsg);
#else //���ȷֲ�
            double unitime = Uniform();
            if (Verbose >= VERBOSE_DETAIL_DEBUG_MESSAGES) {
                EV << "Uniform interval: "<<unitime<<"\n";
            }
            scheduleAt(simTime()+unitime,selfMsgGenMsg);
#endif

        }else{ //dropFlag == true
            scheduleAt(simTime() + CLK_CYCLE * FlitLength, selfMsgGenMsg);
            dropFlag = false;
        }
    }
}

void Processor::handleBufferInfoMsg(cMessage *msg)
{
    BufferInfoMsg *bufferInfoMsg = check_and_cast<BufferInfoMsg*>(msg);
    int from_port=bufferInfoMsg->getFrom_port();
    int vcid = bufferInfoMsg->getVcid();
    BufferConnectCredit[vcid]++; //increment credit count

    if (Verbose >= VERBOSE_DEBUG_MESSAGES) {
        EV<<"Receiving bufferInfoMsg >> PROCESSOR: "<<getIndex()<<"("<<ppid2plid(getIndex())<<"), INPORT: "<<from_port<<
            ", Received MSG: { "<<bufferInfoMsg<<" }\n";
        EV<<"BufferConnectCredit["<<vcid<<"]="<<BufferConnectCredit[vcid]<<"\n";
        EV<<"Msg transmition time: "<<simTime().dbl() - bufferInfoMsg->getTransmit_start() << "\n";
    }

    simtime_t credit_msg_delay = bufferInfoMsg->getArrivalTime() - bufferInfoMsg->getCreationTime();
    if (simTime().dbl() > RecordStartTime) {
//                creditMsgDelayTime.record(credit_msg_delay.dbl());
        creditMsgDelayTimeTotal += credit_msg_delay.dbl();
        creditMsgDelayTimeCount += 1;
    }

    delete bufferInfoMsg;
}

void Processor::handleDataPkt(cMessage *msg)
{
    DataPkt* datapkt = check_and_cast<DataPkt*>(msg);
    if(datapkt->getIsHead()) {
        int dest_id = datapkt->getDst_ppid();
        int index = getIndex();
        if(dest_id != index) {
            if (Verbose >= VERBOSE_DEBUG_MESSAGES) {
                EV << ">>>>>>>>>>Message {" << datapkt << " } arrived at wrong destination at node " <<index<<"("<<ppid2plid(index)<<")<<<<<<<<<<\n";
                assert(dest_id == index);
            }
        }
    }


    // Message arrived
    // ������·����ĵ�λ��flit����packet�����Ի���ֲ�ͬ��ͨ����body flit��������
    int current_ppid = getIndex();
    int hopcount = datapkt->getHopCount();
    if (Verbose >= VERBOSE_DEBUG_MESSAGES) {
        EV << ">>>>>>>>>>Message {" << datapkt << " } arrived after " << hopcount <<
                " hops at node "<<current_ppid<<"("<<ppid2plid(current_ppid)<<")<<<<<<<<<<\n";
        EV<<"Msg transmition time: "<<simTime().dbl() - datapkt->getTransmit_start() << "\n";
    }

    // update statistics.
    if (simTime().dbl() > RecordStartTime) {
        numFlitReceived++;
        flitByHop += hopcount + 1; //�������һ��·������processor
    }

//            if (datapkt->getIsHead() == true) {
//                packageDelayCount = datapkt->getFlitCount();
//                headFlitGenTime = datapkt->getCreationTime().dbl(); //��ʱ�ı�ʾ�����Ӳ�����Flit����Flit��Ŀ��ڵ���գ����а����ڷ��Ͷ�txQueue�ĵȴ�ʱ��
//            }
//            packageDelayCount--;
//            if(Verbose >= VERBOSE_DETAIL_DEBUG_MESSAGES) {
//                EV << "In processor " << getIndex() << "(" << ppid2plid(getIndex()) << "), packageDelayCount = " << packageDelayCount << endl;
//            }
//            if (packageDelayCount == 0) {
//                packageDelayTime.record(simTime().dbl() - headFlitGenTime);
//                numPackageReceived++;
//            }
    if(datapkt->getIsTail() == true) {
        if (simTime().dbl() > RecordStartTime) {
            numPackageReceived++;
//                    packageDelayTime.record(simTime().dbl() - datapkt->getPackageGenTime());
            packetDelayTimeTotal += simTime().dbl() - datapkt->getPackageGenTime();
            packetDelayTimeCount += 1;
        }

    }
    if (simTime().dbl() > RecordStartTime) {
//                flitDelayTime.record(simTime().dbl() - datapkt->getCreationTime());
        flitDelayTimeTotal += simTime().dbl() - datapkt->getCreationTime().dbl();
        flitDelayTimeCount += 1;
//                hopCountVector.record(hopcount);
        hopCountTotal += hopcount;
        hopCountCount += 1;
    }

    delete datapkt;
}


void Processor::checkGenMsg(DataPkt* datapkt) {
    EV << "From Processor: " << getIndex() << "(" << ppid2plid(getIndex()) << "), flitCount = " <<datapkt->getFlitCount()
            << ", src_ppid = " << datapkt->getSrc_ppid() << ", dst_ppid = " << datapkt->getDst_ppid()
            << ", packetGenTime = " << datapkt->getPackageGenTime() << ", isHead = " << datapkt->getIsHead()
            << ", isTail = " << datapkt->getIsTail() << ", getVcid = " << datapkt->getVc_id()
            << ", getHopCount = " <<datapkt->getHopCount() << ", fromPort = " << datapkt->getFrom_router_port() << endl;

}

int Processor::generateBestVCID() {
    //int vc_id = intuniform(0,VC-1); //�������vcͨ��, ������һ��router��vc buffer�����ѡ����ʵ�vcid
    int best_vcid = 0;
    for(int i = 0; i < VC; i++){
        if(BufferConnectCredit[i] > BufferConnectCredit[best_vcid]){
            best_vcid = i;
        }
    }
    return best_vcid;
}


DataPkt* Processor::generateMessage(bool isHead, bool isTail, int flitCount, int flitID, int vcid)
{

    if(isHead){
        // Head Flit

        // Produce source and destination address
        int current_ppid = getIndex();

        int dst_ppid = -1;
        switch(traffic) {
            case UNI: dst_ppid = uni(); break;
            case HOTSPOT: dst_ppid = hotspot(); break;
            case TRANSPOSE: dst_ppid = transpose(); break;
            case COMPLEMENT: dst_ppid = complement(); break;
            case BITREVERSAL: dst_ppid = bitreversal(); break;
            default: assert(false);
        }
        assert(dst_ppid != -1);

        int current_plid = ppid2plid(current_ppid);
        int dst_plid = ppid2plid(dst_ppid);

        char msgname[300];//��ʼ����Ŀռ�̫С�������ݱ��ı�!!!!!!!
        sprintf(msgname, "Head Flit, From processor node %d(%d) to node %d(%d), Flit Length: %d, Flit No: %d", current_ppid,current_plid,dst_ppid,dst_plid,flitCount,flitID+1);

        // Create message object and set source and destination field.
        DataPkt *msg = new DataPkt(msgname);
        msg->setSrc_ppid(current_ppid);//���÷�����processor���
        msg->setDst_ppid(dst_ppid);//���ý��յ�processor���
        msg->setFrom_router_port(getNextRouterPortP());//�����յ���msg��Router�˿�
        msg->setVc_id(vcid);//����VCID
        msg->setIsHead(isHead); //����isHead flag
        msg->setIsTail(isTail);
        msg->setFlitCount(flitCount); // ����Flit Count
        msg->setPackageGenTime(simTime().dbl());
        msg->setByteLength(FlitSize);

        if (Verbose >= VERBOSE_DETAIL_DEBUG_MESSAGES) {
            EV<<"From Processor::generateMessage, flit count: "<<msg->getFlitCount()<<", FlitLength: "<<flitCount<<"\n";
        }
        //EV<<current_ppid<<" "<<dst_ppid<<"\n";
        //EV<<msg->getSrc_ppid()<<" "<<msg->getDst_ppid()<<"\n";

        return msg;
    }else{

        char msgname[300];//��ʼ����Ŀռ�̫С�������ݱ��ı�!!!!!!!
        sprintf(msgname, "Body/Tail Flit, Flit Length: %d, Flit No: %d", flitCount, flitID+1);

        // Create message object and set source and destination field.
        DataPkt *msg = new DataPkt(msgname);
        msg->setSrc_ppid(-1);//���÷�����processor���
        msg->setDst_ppid(-1);//���ý��յ�processor���
        msg->setFrom_router_port(getNextRouterPortP());//�����յ���msg��Router�˿�
        msg->setVc_id(vcid);//����VC ID
        msg->setIsHead(isHead); //����isHead flag
        msg->setIsTail(isTail);
        msg->setFlitCount(-1);
        msg->setPackageGenTime(simTime().dbl());
        msg->setByteLength(FlitSize);
        return msg;

    }
}

//�ж�ͨ���Ƿ���У����Դ�������
simtime_t Processor::channelAvailTime(){
    cChannel* txChannel = gate("port$o")->getTransmissionChannel();
    simtime_t txFinishTime = txChannel->getTransmissionFinishTime();
    return txFinishTime;
}

void Processor::calcInputBufferOccupancy()
{
    if(simTime().dbl() > RecordStartTime) {
        for(int i = 0; i < VC; i++) {
            inputBufferOccupancy += 1.0 * BufferDepth - BufferConnectCredit[i];
            if(BufferConnectCredit[i] == 0) {
                ++inputBufferFullTimes;
            } else if(BufferConnectCredit[i] == BufferDepth) {
                ++inputBufferEmptyTimes;
            }
        }
    }
}

//processorת����·���㷨,processorֻ��һ��prot,ֱ��ת����ȥ����
void Processor::forwardMessage(DataPkt *msg)
{

    msg->setFrom_router_port(getNextRouterPortP());// �����յ�����Ϣ·�����Ķ˿ں�
    msg->setTransmit_start(simTime().dbl());
    send(msg,"port$o");
    if (Verbose >= VERBOSE_DEBUG_MESSAGES) {
        EV << "Forwarding message { " << msg << " } from processor "<<getIndex()<<"("<<ppid2plid(getIndex())<<") to router, VCID = "<<msg->getVc_id()<<"\n";
    }

}


//double Processor::ParetoON() {
//    double exp_time = exponential((double)1/ALPHA_ON);
//    exp_time = round(exp_time / TimeScale) * CLK_CYCLE;
//    if(exp_time < CLK_CYCLE) {
//        exp_time = CLK_CYCLE;
//    }
//    return exp_time;
//}
//
//double Processor::ParetoOFF() {
//    double exp_time = exponential((double)1/ALPHA_OFF);
//    exp_time = round(exp_time / TimeScale) * CLK_CYCLE;
//    if(exp_time < CLK_CYCLE) {
//        exp_time = CLK_CYCLE;
//    }
//    return exp_time;
//}

double Processor::Poisson() {
    double exp_time = exponential((double)1.0/LAMBDA);
//    exp_time = round(exp_time / TimeScale) * CLK_CYCLE * FlitLength;
    exp_time = (exp_time / TimeScale) * CLK_CYCLE * FlitLength;
    if(exp_time < CLK_CYCLE) {
        exp_time = CLK_CYCLE;
    }
    return exp_time; //����һ���Բ���FlitLength��Flit�����ʱ����ΪFlitLength�ı���������txQueue�ᱬ
}

double Processor::Uniform() {
//    double time = round(1.0 / INJECTION_RATE) * CLK_CYCLE * FlitLength;
    double time = (1.0 / INJECTION_RATE) * CLK_CYCLE * FlitLength;
    if(time < CLK_CYCLE) {
        time = CLK_CYCLE;
    }
    return time;
}

void Processor::finish()
{
    // This function is called by OMNeT++ at the end of the simulation.
    EV << "Flit Sent: " << numFlitSent << endl;
    EV << "Package Sent: " << numPackageSent << endl;
    EV << "Flit Received: " << numFlitReceived << endl;
    EV << "Package Received: " << numPackageReceived << endl;
    EV << "Dropped:  " << numPktDropped << endl;

    recordScalar("flitSent", numFlitSent);
    recordScalar("packageSent", numPackageSent);
    recordScalar("flitReceived", numFlitReceived);
    recordScalar("packageReceived", numPackageReceived);
    recordScalar("packetDropped", numPktDropped);
    recordScalar("flitByHop", flitByHop);

    recordScalar("hopCountTotal", hopCountTotal);
    recordScalar("hopCountCount", hopCountCount);
    recordScalar("flitDelayTimeTotal", flitDelayTimeTotal);
    recordScalar("flitDelayTimeCount", flitDelayTimeCount);
    recordScalar("packetDelayTimeTotal", packetDelayTimeTotal);
    recordScalar("packetDelayTimeCount", packetDelayTimeCount);
    recordScalar("creditMsgDelayTimeTotal", creditMsgDelayTimeTotal);
    recordScalar("creditMsgDelayTimeCount", creditMsgDelayTimeCount);

    double timeCount = (simTime().dbl() - RecordStartTime) / (CLK_CYCLE);

    double totalInputBufferOccupancy = 1.0 * VC * BufferDepth * timeCount;
    inputBufferOccupancy = inputBufferOccupancy / totalInputBufferOccupancy;
    recordScalar("processorInputBufferOccupancy", inputBufferOccupancy);
    recordScalar("processorInputBufferFullTimes", inputBufferFullTimes / (timeCount * VC));
    recordScalar("processorInputBufferEmptyTimes", inputBufferEmptyTimes / (timeCount * VC));
    recordScalar("channelUnavailTimes", channelUnavailTimes);

    if(getIndex() == 0) {
        recordScalar("timeCount", timeCount);
        recordScalar("processorNum", ProcessorNum);
        recordScalar("flitLength", FlitLength);
    }

}

int Processor::uni() {
    // Produce source and destination address
    int current_ppid = getIndex();
    int n = getVectorSize();//processor������
    int dst_ppid = intuniform(0, n-2); //��������ģ��
    if (dst_ppid >= current_ppid)
        dst_ppid++;//��֤��ȡ��current_ppid
    return dst_ppid;

}

int Processor::hotspot() {
    //Returns a random variate with uniform distribution in the range [a,b).
    double unif = uniform(0.0, 10.0);
    int ret = 0;
    if(Hotspot > unif) {
        if(hotspotIndex == -1) {
            hotspotIndex = uni();
        }
        ret = hotspotIndex;
    } else {
        ret = uni();
    }
    return ret;
}

int Processor::transpose() {
    int n = getIndex();
    int len = (int) (log(ProcessorNum) / log(2));
    int mask = ProcessorNum - 1;
    int rightshift = len - len / 2;
    int leftshift = len / 2;
    int ret = ((n >> rightshift) | (n << leftshift)) & mask;
    return ret;
}

int Processor::complement() {
    int n = getIndex();
    int mask = ProcessorNum - 1;
    int ret = (~n) & mask;
    return ret;
}

int Processor::bitreversal() {
    int n = getIndex();
    int len = (int) (log(ProcessorNum) / log(2));
    int ret = 0;
    for (; n; n >>= 1) {
        ret = ret << 1;
        ret |= n & 1;
        len--;
    }
    ret = ret << len;
    return ret;

}
