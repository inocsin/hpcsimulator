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

Processor::Processor(){
    selfMsgGenMsg=nullptr;
    selfMsgSendMsg=nullptr;

}

Processor::~Processor(){
    cancelAndDelete(selfMsgGenMsg);
    cancelAndDelete(selfMsgSendMsg);

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

//    hopCountVector.setName("hopCount");
//    flitDelayTime.setName("flitDelayTime");
//    packageDelayTime.setName("packageDelayTime");
//    creditMsgDelayTime.setName("creditMsgDelayTime");

    selfMsgSendMsg = new cMessage("selfMsgSendMsg");//ע��˳���ȷ���buffer�����msg���ٲ����µ�msg������һ��flit��Ҫ2�����ڲŻᷢ��ȥ
    scheduleAt(Sim_Start_Time, selfMsgSendMsg);
    selfMsgGenMsg = new cMessage("selfMsgGenMsg");
    scheduleAt(Sim_Start_Time, selfMsgGenMsg);

    for (int i = 0; i < VC; i++) {
        BufferConnectCredit[i] = BufferDepth; //��ʼ������������router��buffer��Ϊ��
    }


    dropFlag = false;


}

void Processor::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        //********************���������ݵ��Զ�ʱ��Ϣ********************
        if(msg == selfMsgSendMsg) {
            //****************************ת��flit**************************
            if(!txQueue.isEmpty()){ //���Ͷ���������
                DataPkt* current_forward_msg = (DataPkt*) txQueue.front();
                int vcid = current_forward_msg->getVc_id();
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

            scheduleAt(std::max(simTime()+CLK_CYCLE,channelAvailTime()),selfMsgSendMsg);

        }else if(msg == selfMsgGenMsg){

//            if(getIndex() == 0){ //processor����msg��ģʽ,��Ҫ�Ľ�
            if (true) {

                //**********************����flit*****************************
                if(txQueue.getLength() + FlitLength <= ProcessorBufferDepth){ //Ҫ�����µ�Packet(head flit + body flit)��ͬʱbuffer���пռ����洢
                    int bestVCID = generateBestVCID();
                    for(int i = 0; i < FlitLength; i++) {
                        if(i == 0) {
                            DataPkt* msg = generateMessage(true, false, FlitLength, i, bestVCID);
                            txQueue.insert(msg);
                            if(Verbose >= VERBOSE_DETAIL_DEBUG_MESSAGES) {
                                checkGenMsg(msg);
                            }
                            if (Verbose >= VERBOSE_DEBUG_MESSAGES) {
                                EV << "<<<<<<<<<<Processor: "<<getIndex()<<"("<<ppid2plid(getIndex())<<") is generating Head Flit>>>>>>>>>>\n";
                                EV << msg << endl;
                            }
                        } else if(i == FlitLength - 1) {
                            DataPkt* msg = generateMessage(false, true, FlitLength, i, bestVCID);
                            txQueue.insert(msg);
                            if(Verbose >= VERBOSE_DETAIL_DEBUG_MESSAGES) {
                                checkGenMsg(msg);
                            }
                            if (Verbose >= VERBOSE_DEBUG_MESSAGES) {
                                EV << "<<<<<<<<<<Processor: "<<getIndex()<<"("<<ppid2plid(getIndex())<<") is generating Tail Flit>>>>>>>>>>\n";
                                EV << msg << endl;
                            }
                        } else {
                            DataPkt* msg = generateMessage(false, false, FlitLength, i, bestVCID);
                            txQueue.insert(msg);
                            if(Verbose >= VERBOSE_DETAIL_DEBUG_MESSAGES) {
                                checkGenMsg(msg);
                            }
                            if (Verbose >= VERBOSE_DEBUG_MESSAGES) {
                                EV << "<<<<<<<<<<Processor: "<<getIndex()<<"("<<ppid2plid(getIndex())<<") is generating Body Flit>>>>>>>>>>\n";
                                EV << msg << endl;
                            }
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


    }else{
        //************************��self message*********************
        //************************�յ�buffer������Ϣ******************
        if(strcmp("bufferInfoMsg", msg->getName()) == 0){

            BufferInfoMsg *bufferInfoMsg = check_and_cast<BufferInfoMsg*>(msg);
            int from_port=bufferInfoMsg->getFrom_port();
            int vcid = bufferInfoMsg->getVcid();
            BufferConnectCredit[vcid]++; //increment credit count

            if (Verbose >= VERBOSE_DEBUG_MESSAGES) {
                EV<<"Receiving bufferInfoMsg >> PROCESSOR: "<<getIndex()<<"("<<ppid2plid(getIndex())<<"), INPORT: "<<from_port<<
                    ", Received MSG: { "<<bufferInfoMsg<<" }\n";
                EV<<"BufferConnectCredit["<<vcid<<"]="<<BufferConnectCredit[vcid]<<"\n";
            }

            simtime_t credit_msg_delay = bufferInfoMsg->getArrivalTime() - bufferInfoMsg->getCreationTime();
            if (simTime().dbl() > RecordStartTime) {
//                creditMsgDelayTime.record(credit_msg_delay.dbl());
                creditMsgDelayTimeTotal += credit_msg_delay.dbl();
                creditMsgDelayTimeCount += 1;
            }



            delete bufferInfoMsg;

        }else{
        //***********************�յ�DataPkt��Ϣ***********************
            DataPkt* datapkt = check_and_cast<DataPkt*>(msg);

            // Message arrived
            // ������·����ĵ�λ��flit����packet�����Ի���ֲ�ͬ��ͨ����body flit��������
            int current_ppid = getIndex();
            int hopcount = datapkt->getHopCount();
            if (Verbose >= VERBOSE_DEBUG_MESSAGES) {
                EV << ">>>>>>>>>>Message {" << datapkt << " } arrived after " << hopcount <<
                        " hops at node "<<current_ppid<<"("<<ppid2plid(current_ppid)<<")<<<<<<<<<<\n";
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

    }
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
        int n = getVectorSize();//processor������
        if (Verbose >= VERBOSE_DETAIL_DEBUG_MESSAGES) {
            EV<<"There are "<< n << " processors"<<"\n";
        }
        //EV<<n<<"\n";
#ifdef UNIFORM //���ȷֲ�
        int dst_ppid = intuniform(0, n-2); //��������ģ��
        //EV<<dst_ppid<<"\n";
        if (dst_ppid >= current_ppid)
            dst_ppid++;//��֤��ȡ��current_ppid
#endif

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

//processorת����·���㷨,processorֻ��һ��prot,ֱ��ת����ȥ����
void Processor::forwardMessage(DataPkt *msg)
{

    msg->setFrom_router_port(getNextRouterPortP());// �����յ�����Ϣ·�����Ķ˿ں�
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



    if(getIndex() == 0) {
        double timeCount = (simTime().dbl() - RecordStartTime) / (CLK_CYCLE);
        recordScalar("timeCount", timeCount);
    }

}

