/*
 * simple_processor.cc
 *
 *  Created on: 2017年5月7日
 *      Author: Vincent
 */

#include "processor.h"

class SimpleProcessor : public Processor {
protected:
    //纯虚函数，由子类实现
    virtual int ppid2plid(int ppid) override;
    virtual int plid2ppid(int plid) override;
    virtual int getNextRouterPortP() override; //计算与processor相连的router的端口
};

Define_Module(SimpleProcessor);

int SimpleProcessor::ppid2plid(int ppid) {
    return ppid;
}

int SimpleProcessor::plid2ppid(int plid) {
    return plid;
}

int SimpleProcessor::getNextRouterPortP() {
    return getIndex();
}
