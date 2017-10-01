#ifndef FT_ROUTER_H_
#define FT_ROUTER_H_

#include "router.h"
#include "fat_tree_topo.h"

class FtRouter : public Router {

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

Define_Module(FtRouter);

#endif /* FT_ROUTER_H_ */
