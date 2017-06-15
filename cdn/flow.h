//
// Created by fang.junpeng on 2017/3/28.
//
#include <stack>
#include "SmartPtr.h"
#include "Timer.h"

using namespace ffun;

struct Net;

#ifndef HUAWEI_RT_FLOW_H
#define HUAWEI_RT_FLOW_H

#define INF 6000
#define MAX_PATH_NUM 50000

struct Flow{
    Net* net;
    long MaxFlow;

    int flowOfNode[1002]={0};      //累积增广流量
    int flowOfConNode[500]={0};    //累积增广流量
    int flowOfConNodeCost[500]={0};//累积增广流量费用
    int minCost;//记录每次最小费用流的总费用
    int miniCostUntilNow;
    int	FeasiblePath[MAX_PATH_NUM][1002];	//潜在最优路径
    int FeasiblePathNum;//路径计数
    std::stack<int>	bestPath[MAX_PATH_NUM];//目前为止最优路径
    int bestPathNum;//路径计数
    Flow();
    void init(Net& net);
};

struct FlowEvent:public TimerEvent{
    FlowEvent(Flow* flow,char* filename);
    virtual void do_event();
private:
    Flow* flow;
    char* filename;
};

#endif //HUAWEI_RT_FLOW_H
