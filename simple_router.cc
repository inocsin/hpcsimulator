/*
 * simple_router.cc
 *
 *  Created on: 2017��5��7��
 *      Author: Vincent
 */

#include "high_radix_router.h"

//SimpleRouter
//����·���������ж˿�����processor������·��������

//public Router, FtRouter, HighRadixRouter
class SimpleRouter : public HighRadixRouter {

protected:
    //���麯�������ݾ����������ʵ��
    virtual int ppid2plid(int ppid) override;
    virtual int plid2ppid(int plid) override;
    virtual int swpid2swlid(int swpid) override;
    virtual int swlid2swpid(int swlid) override;
    virtual int calRoutePort(DataPkt* msg) override;
    virtual int getNextRouterPort(int current_out_port) override; //������һ��������router�Ķ˿�
    virtual bool connectToProcessor(int port_num) override;
};

Define_Module(SimpleRouter);

int SimpleRouter::ppid2plid(int ppid) {
    //����·���� ppid = plid
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
    //����ֻ��һ��·������·������processorֱ����������˽��ն˿�Ϊprocessor��0�˿�
    return 0;
}

bool SimpleRouter::connectToProcessor(int port_num) {
    return true;
}
