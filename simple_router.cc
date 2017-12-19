/*
 * simple_router.cc
 *
 *  Created on: 2017年5月7日
 *      Author: Vincent
 */

#include "high_radix_router.h"

//SimpleRouter
//单个路由器，所有端口连接processor，测试路由器性能

//public Router, FtRouter, HighRadixRouter
class SimpleRouter : public HighRadixRouter {

protected:
    //纯虚函数，根据具体的拓扑来实现
    virtual int ppid2plid(int ppid) override;
    virtual int plid2ppid(int plid) override;
    virtual int swpid2swlid(int swpid) override;
    virtual int swlid2swpid(int swlid) override;
    virtual int calRoutePort(DataPkt* msg) override;
    virtual int getNextRouterPort(int current_out_port) override; //计算下一个相连的router的端口
    virtual bool connectToProcessor(int port_num) override;
};

Define_Module(SimpleRouter);

int SimpleRouter::ppid2plid(int ppid) {
    //单个路由器 ppid = plid
    return ppid;
}

int SimpleRouter::plid2ppid(int plid) {
    return plid;
}

int SimpleRouter::swpid2swlid(int swpid) {
    return swpid;
}

int SimpleRouter::swlid2swpid(int swlid) {
    return swlid;
}

int SimpleRouter::calRoutePort(DataPkt* msg) {
    int dst_ppid = msg->getDst_ppid();
    return dst_ppid;
}

int SimpleRouter::getNextRouterPort(int current_out_port) {
    //由于只有一个路由器，路由器与processor直接相连，因此接收端口为processor的0端口
    return 0;
}

bool SimpleRouter::connectToProcessor(int port_num) {
    return true;
}
