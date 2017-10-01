/*
 * ft_router.cc
 *
 *  Created on: 2017��4��26��
 *      Author: Vincent
 */


#include "ft_router.h"


//�����յ���msg��·�����Ķ˿ں�
int FtRouter::getNextRouterPort(int current_out_port){


    int cur_swpid=getIndex();//��ǰ·������id
    int cur_swlid=swpid2swlid(cur_swpid);
    int level=cur_swlid/pow(100,LevelNum-1);//Router��level
    bool lowerRouter = (current_out_port>=(PortNum/2))&&(level!=LevelNum-1); //�ж��Ƿ�����ת��������ת����Ϊ�²�router

    int ctmp=(cur_swlid%((int)pow(100,LevelNum-1)));//ȥ����level��swlid
    int k;//��һ��Router�Ľ��ܶ˿�
    if(!lowerRouter){//�ϲ��Router
        if(level==0){
            k=0;//level==0ʱ��Ϊ�϶˿ڣ����msg����processor����processorֻ��һ���˿ڣ�Ĭ��processor�Ľ��ն˿�Ϊ0
        }else{
            int lowLevel=level-1;//�²��level
            k = (ctmp/((int) pow(100,lowLevel)))%100+PortNum/2;
        }

    }else{ //�²��Router
        k = (ctmp/((int) pow(100,level)))%100;
    }
    return k;
}

bool FtRouter::connectToProcessor(int port_num) {
    int cur_swpid=getIndex();//��ǰ·������id
    int cur_swlid=swpid2swlid(cur_swpid);
    int level=cur_swlid/pow(100,LevelNum-1);//Router��level
    if (level == 0 && port_num < PortNum/2)
        return true;
    return false;
}

//��ppid����plid
int FtRouter::ppid2plid(int ppid){
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
int FtRouter::plid2ppid(int plid){
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

//��swpid����swlid
int FtRouter::swpid2swlid(int swpid){
    //�����ж�swpid����һ��
    int level=0;
    bool find_level = false;
    for(int i = 0; i < LevelNum - 1; i++){
        if(swpid >= i * SwLowEach and swpid < (i + 1) * SwLowEach){
            level = i;
            find_level = true;
            break;
        }
    }
    if(!find_level)
        level = LevelNum - 1;
    //�Ѿ��ҵ�switch���ڲ㣬������������б���
    //�ȶԷǶ����switch���б���
    if(level < LevelNum - 1){
        int tmp = swpid - level * SwLowEach;
        int IDtmp = 0;
        int mul = 1;
        for(int i = 0; i < LevelNum - 2; i++){
            IDtmp = mul * (tmp % (PortNum/2))+IDtmp;
            tmp = (int) (tmp / (PortNum / 2));
            mul = mul * 100;
        }
        IDtmp = IDtmp + mul * tmp;
        mul = mul * 100;
        IDtmp = mul * level + IDtmp;//��ǰ��������Ĳ���
        return IDtmp;
    }
    //�������Զ����switch���в���
    else{
        int tmp = swpid;
        int IDtmp = 0;
        int mul = 1;
        for(int i = 0; i < LevelNum - 1; i++){
            IDtmp = mul * (tmp % (PortNum / 2)) + IDtmp;
            tmp = (int) (tmp / (PortNum / 2));
            mul = mul * 100;
        }
        IDtmp = mul * level + IDtmp;
        return IDtmp;
    }
}

//swlidתswpid
int FtRouter::swlid2swpid(int swlid){
    int tmp = swlid;
    int level = tmp / (pow(100, (LevelNum - 1)));
    tmp = tmp % ((int)pow(100, (LevelNum-1)));
    int IDtmp = level * SwLowEach;
    int mul = 1;
    for(int i = 0; i < LevelNum - 1; i++){
        IDtmp = IDtmp + mul * (tmp % 100);
        mul = mul * (PortNum / 2);
        tmp = tmp / 100;
    }
    return IDtmp;
}

//���ݵ�ǰrouter��swpid��msg��dst_ppid������ת���Ķ˿�
int FtRouter::calRoutePort(DataPkt *msg){
    int cur_swpid = getIndex();//��ǰ·������id
    int cur_swlid = swpid2swlid(cur_swpid);
    int level = cur_swlid / pow(100,LevelNum-1);//Router��level
    int dst_ppid = msg->getDst_ppid();
    int dst_plid = ppid2plid(dst_ppid);
    //EV<<dst_ppid<<" "<<dst_plid<<"\n";
    //�ж�switch�Ƿ�Ϊ����
    int ptmp = dst_plid / pow(100,level+1);//
    int ctmp = (cur_swlid%((int)pow(100,LevelNum-1)))/pow(100,level);
    bool isAncestor = (ptmp == ctmp);
    int k;//ת���Ķ˿�
    //EV<<cur_swpid<<" "<<cur_swlid<<" "<<level<<" "<<dst_ppid<<" "<<dst_plid<<" "<<ptmp<<" "<<ctmp<<"\n";
    //���switch��dst_ppid�����ȣ������¶˿�ת�����������϶˿�ת��
    if(isAncestor){
        //����ת��
        k=(dst_plid/((int)pow(100,level)))%100;//ͨ���˿�pl������ת��
        //EV<<"isAncestor, k="<<k<<"\n";
        return k;
    }else{
        //����ת��
        k=(dst_plid/((int)pow(100,level)))%100+PortNum/2;//k=pl��+m/2
        //EV<<"notAncestor, k="<<k<<" "<<dst_ppid<<" "<<dst_plid<<" "<<(int)pow(10,level)<<" "<<(dst_plid/((int)pow(10,level)))%10<<"\n";
        return k;
    }

}


