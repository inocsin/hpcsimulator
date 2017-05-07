/*
 * simple_processor.cc
 *
 *  Created on: 2017��5��7��
 *      Author: Vincent
 */

#include "processor.h"

class SimpleProcessor : public Processor {
protected:
    //���麯����������ʵ��
    virtual int ppid2plid(int ppid) override;
    virtual int plid2ppid(int plid) override;
    virtual int getNextRouterPortP() override; //������processor������router�Ķ˿�
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
