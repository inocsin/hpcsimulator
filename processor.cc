/*
 * processor.cc
 *
 *  Created on: 2016年7月30日
 *      Author: Vincent
 *
 *  Function:
 *      Flit模式：Head Flit和Body Flit
 *      时间分布：自相似(FlitLength不定长)，泊松(FlitLength定长)
 *      空间分布：均匀
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

    selfMsgSendMsg = new cMessage("selfMsgSendMsg");//注意顺序，先发送buffer里面的msg，再产生新的msg，这样一个flit需要2个周期才会发出去
    scheduleAt(Sim_Start_Time, selfMsgSendMsg);
    selfMsgGenMsg = new cMessage("selfMsgGenMsg");
    scheduleAt(Sim_Start_Time, selfMsgGenMsg);

    for (int i = 0; i < VC; i++) {
        BufferConnectCredit[i] = BufferDepth; //初始化与它相连的router的buffer都为空
    }


    dropFlag = false;


}

void Processor::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        //********************发送新数据的自定时消息********************
        if(msg == selfMsgSendMsg) {
            //****************************转发flit**************************
            if(!txQueue.isEmpty()){ //发送队列有数据
                DataPkt* current_forward_msg = (DataPkt*) txQueue.front();
                int vcid = current_forward_msg->getVc_id();
                if(channelAvailTime() <= simTime() && BufferConnectCredit[vcid] != 0) { //发送端口空闲，下一个节点有buffer接受此flit
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

//            if(getIndex() == 0){ //processor产生msg的模式,需要改进
            if (true) {

                //**********************产生flit*****************************
                if(txQueue.getLength() + FlitLength <= ProcessorBufferDepth){ //要产生新的Packet(head flit + body flit)，同时buffer又有空间来存储
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

                }else{//要产生新的package，但是buffer空间不够，drop掉该package
                    if (simTime().dbl() > RecordStartTime) {
                        numPktDropped++;
                    }
                    //如果drop了一个packet的，则置为1，进入下面的定时时，
                    //会以一个时钟周期作为定时单位，而不是泊松或自相似的时间间隔
                    dropFlag = true;

                }

                //**********************产生定时消息*****************************
                //package之间的时间间隔为泊松分布或均匀分布
                if(dropFlag == false){

#ifdef POISSON_DIST //泊松分布
                    double expTime = Poisson();
                    if (Verbose >= VERBOSE_DETAIL_DEBUG_MESSAGES) {
                        EV << "Poisson interval: "<<expTime<<"\n";
                    }
                    scheduleAt(simTime()+expTime,selfMsgGenMsg);
#else //均匀分布
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
        //************************非self message*********************
        //************************收到buffer更新消息******************
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
        //***********************收到DataPkt消息***********************
            DataPkt* datapkt = check_and_cast<DataPkt*>(msg);

            // Message arrived
            // 由于链路分配的单位是flit而非packet，所以会出现不同虚通道的body flit接连到达
            int current_ppid = getIndex();
            int hopcount = datapkt->getHopCount();
            if (Verbose >= VERBOSE_DEBUG_MESSAGES) {
                EV << ">>>>>>>>>>Message {" << datapkt << " } arrived after " << hopcount <<
                        " hops at node "<<current_ppid<<"("<<ppid2plid(current_ppid)<<")<<<<<<<<<<\n";
            }

            // update statistics.
            if (simTime().dbl() > RecordStartTime) {
                numFlitReceived++;
                flitByHop += hopcount + 1; //包含最后一跳路由器到processor
            }

//            if (datapkt->getIsHead() == true) {
//                packageDelayCount = datapkt->getFlitCount();
//                headFlitGenTime = datapkt->getCreationTime().dbl(); //延时的表示方法从产生该Flit到该Flit被目标节点接收，其中包括在发送端txQueue的等待时间
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
    //int vc_id = intuniform(0,VC-1); //随机分配vc通道, 根据下一跳router的vc buffer情况来选择合适的vcid
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
        int n = getVectorSize();//processor的数量
        if (Verbose >= VERBOSE_DETAIL_DEBUG_MESSAGES) {
            EV<<"There are "<< n << " processors"<<"\n";
        }
        //EV<<n<<"\n";
#ifdef UNIFORM //均匀分布
        int dst_ppid = intuniform(0, n-2); //均匀流量模型
        //EV<<dst_ppid<<"\n";
        if (dst_ppid >= current_ppid)
            dst_ppid++;//保证不取到current_ppid
#endif

        int current_plid = ppid2plid(current_ppid);
        int dst_plid = ppid2plid(dst_ppid);

        char msgname[300];//初始分配的空间太小导致数据被改变!!!!!!!
        sprintf(msgname, "Head Flit, From processor node %d(%d) to node %d(%d), Flit Length: %d, Flit No: %d", current_ppid,current_plid,dst_ppid,dst_plid,flitCount,flitID+1);

        // Create message object and set source and destination field.
        DataPkt *msg = new DataPkt(msgname);
        msg->setSrc_ppid(current_ppid);//设置发出的processor编号
        msg->setDst_ppid(dst_ppid);//设置接收的processor编号
        msg->setFrom_router_port(getNextRouterPortP());//设置收到该msg的Router端口
        msg->setVc_id(vcid);//设置VCID
        msg->setIsHead(isHead); //设置isHead flag
        msg->setIsTail(isTail);
        msg->setFlitCount(flitCount); // 设置Flit Count
        msg->setPackageGenTime(simTime().dbl());
        msg->setByteLength(FlitSize);

        if (Verbose >= VERBOSE_DETAIL_DEBUG_MESSAGES) {
            EV<<"From Processor::generateMessage, flit count: "<<msg->getFlitCount()<<", FlitLength: "<<flitCount<<"\n";
        }
        //EV<<current_ppid<<" "<<dst_ppid<<"\n";
        //EV<<msg->getSrc_ppid()<<" "<<msg->getDst_ppid()<<"\n";

        return msg;
    }else{

        char msgname[300];//初始分配的空间太小导致数据被改变!!!!!!!
        sprintf(msgname, "Body/Tail Flit, Flit Length: %d, Flit No: %d", flitCount, flitID+1);

        // Create message object and set source and destination field.
        DataPkt *msg = new DataPkt(msgname);
        msg->setSrc_ppid(-1);//设置发出的processor编号
        msg->setDst_ppid(-1);//设置接收的processor编号
        msg->setFrom_router_port(getNextRouterPortP());//设置收到该msg的Router端口
        msg->setVc_id(vcid);//设置VC ID
        msg->setIsHead(isHead); //设置isHead flag
        msg->setIsTail(isTail);
        msg->setFlitCount(-1);
        msg->setPackageGenTime(simTime().dbl());
        msg->setByteLength(FlitSize);
        return msg;

    }
}

//判断通道是否空闲，可以传输数据
simtime_t Processor::channelAvailTime(){
    cChannel* txChannel = gate("port$o")->getTransmissionChannel();
    simtime_t txFinishTime = txChannel->getTransmissionFinishTime();
    return txFinishTime;

}

//processor转发的路由算法,processor只有一个prot,直接转发出去即可
void Processor::forwardMessage(DataPkt *msg)
{

    msg->setFrom_router_port(getNextRouterPortP());// 设置收到该信息路由器的端口号
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
    return exp_time; //由于一次性产生FlitLength个Flit，因此时间间隔为FlitLength的倍数，否则txQueue会爆
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

