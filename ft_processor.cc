/*
 * ft_processor.cc
 *
 *  Created on: 2017��4��26��
 *      Author: Vincent
 */

#include "processor.h"
#include "fat_tree_topo.h"

class FtProcessor : public Processor {
protected:
    //���麯����������ʵ��
    virtual int ppid2plid(int ppid) override;
    virtual int plid2ppid(int plid) override;
    virtual int getNextRouterPortP() override; //������processor������router�Ķ˿�
};

Define_Module(FtProcessor);

int FtProcessor::getNextRouterPortP(){
    int current_ppid=getIndex();
    int plid=ppid2plid(current_ppid);
    int port=plid%100;
    return port; //���غ�processor������router�Ķ˿�
}


//��ppid����plid
int FtProcessor::ppid2plid(int ppid){
    int idtmp = ppid;
    int idfinal = 0;
    int mul = 1;
    for(int i = 0; i < LevelNum - 1; i++){
        idfinal = idfinal + idtmp % (PortNum / 2)*mul;
        mul = mul * 100;
        idtmp = (int) (idtmp / (PortNum / 2));
    }
    idfinal = idfinal + idtmp * mul;
    return idfinal;
}

//��plid����ppid
int FtProcessor::plid2ppid(int plid){
    int tmp = plid;
    int mul = 1;
    int IDtmp = 0;
    for(int i = 0; i < LevelNum; i++){
        IDtmp = IDtmp + mul * (tmp % 100);
        mul = mul * (PortNum / 2);
        tmp = tmp / 100;
    }
    return IDtmp;
}
