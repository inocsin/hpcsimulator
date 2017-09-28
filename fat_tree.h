#ifndef FAT_TREE_H_
#define FAT_TREE_H_

#include "router.h"
#include "fat_tree_topo.h"

class FtRouter : public Router {

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

Define_Module(FtRouter);

#endif /* FAT_TREE_H_ */
