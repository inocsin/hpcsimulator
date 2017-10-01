/*
 * ft_router.cc
 *
 *  Created on: 2017年4月26日
 *      Author: Vincent
 */


#include "ft_router.h"


//计算收到该msg的路由器的端口号
int FtRouter::getNextRouterPort(int current_out_port){


    int cur_swpid=getIndex();//当前路由器的id
    int cur_swlid=swpid2swlid(cur_swpid);
    int level=cur_swlid/pow(100,LevelNum-1);//Router的level
    bool lowerRouter = (current_out_port>=(PortNum/2))&&(level!=LevelNum-1); //判断是否向上转发，向上转发则为下层router

    int ctmp=(cur_swlid%((int)pow(100,LevelNum-1)));//去除掉level的swlid
    int k;//下一个Router的接受端口
    if(!lowerRouter){//上层的Router
        if(level==0){
            k=0;//level==0时，为上端口，因此msg发到processor，而processor只有一个端口，默认processor的接收端口为0
        }else{
            int lowLevel=level-1;//下层的level
            k = (ctmp/((int) pow(100,lowLevel)))%100+PortNum/2;
        }

    }else{ //下层的Router
        k = (ctmp/((int) pow(100,level)))%100;
    }
    return k;
}

bool FtRouter::connectToProcessor(int port_num) {
    int cur_swpid=getIndex();//当前路由器的id
    int cur_swlid=swpid2swlid(cur_swpid);
    int level=cur_swlid/pow(100,LevelNum-1);//Router的level
    if (level == 0 && port_num < PortNum/2)
        return true;
    return false;
}

//从ppid计算plid
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

//从plid计算ppid
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

//从swpid计算swlid
int FtRouter::swpid2swlid(int swpid){
    //首先判断swpid在哪一层
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
    //已经找到switch所在层，接下来对其进行编码
    //先对非顶层的switch进行编码
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
        IDtmp = mul * level + IDtmp;//最前面加上它的层数
        return IDtmp;
    }
    //接下来对顶层的switch进行操作
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

//swlid转swpid
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

//根据当前router的swpid和msg的dst_ppid来计算转发的端口
int FtRouter::calRoutePort(DataPkt *msg){
    int cur_swpid = getIndex();//当前路由器的id
    int cur_swlid = swpid2swlid(cur_swpid);
    int level = cur_swlid / pow(100,LevelNum-1);//Router的level
    int dst_ppid = msg->getDst_ppid();
    int dst_plid = ppid2plid(dst_ppid);
    //EV<<dst_ppid<<" "<<dst_plid<<"\n";
    //判断switch是否为祖先
    int ptmp = dst_plid / pow(100,level+1);//
    int ctmp = (cur_swlid%((int)pow(100,LevelNum-1)))/pow(100,level);
    bool isAncestor = (ptmp == ctmp);
    int k;//转发的端口
    //EV<<cur_swpid<<" "<<cur_swlid<<" "<<level<<" "<<dst_ppid<<" "<<dst_plid<<" "<<ptmp<<" "<<ctmp<<"\n";
    //如果switch是dst_ppid的祖先，则向下端口转发，否则向上端口转发
    if(isAncestor){
        //向下转发
        k=(dst_plid/((int)pow(100,level)))%100;//通过端口pl’进行转发
        //EV<<"isAncestor, k="<<k<<"\n";
        return k;
    }else{
        //向上转发
        k=(dst_plid/((int)pow(100,level)))%100+PortNum/2;//k=pl’+m/2
        //EV<<"notAncestor, k="<<k<<" "<<dst_ppid<<" "<<dst_plid<<" "<<(int)pow(10,level)<<" "<<(dst_plid/((int)pow(10,level)))%10<<"\n";
        return k;
    }

}


