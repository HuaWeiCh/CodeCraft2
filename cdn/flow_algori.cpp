//
// Created by fang.junpeng on 2017/3/30.
//
#include "flow_algori.h"
//#include <iostream>
#include <queue>
#include <list>
//#include <string>
#include <cstring>

using namespace std;

//#define groupNum 6					//定义种群大小
//#define NG 1000							//种群的最大繁殖代数

/*********************************************************************
**************************XJBS函数:贪心加松弛***************************
*********************************************************************/
list<int>   directSvericeTable;
list<int>   commonSvericeTable;
list<int>   bestDirectSvericeTable;
list<int>   bestCommonSvericeTable;
//stack<int>  clloctDissatisfyConNode;      //记录未满足的消费节点，或费用大于直连的情况；更新在最小费用最大流里
bool      ifIncommonSvericeTableFlag[1002]; //防止遍历到两个消费节点共有一个网络节点,一直选取却不满足，陷入死循环
//被选中后便被标记，一个step有效，意外被删去本step内不能再次加入
int       relaxNode=-1;                     //上次松弛的网络节点，
bool      ifRelaxationFlag[1002];           //是否可以松弛的标记，初始为true


void xjbs(Net& net,Flow& flow) {
    long tmpFlow;
    bool stepOverFlag=false;
    //*************************相应变量/数组的初始化***************************
    relaxNode = -1;
    for(int i=0;i<1002;i++) ifRelaxationFlag[i] = true;
    for(int i=0;i<1002;i++) ifIncommonSvericeTableFlag[i] = false;
    //*************************初始化直连服务器列表**************************
    for(int i=0;i<net.consumNodeNum;i++)    directSvericeTable.push_front(net.conNode[i].linkNode);
    //*************************初始化链表最优解组合**************************
    for(int i=0;i<net.consumNodeNum;i++)    bestDirectSvericeTable.push_front(net.conNode[i].linkNode);
    //****************************进入xjbs模式*******************************
    while(!ifExit(net,flow)) {
        for(int i=0;i<1002;i++) ifIncommonSvericeTableFlag[i]=false;//去掉对一些特殊原因从公共表中被删除的限制
        if(relaxNode>=0) ifIncommonSvericeTableFlag[relaxNode]=true;//为了保证本次松弛的点，本step中不会被再次选中
        while(!ifStepIsOver(net,flow,stepOverFlag)) {
            //*****************根据两张链表初始化服务器列表*******************
            net.serviceTableNum=0;
            if(!directSvericeTable.empty()) {   //无检查重复的操作，理论上不会出现
                for(list<int>::iterator ir=directSvericeTable.begin();ir!=directSvericeTable.end();ir++) {
                    net.serviceTable[net.serviceTableNum]=*ir;
                    net.serviceTableNum++;
                }
            }
            if(!commonSvericeTable.empty()) {   //无检查重复的操作，理论上不会出现
                for(list<int>::iterator ir=commonSvericeTable.begin();ir!=commonSvericeTable.end();ir++){
                    net.serviceTable[net.serviceTableNum] = *ir;
                    net.serviceTableNum++;
                    ifIncommonSvericeTableFlag[*ir] = true; //标记公共列表中元素
                }
            }
            //for(int i=0;i<net.serviceTableNum;i++)  cout<<net.serviceTable[i]<<" ";
            //cout<<endl;
            //***********************运行最小费用最大流函数*************************
            flow.minCost=net.serviceTableNum*net.serverPrice;//操作之前应完成服务器节点的更新
            for(int j=0;j<net.nodeNum+1;j++) {         //重新初始化带宽capacity,包括超级汇点;超级源点的更新在插入中
                for(int k=0;k<net.node[j].linkedNum;k++)
                    net.node[j].capacity[k]=net.node[j].state[k];
            }

            net.freeSuperSource();
            net.insertSuperSource(net.serviceTable,net.serviceTableNum);
            tmpFlow=minCostMaxFlow(net,flow,net.nodeNum+1,net.nodeNum);
            //****************************检查更新*************************
            if(flow.MaxFlow==tmpFlow) {
                //**满足最大流的情况下(防止本step下再次选中陷入死循环) 才检查冗余情况:删除不作为的服务器节点**
                for(list<int>::iterator ir=commonSvericeTable.begin();ir!=commonSvericeTable.end();) {
                    if(flow.flowOfNode[*ir]==0) {
                        flow.minCost -= net.serverPrice;
                        ir=commonSvericeTable.erase(ir);
                    }else{
                        ir++;
                    }
                }           //这里的删除操作不知道会不会出问题??????????
                //*********************两个操作链表的最优解组合存储*********************
                bestDirectSvericeTable.clear();
                bestCommonSvericeTable.clear();
                for(list<int>::iterator ir=directSvericeTable.begin();ir!=directSvericeTable.end();ir++) {
                    bestDirectSvericeTable.push_back(*ir);
                }
                for(list<int>::iterator ir=commonSvericeTable.begin();ir!=commonSvericeTable.end();ir++) {
                    bestCommonSvericeTable.push_back(*ir);
                }
                //***********************最小费用及相应路径的更新**********************
                if(flow.minCost<flow.miniCostUntilNow) {
                    flow.miniCostUntilNow=flow.minCost;//更新最优费用
                    cout<<"miniCostUntilNow:"<<flow.miniCostUntilNow<<endl;

                    //更新最优路径
                    for(int k=0;k<flow.bestPathNum;k++)//清空原来的
                        while(!flow.bestPath[k].empty())    flow.bestPath[k].pop();

                    for(int k=0,v;k<flow.FeasiblePathNum;k++) {//更新
                        v=0;
                        while(flow.FeasiblePath[k][v] != -1) {  //理论上应该有长度的判断，不过实际应该是总有-1存在
                            flow.bestPath[k].push(flow.FeasiblePath[k][v]);v++;
                        }
                    }
                    flow.bestPathNum=flow.FeasiblePathNum;
                }
            }
            //*******************一个step的退出条件检测及相应处理*********************
            if(stepOverFlag) {
                stepOverFlag=false; //标志置位，为下一个step做准备
                //***************两个操作链表写入现有最优解情况下的组合*****************
                directSvericeTable.clear();
                commonSvericeTable.clear();
                for(list<int>::iterator ir=bestDirectSvericeTable.begin();ir!=bestDirectSvericeTable.end();ir++) {
                  directSvericeTable.push_back(*ir);
                }
                for(list<int>::iterator ir=bestCommonSvericeTable.begin();ir!=bestCommonSvericeTable.end();ir++) {
                  commonSvericeTable.push_back(*ir);
                }
                break;      //跳出内层while循环，进入松弛环节
            }
            //*************判断未满足的消费节点，更新directSvericeTable***************
            for(int i=0;i<net.consumNodeNum;i++) {
                if(flow.flowOfConNode[i] < net.conNode[i].demand) {
                    directSvericeTable.push_front(net.conNode[i].linkNode);
                }
            }
            //***********判断超服务器费用的消费节点，更新directSvericeTable*************
            for(int i=0;i<net.consumNodeNum;i++) {
                if(flow.flowOfConNodeCost[i] > net.serverPrice) {
                    directSvericeTable.push_front(net.conNode[i].linkNode);
                }
            }//关于可能的 又超费用，又不满足的情况的如下处置：
            directSvericeTable.unique();//删除直连服务器链表中的重复值
            //***********检测是否完成一个step，在while条件中**************
        }
        //************×*×检测是否继续松弛操作，在while条件中***************
    }
}
/*********************************************************************
***********************ifExit函数:判断松弛是否完成************************
*********************************************************************/
bool ifExit(Net& net,Flow& flow) {
    int tmp=-2;
    static int cycleFlag = 0;//防止小图中commonSvericeTable因为持续松弛，变成空状态而形成循环
    if(!commonSvericeTable.empty()) {
        tmp=commonSvericeTable.front();
        //cout<<"commonSvericeTable.front:"<<commonSvericeTable.front()<<endl;
        if(!ifRelaxationFlag[tmp]) tmp=-1;
        cycleFlag = 0;
    } else {
        cycleFlag++;
    }

    if(tmp==-1) {   //只有commonSvericeTable列表不为空，且列表中全部已经松弛过才会进入
        return true;
    }
    else if(tmp>=0) {    //进入即代表有松弛操作(列表不为空,且没有完全松弛)
        relaxNode=tmp;
        ifRelaxationFlag[relaxNode]=false;//加入已松弛行列,直至全部退出，也不会重新入列
        //************************删除操作**************************
        commonSvericeTable.pop_front();
        return false;
    } else {              //xjbs()的首次进入tmp=-2；不进行松弛操作，直接返回
        if(cycleFlag>=2)    return true;
        return false;
    }
}
/**************************************************************************
***************ifStepIsOver函数:假设一轮的优化算是一个Step*****************
**************************************************************************/
bool ifStepIsOver(Net& net,Flow& flow,bool& flag) {
    int nodeSlected=0;
    int theMaxNum=0;

    for(int i=0;i<net.nodeNum;i++)  net.commonFlag[i]=0;   //预处理，清零上次计数
    if(!directSvericeTable.empty()) {        //查找可能的公共节点
        //commonFlag中保存节点所能到达的消费节点数
        for(list<int>::iterator ir=directSvericeTable.begin();ir!=directSvericeTable.end();ir++) {
            for (int j = 0; j < net.conNode[net.indexOfconNode[*ir]].feasibleServiceNodeNum; j++) {
                net.commonFlag[net.conNode[net.indexOfconNode[*ir]].feasibleServiceNode[j]]++;//累计消费点个数
            }//Bug!Bug!!Bug!!!      最大的逻辑错误
        }
        //for(list<int>::iterator ir=directSvericeTable.begin();ir!=directSvericeTable.end();ir++) {
        //    for (int j = 0; j < net.conNode[*ir].feasibleServiceNodeNum; j++) {
        //        net.commonFlag[net.conNode[*ir].feasibleServiceNode[j]]++;//累计消费点个数
        //    }//Bug!Bug!!Bug!!!      最大的逻辑错误
        //}
    }
    //***************检索出现最多的潜在服务器节点*****************
    //for(int i=0;i<net.nodeNum;i++)  cout<<net.commonFlag[i]<<" ";
    //cout<<endl;

    do {
        for(int j=0;j<net.nodeNum;j++) {
            if((net.commonFlag[j]>theMaxNum)) {
                theMaxNum=net.commonFlag[j];nodeSlected=j;
            }
        }
        if(ifIncommonSvericeTableFlag[nodeSlected]){        //已经被选过，便不会再次被选中
            net.commonFlag[nodeSlected]=0;  theMaxNum = 0;
        }
        else    break;
    }while(1);

    if(theMaxNum>1) {
        //************************nodeSlected加入commonSvericeTable**************************
        if(ifRelaxationFlag[nodeSlected]) { //根据是否松弛，判断nodeSlected点 加入链表的前或后
            //这样基本保证如果链表中有可松弛的点肯定在链表的前方
            commonSvericeTable.push_back(nodeSlected);
            ifIncommonSvericeTableFlag[nodeSlected]=true;
        } else {
            commonSvericeTable.push_front(nodeSlected);
            ifIncommonSvericeTableFlag[nodeSlected]=true;
        }
        //**********删除与nodeSlected对应的多个消费节点相连的节点从directSvericeTable中删除***********
        for(list<int>::iterator ir=directSvericeTable.begin();ir!=directSvericeTable.end();) {
            for(int j=0;j<net.conNode[net.indexOfconNode[*ir]].feasibleServiceNodeNum;j++) {
                if(net.conNode[net.indexOfconNode[*ir]].feasibleServiceNode[j]==nodeSlected) {
                    ir=directSvericeTable.erase(ir);
                    goto	loop1;
                }
            }
            ir++;
            loop1:	;
        }
    } else {
        //***************对两个链表作出调整:这里不需要额外操作*****************
        flag=true;  //标志，用于xjbs(),一个Step的跳出
    }
    return false;   //这里实际不作为跳出的判断；跳出靠上方的flag，在xjbs()中break
}


int		dis[1002];			//spfa函数中到各点最短距离的记录数组
int 	pre[1002];			//spfa函数中前向节点的记录数组
bool	ifVisited[1002];	//spfa函数中记录节点是否访问过
/***************************************************************************
*************minCostMaxFlow函数:计算选定服务器节点的最小费用****************
***************************************************************************/
//1.增广路前半段的初始化问题
//2.超出现有最小费用退出问题
int minCostMaxFlow(Net& net,Flow& flow,int s,int t) {
    int	count;
    int max_flow = 0; // 总流量
    int minflow;

    for(int i=0;i<net.nodeNum;i++)  flow.flowOfNode[i]=0;
    for(int i=0;i<net.consumNodeNum;i++)  flow.flowOfConNode[i]=0;
    for(int i=0;i<net.consumNodeNum;i++)  flow.flowOfConNodeCost[i]=0;

    if(flow.FeasiblePathNum>0) {        //重新初始化FeasiblePath及计数
        for(int i=0,j;i<flow.FeasiblePathNum;i++) {
            j=0;
            while((flow.FeasiblePath[i][j] != -1)&&(j<1000)) {
                flow.FeasiblePath[i][j]=-1;
                j++;
            }
        }
        flow.FeasiblePathNum=0;
    }

    //spfa初始化:每次费用流的首次，以后重写数据将有所不同
    //for(int i=0;i<nodeNum+2;i++)  pre[i]=-1;
    //for(int i=0;i<nodeNum+2;i++)  dis[i]=INF;
    //for(int i=0;i<nodeNum+2;i++)  ifVisited[i]=false;

    int jj = t;//放在for循环外为了记录服务器所提供的流量
    while(spfa(net,flow,s,t)) { //可行增广路径在spfa中完成
        //********************寻找最小带宽处******************
        minflow = INF + 1;
        for(int i = pre[t],j=t;i != s;i = pre[i]) {
            for(int k=0;k<net.node[i].linkedNum;k++) {
                if(net.node[i].linkedNode[k]==j) {
                    if(net.node[i].capacity[k] < minflow) {
                        minflow = net.node[i].capacity[k];
                        //reWriteStandard = dis[j];
                        // cout<<reWriteStandard<<" ";
                    }
                }
            }
            jj=i;j=i;
        }
        //cout<<endl;

        //********************加入一些策略性判断的记录********************
        flow.flowOfConNodeCost[net.indexOfconNode[pre[t]]] +=dis[t] * minflow;//每个消费节点累积费用
        flow.flowOfConNode[net.indexOfconNode[pre[t]]] +=minflow;             //每个消费节点累积通过流量
        flow.flowOfNode[jj] +=minflow;                                        //每个服务器节点累积分发的流量

        max_flow += minflow;                                                  //总流量的累积
        flow.minCost += dis[t] * minflow;                                     //总费用的累积

        //********************以下为记录路径等信息********************
        flow.FeasiblePath[flow.FeasiblePathNum][0]=minflow;                   //加入本条路的流量
        flow.FeasiblePath[flow.FeasiblePathNum][1]=net.indexOfconNode[pre[t]];//加入消费节点
        count=2;
        for(int i = pre[t],j = t;i != s;i = pre[i]) {
            for(int k=0;k<net.node[i].linkedNum;k++) {
                if(net.node[i].linkedNode[k]==j) {
                    net.node[i].capacity[k] -= minflow;
                    flow.FeasiblePath[flow.FeasiblePathNum][count]=i;
                    count++;
                }
            }
            j=i;
        }
        flow.FeasiblePathNum++;
    }
    return	max_flow;// 最大流
}

bool spfa(Net& net,Flow& flow,int s,int t) {
    int u,v;
    queue<int>q;

    //spfa的初始化
    for(int i=0;i<net.nodeNum+2;i++)  pre[i]=-1;
    for(int i=0;i<net.nodeNum+2;i++)  dis[i]=INF;
    for(int i=0;i<net.nodeNum+2;i++)  ifVisited[i]=false;

    dis[s] = 0;
    ifVisited[s] = true;
    q.push(s);                //superSource入队

    while(!q.empty()) {
        u = q.front();
        q.pop();
        ifVisited[u] = false;//release from queue q

        if(u != net.nodeNum) {   //超级汇点没有向外分发功能
            for(int i = 0; i < net.node[u].linkedNum; i++) {
                v = net.node[u].linkedNode[i];
                if((net.node[u].capacity[i]>0) && (dis[v] > (dis[u]+net.node[u].cost[i]))) {
                    dis[v] = dis[u] + net.node[u].cost[i];
                    pre[v] = u;				    //用于前向搜索
                    if(!ifVisited[v]) {
                        q.push(v);
                        ifVisited[v]=true;
                    }
                }
            }
        }
    }

    if(dis[t] == INF)	return false;
    return true;
}
